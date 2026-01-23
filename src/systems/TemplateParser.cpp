/**
 * @file TemplateParser.cpp
 * @brief HTML template parser with Vue-like directives (v-if, v-for, {{ }}, @click)
 * @lines ~825
 *
 * Main entry point:
 * - Evaluate() - Full template evaluation with perf instrumentation (line 70, ~80 lines)
 *
 * Core parsing pipeline:
 * - Parse() - Gumbo HTML parse with caching (line 37, ~30 lines)
 * - ProcessNode() - Recursive DOM walker (line 152, ~55 lines)
 * - SerializeElement() - HTML output builder with directive processing (line 210, ~165 lines)
 *
 * Directive handlers:
 * - ProcessVForElement() - v-for list rendering (line 376, ~40 lines)
 * - SerializeElementForIteration() - v-for item template (line 417, ~150 lines)
 * - ProcessInterpolations() - {{ expression }} expansion (line 664, ~25 lines)
 * - ProcessBindClass() - :class="expr" binding (line 692, ~55 lines)
 *
 * Dependencies:
 * - Gumbo: HTML5 parsing (output cached after first parse)
 * - LuaUIState: Expression evaluation (v-if conditions, {{ }} values)
 * - ExpressionCache: Compiled Lua functions (via LuaUIState)
 *
 * Performance:
 * - Baseline: ~3.2ms average (824 lines template)
 * - Budget: 10ms (currently at 32% of budget)
 * - Instrumented: min/max/avg tracking every 100 evaluations
 */

#include "systems/TemplateParser.h"
#include "util/Logger.h"
#include <gumbo.h>
#include "util/Logger.h"
#include <regex>
#include <sstream>
#include <map>
#include <chrono>

TemplateParser::TemplateParser()
{
}

TemplateParser::~TemplateParser()
{
    FreeCachedGumbo();
}

void TemplateParser::FreeCachedGumbo()
{
    if (m_cachedGumboOutput && m_gumboOwned)
    {
        GumboOptions options = kGumboDefaultOptions;
        gumbo_destroy_output(&options, m_cachedGumboOutput);
        m_cachedGumboOutput = nullptr;
        m_gumboOwned = false;
    }
}

std::string TemplateParser::HashTemplate(const std::string& str)
{
    // Simple hash using std::hash - sufficient for change detection
    std::hash<std::string> hasher;
    return std::to_string(hasher(str));
}

void TemplateParser::Parse(const std::string &html)
{
    m_template = html;
    m_directives.clear();

    // Compute hash for change detection
    std::string newHash = HashTemplate(html);

    // Check if template actually changed
    if (newHash == m_cachedTemplateHash && m_cachedGumboOutput)
    {
        LOG_TRACE_L2("[TemplateParser] Template unchanged, reusing cached parse ({} bytes)", html.size());
        return;
    }

    // Template changed - need to reparse
    LOG_TRACE_L2("[TemplateParser] Parsing template ({} bytes) with Gumbo", html.size());

    // Free old cached output
    FreeCachedGumbo();

    // Parse and cache the new output
    GumboOptions options = kGumboDefaultOptions;
    m_cachedGumboOutput = gumbo_parse_with_options(&options, m_template.data(), m_template.length());
    m_gumboOwned = true;
    m_cachedTemplateHash = newHash;

    if (!m_cachedGumboOutput)
    {
        LOG_ERROR("[TemplateParser] Failed to parse HTML with Gumbo");
    }
}

std::string TemplateParser::Evaluate(LuaUIState &state)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    if (!state.IsReady())
    {
        LOG_ERROR("[TemplateParser] Lua state not ready");
        return "";
    }

    // Reset event handlers for fresh evaluation
    ResetEventHandlers();

    // Use cached Gumbo output if available (Phase 1 optimization)
    GumboOutput *output = m_cachedGumboOutput;
    bool usedCache = (output != nullptr);

    // If not cached, parse now (shouldn't happen if Parse() was called first)
    bool needsCleanup = false;
    if (!output)
    {
        LOG_TRACE_L2("[TemplateParser] No cached Gumbo output, parsing now");
        GumboOptions options = kGumboDefaultOptions;
        output = gumbo_parse_with_options(&options, m_template.data(), m_template.length());
        needsCleanup = true;  // We created this, so we need to clean it up

        if (!output)
        {
            LOG_ERROR("[TemplateParser] Failed to parse HTML with Gumbo");
            return "";
        }
    }

    // Process the tree
    std::string result = ProcessNode(output->root, state);

    // Only cleanup if we created a temporary output (not cached)
    if (needsCleanup)
    {
        GumboOptions options = kGumboDefaultOptions;
        gumbo_destroy_output(&options, output);
    }

    // Performance metrics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    // Track statistics
    m_totalEvaluations++;
    m_totalEvaluationTimeUs += durationUs;
    if (durationUs > m_maxEvaluationTimeUs)
    {
        m_maxEvaluationTimeUs = durationUs;
    }
    if (durationUs < m_minEvaluationTimeUs)
    {
        m_minEvaluationTimeUs = durationUs;
    }

    // Log timing (DEBUG level for regular, WARNING for slow renders)
    constexpr int64_t PERF_BUDGET_US = 10000;  // 10ms budget
    if (durationUs > PERF_BUDGET_US)
    {
        LOG_WARNING("[TemplateParser] PERF: Evaluate took {}μs (budget: {}μs, gumbo_cached: {})",
                    durationUs, PERF_BUDGET_US, usedCache);
    }
    else
    {
        LOG_TRACE_L2("[TemplateParser] Evaluate took {}μs (gumbo_cached: {})", durationUs, usedCache);
    }

    // Log summary stats every 100 evaluations
    if (m_totalEvaluations % 100 == 0)
    {
        int64_t avgUs = m_totalEvaluations > 0 ? m_totalEvaluationTimeUs / m_totalEvaluations : 0;
        LOG_INFO("[TemplateParser] Stats after {} evals: avg={}μs, min={}μs, max={}μs",
                 m_totalEvaluations, avgUs, m_minEvaluationTimeUs, m_maxEvaluationTimeUs);
    }

    return result;
}

std::string TemplateParser::ProcessNode(GumboNode *node, LuaUIState &state)
{
    if (!node)
        return "";

    if (node->type == GUMBO_NODE_TEXT || node->type == GUMBO_NODE_WHITESPACE)
    {
        // Process text node - handle {{}} interpolations
        std::string text(node->v.text.text);
        return ProcessInterpolations(text, state);
    }

    if (node->type == GUMBO_NODE_ELEMENT)
    {
        GumboElement &element = node->v.element;

        // Check for v-if attribute
        GumboAttribute *vif = gumbo_get_attribute(&element.attributes, "v-if");
        if (vif)
        {
            bool condition = state.EvaluateCondition(vif->value);
            if (!condition)
            {
                // Skip this element and its children
                return "";
            }
        }

        // Check for v-table attribute (must come before v-for)
        GumboAttribute *vtable = gumbo_get_attribute(&element.attributes, "v-table");
        if (vtable)
        {
            return ProcessVTableElement(node, vtable->value, state);
        }

        // Check for v-for attribute
        GumboAttribute *vfor = gumbo_get_attribute(&element.attributes, "v-for");
        if (vfor)
        {
            return ProcessVForElement(node, vfor->value, state);
        }

        // Regular element - serialize it
        return SerializeElement(node, state);
    }

    if (node->type == GUMBO_NODE_DOCUMENT)
    {
        // For document node, just process children
        // Use string concatenation with reserve for better performance
        std::string result;
        result.reserve(m_template.size()); // Estimate output size from template size

        GumboVector *children = &node->v.document.children;
        for (unsigned int i = 0; i < children->length; i++)
        {
            GumboNode *child = static_cast<GumboNode *>(children->data[i]);
            result += ProcessNode(child, state);
        }
        return result;
    }

    return "";
}

std::string TemplateParser::SerializeElement(GumboNode *node, LuaUIState &state)
{
    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return "";

    GumboElement &element = node->v.element;
    std::ostringstream oss;

    // Opening tag
    oss << "<" << gumbo_normalized_tagname(element.tag);

    // First pass: detect @event directives and collect special attributes
    std::string elemId;
    std::map<std::string, std::string> eventHandlers;
    std::string vModelExpr;
    std::string vBindClassExpr;
    std::string existingClass;

    for (unsigned int i = 0; i < element.attributes.length; i++)
    {
        GumboAttribute *attr = static_cast<GumboAttribute *>(element.attributes.data[i]);
        std::string attrName(attr->name);

        // Check for @event directive (e.g., @click, @mouseover)
        if (attrName.length() > 1 && attrName[0] == '@')
        {
            std::string eventType = attrName.substr(1); // Remove '@' prefix
            std::string handlerExpr(attr->value);

            // Generate element ID if first @event found
            if (elemId.empty())
            {
                elemId = "event_" + std::to_string(m_nextEventId++);
            }

            eventHandlers[eventType] = handlerExpr;
            LOG_TRACE_L1("[TemplateParser] Found @{} directive: {} -> {}", eventType, elemId, handlerExpr);
        }
        // Check for v-model
        else if (attrName == "v-model")
        {
            vModelExpr = attr->value;
        }
        // Check for :class or v-bind:class
        else if (attrName == ":class" || attrName == "v-bind:class")
        {
            vBindClassExpr = attr->value;
        }
        // Collect existing class attribute
        else if (attrName == "class")
        {
            existingClass = attr->value;
        }
    }

    // If element has event handlers, add data-event-id and store handlers
    if (!elemId.empty())
    {
        oss << " data-event-id=\"" << elemId << "\"";
        m_eventHandlers[elemId] = eventHandlers;
    }

    // Process :class / v-bind:class directive
    std::string finalClass = existingClass;
    if (!vBindClassExpr.empty())
    {
        std::string dynamicClasses = ProcessBindClass(vBindClassExpr, state);
        if (!dynamicClasses.empty())
        {
            if (!finalClass.empty())
            {
                finalClass += " ";
            }
            finalClass += dynamicClasses;
        }
    }

    // Second pass: serialize regular attributes (skip directives)
    bool classWritten = false;
    for (unsigned int i = 0; i < element.attributes.length; i++)
    {
        GumboAttribute *attr = static_cast<GumboAttribute *>(element.attributes.data[i]);
        std::string attrName(attr->name);

        // Skip directive attributes
        if (attrName == "v-if" || attrName == "v-for" || attrName == "v-model" ||
            attrName == ":class" || attrName == "v-bind:class" ||
            attrName == ":value" || attrName == "v-bind:value" ||
            attrName == "v-html")
        {
            continue;
        }

        // Skip @event directives (already processed)
        if (attrName.length() > 1 && attrName[0] == '@')
        {
            continue;
        }

        // Handle class attribute specially (merge with :class)
        if (attrName == "class")
        {
            oss << " class=\"" << finalClass << "\"";
            classWritten = true;
            continue;
        }

        oss << " " << attr->name;
        if (attr->value && strlen(attr->value) > 0)
        {
            // Process interpolations in attribute values
            std::string attrValue(attr->value);
            std::string processedValue = ProcessInterpolations(attrValue, state);
            oss << "=\"" << processedValue << "\"";
        }
    }

    // Write class if it wasn't in original attributes but we have dynamic classes
    if (!classWritten && !finalClass.empty())
    {
        oss << " class=\"" << finalClass << "\"";
    }

    // Handle v-model for input elements - add value attribute
    if (!vModelExpr.empty() && element.tag == GUMBO_TAG_INPUT)
    {
        std::string value = state.EvaluateAsString(vModelExpr);
        oss << " value=\"" << value << "\"";
    }

    oss << ">";

    // Handle v-model for textarea - inject content
    if (!vModelExpr.empty() && element.tag == GUMBO_TAG_TEXTAREA)
    {
        std::string content = state.EvaluateAsString(vModelExpr);
        oss << content;
    }
    // Handle v-html directive
    else if (GumboAttribute *vhtml = gumbo_get_attribute(&element.attributes, "v-html"))
    {
        std::string htmlContent = state.EvaluateAsString(vhtml->value);
        oss << htmlContent;
    }
    else
    {
        // Regular children processing
        GumboVector *children = &element.children;
        for (unsigned int i = 0; i < children->length; i++)
        {
            GumboNode *child = static_cast<GumboNode *>(children->data[i]);
            oss << ProcessNode(child, state);
        }
    }

    // Closing tag (only for non-void elements)
    if (element.tag != GUMBO_TAG_BR && element.tag != GUMBO_TAG_IMG &&
        element.tag != GUMBO_TAG_INPUT && element.tag != GUMBO_TAG_META &&
        element.tag != GUMBO_TAG_LINK)
    {
        oss << "</" << gumbo_normalized_tagname(element.tag) << ">";
    }

    return oss.str();
}

std::string TemplateParser::ProcessVForElement(GumboNode *node, const std::string &expression, LuaUIState &state)
{
    // Parse v-for expression: "item in collection"
    std::regex forExprRegex(R"(\s*(\w+)\s+in\s+(.+)\s*)");
    std::smatch match;

    if (!std::regex_match(expression, match, forExprRegex))
    {
        LOG_ERROR("[TemplateParser] Invalid v-for expression: {}", expression);
        return "";
    }

    std::string itemVar = match[1].str();
    std::string collectionExpr = match[2].str();

    // Get the collection from Lua state
    sol::object collection = state.GetValue(collectionExpr);

    if (!collection.is<sol::table>())
    {
        LOG_ERROR("[TemplateParser] v-for collection not found or not a table: {}", collectionExpr);
        return "";
    }

    sol::table collectionTable = collection.as<sol::table>();
    std::ostringstream result;

    // Iterate over collection
    for (auto &pair : collectionTable)
    {
        sol::object item = pair.second;

        // Clone the element for this iteration
        // For now, we'll serialize and re-process with item context
        std::string elementHTML = SerializeElementForIteration(node, state, itemVar, item);
        result << elementHTML;
    }

    return result.str();
}

std::string TemplateParser::ProcessVTableElement(GumboNode *node, const std::string &expression, LuaUIState &state)
{
    // Get the table data from Lua state
    sol::object tableDataObj = state.GetValue(expression);

    if (!tableDataObj.is<sol::table>())
    {
        LOG_ERROR("[TemplateParser] v-table data not found or not a table: {}", expression);
        return "";
    }

    sol::table tableData = tableDataObj.as<sol::table>();

    // Try to get columns/headers
    sol::object columnsObj = tableData["columns"];
    if (!columnsObj.valid() || columnsObj.get_type() == sol::type::nil)
    {
        // Try alternative naming: "headers"
        columnsObj = tableData["headers"];
    }

    // Try to get rows/data
    sol::object rowsObj = tableData["rows"];
    if (!rowsObj.valid() || rowsObj.get_type() == sol::type::nil)
    {
        // Try alternative naming: "data"
        rowsObj = tableData["data"];
    }

    if (!columnsObj.is<sol::table>() || !rowsObj.is<sol::table>())
    {
        LOG_ERROR("[TemplateParser] v-table requires 'columns' (or 'headers') and 'rows' (or 'data') arrays");
        return "";
    }

    sol::table columns = columnsObj.as<sol::table>();
    sol::table rows = rowsObj.as<sol::table>();

    // Get node tag name to preserve container
    GumboElement &element = node->v.element;
    const char* tagName = gumbo_normalized_tagname(element.tag);

    // Build result HTML
    std::ostringstream oss;

    // Start container tag (preserve original tag, but usually <table>)
    oss << "<" << tagName;

    // Copy attributes from original element (except v-table)
    for (unsigned int i = 0; i < element.attributes.length; i++)
    {
        GumboAttribute *attr = static_cast<GumboAttribute *>(element.attributes.data[i]);
        std::string attrName(attr->name);

        // Skip v-table directive itself
        if (attrName == "v-table")
            continue;

        oss << " " << attr->name;
        if (attr->value && strlen(attr->value) > 0)
        {
            oss << "=\"" << attr->value << "\"";
        }
    }

    oss << ">";

    // Generate thead
    oss << "<thead><tr>";
    for (auto &colPair : columns)
    {
        sol::object colObj = colPair.second;
        std::string colName;
        if (colObj.is<std::string>())
        {
            colName = colObj.as<std::string>();
        }
        else if (colObj.is<int>())
        {
            colName = std::to_string(colObj.as<int>());
        }
        else if (colObj.is<double>())
        {
            colName = std::to_string(colObj.as<double>());
        }
        else
        {
            colName = "Column";
        }
        oss << "<th>" << colName << "</th>";
    }
    oss << "</tr></thead>";

    // Generate tbody
    oss << "<tbody>";
    for (auto &rowPair : rows)
    {
        sol::object rowObj = rowPair.second;
        if (!rowObj.is<sol::table>())
            continue;

        sol::table rowTable = rowObj.as<sol::table>();
        oss << "<tr>";

        for (auto &cellPair : rowTable)
        {
            sol::object cellObj = cellPair.second;
            std::string cellValue;

            if (cellObj.is<std::string>())
            {
                cellValue = cellObj.as<std::string>();
            }
            else if (cellObj.is<int>())
            {
                cellValue = std::to_string(cellObj.as<int>());
            }
            else if (cellObj.is<double>())
            {
                cellValue = std::to_string(cellObj.as<double>());
            }
            else if (cellObj.is<bool>())
            {
                cellValue = cellObj.as<bool>() ? "true" : "false";
            }
            else
            {
                cellValue = "";
            }

            oss << "<td>" << cellValue << "</td>";
        }

        oss << "</tr>";
    }
    oss << "</tbody>";

    // Close container tag
    oss << "</" << tagName << ">";

    return oss.str();
}

std::string TemplateParser::SerializeElementForIteration(GumboNode *node, LuaUIState &state,
                                                         const std::string &itemVar, sol::object &item)
{
    if (!node || node->type != GUMBO_NODE_ELEMENT)
        return "";

    GumboElement &element = node->v.element;
    std::ostringstream oss;

    // Opening tag
    oss << "<" << gumbo_normalized_tagname(element.tag);

    // First pass: detect @event directives and collect special attributes
    std::string elemId;
    std::map<std::string, std::string> eventHandlers;
    std::string vBindClassExpr;
    std::string existingClass;

    for (unsigned int i = 0; i < element.attributes.length; i++)
    {
        GumboAttribute *attr = static_cast<GumboAttribute *>(element.attributes.data[i]);
        std::string attrName(attr->name);

        // Check for @event directive (e.g., @click, @mouseover)
        if (attrName.length() > 1 && attrName[0] == '@')
        {
            std::string eventType = attrName.substr(1); // Remove '@' prefix
            std::string handlerExpr(attr->value);

            // Generate element ID if first @event found
            if (elemId.empty())
            {
                elemId = "event_" + std::to_string(m_nextEventId++);
            }

            // Substitute loop variable values in handler expression
            // e.g., "selectEntity(entity.id)" becomes "selectEntity(1)" if entity.id == 1
            std::string processedHandlerExpr = ProcessIterationInterpolations(handlerExpr, itemVar, item);

            eventHandlers[eventType] = processedHandlerExpr;
            LOG_TRACE_L1("[TemplateParser] Found @{} directive in v-for: {} -> {} (original: {})",
                         eventType, elemId, processedHandlerExpr, handlerExpr);
        }
        // Check for :class or v-bind:class
        else if (attrName == ":class" || attrName == "v-bind:class")
        {
            vBindClassExpr = attr->value;
        }
        // Collect existing class attribute
        else if (attrName == "class")
        {
            existingClass = attr->value;
        }
    }

    // If element has event handlers, add data-event-id and store handlers
    if (!elemId.empty())
    {
        oss << " data-event-id=\"" << elemId << "\"";
        m_eventHandlers[elemId] = eventHandlers;
    }

    // Process :class / v-bind:class directive with iteration context
    std::string finalClass = existingClass;
    if (!vBindClassExpr.empty())
    {
        // First substitute iteration variables in the expression
        std::string processedExpr = ProcessIterationInterpolations(vBindClassExpr, itemVar, item);
        std::string dynamicClasses = ProcessBindClass(processedExpr, state);
        if (!dynamicClasses.empty())
        {
            if (!finalClass.empty())
            {
                finalClass += " ";
            }
            finalClass += dynamicClasses;
        }
    }

    // Second pass: serialize regular attributes (skip v-for, @event, and :class)
    bool classWritten = false;
    for (unsigned int i = 0; i < element.attributes.length; i++)
    {
        GumboAttribute *attr = static_cast<GumboAttribute *>(element.attributes.data[i]);
        std::string attrName(attr->name);

        // Skip directive attributes
        if (attrName == "v-for" || attrName == ":class" || attrName == "v-bind:class")
        {
            continue;
        }

        // Skip @event directives (already processed)
        if (attrName.length() > 1 && attrName[0] == '@')
        {
            continue;
        }

        // Handle class attribute specially (merge with :class)
        if (attrName == "class")
        {
            oss << " class=\"" << finalClass << "\"";
            classWritten = true;
            continue;
        }

        oss << " " << attr->name;
        if (attr->value && strlen(attr->value) > 0)
        {
            // Process iteration interpolations in attribute values
            std::string attrValue(attr->value);
            std::string processedValue = ProcessIterationInterpolations(attrValue, itemVar, item);
            oss << "=\"" << processedValue << "\"";
        }
    }

    // Write class if it wasn't in original attributes but we have dynamic classes
    if (!classWritten && !finalClass.empty())
    {
        oss << " class=\"" << finalClass << "\"";
    }

    oss << ">";

    // Process children with item context
    GumboVector *children = &element.children;
    for (unsigned int i = 0; i < children->length; i++)
    {
        GumboNode *child = static_cast<GumboNode *>(children->data[i]);

        if (child->type == GUMBO_NODE_TEXT || child->type == GUMBO_NODE_WHITESPACE)
        {
            std::string text(child->v.text.text);
            // Replace {{itemVar.property}} with values from item
            std::string processed = ProcessIterationInterpolations(text, itemVar, item);
            oss << processed;
        }
        else
        {
            // Recursively process child elements
            oss << SerializeElementForIteration(child, state, itemVar, item);
        }
    }

    // Closing tag
    if (element.tag != GUMBO_TAG_BR && element.tag != GUMBO_TAG_IMG &&
        element.tag != GUMBO_TAG_INPUT && element.tag != GUMBO_TAG_META &&
        element.tag != GUMBO_TAG_LINK)
    {
        oss << "</" << gumbo_normalized_tagname(element.tag) << ">";
    }

    return oss.str();
}

std::string TemplateParser::ProcessIterationInterpolations(const std::string &text,
                                                           const std::string &itemVar,
                                                           sol::object &item)
{
    std::string result = text;

    // PASS 1: Find {{itemVar.property}} patterns (mustache syntax)
    std::regex itemRefRegex(R"(\{\{\s*)" + itemVar + R"(\.(\w+)\s*\}\})");
    std::smatch match;
    std::string searchStr = result;
    std::ostringstream output;

    while (std::regex_search(searchStr, match, itemRefRegex))
    {
        output << searchStr.substr(0, match.position());

        std::string property = match[1].str();

        // Get value from item
        std::string valueStr = "";
        if (item.is<sol::table>())
        {
            sol::table itemTable = item.as<sol::table>();
            sol::object value = itemTable[property];

            if (value.is<std::string>())
            {
                valueStr = value.as<std::string>();
            }
            else if (value.is<int>())
            {
                valueStr = std::to_string(value.as<int>());
            }
            else if (value.is<double>())
            {
                valueStr = std::to_string(value.as<double>());
            }
        }

        output << valueStr;
        searchStr = match.suffix();
    }

    output << searchStr;
    result = output.str();

    // PASS 2: Find bare itemVar.property patterns (for event handler arguments)
    // This handles cases like @click="selectEntity(entity.id)"
    std::regex bareRefRegex(itemVar + R"(\.(\w+))");
    std::smatch bareMatch;
    std::string searchStr2 = result;
    std::ostringstream output2;

    while (std::regex_search(searchStr2, bareMatch, bareRefRegex))
    {
        output2 << searchStr2.substr(0, bareMatch.position());

        std::string property = bareMatch[1].str();

        // Get value from item (same logic as mustache case)
        std::string valueStr = "";
        if (item.is<sol::table>())
        {
            sol::table itemTable = item.as<sol::table>();
            sol::object value = itemTable[property];

            if (value.is<std::string>())
            {
                valueStr = value.as<std::string>();
            }
            else if (value.is<int>())
            {
                valueStr = std::to_string(value.as<int>());
            }
            else if (value.is<uint32_t>())
            {
                valueStr = std::to_string(value.as<uint32_t>());
            }
            else if (value.is<double>())
            {
                valueStr = std::to_string(value.as<double>());
            }
        }

        output2 << valueStr;
        searchStr2 = bareMatch.suffix();
    }

    output2 << searchStr2;
    return output2.str();
}

std::string TemplateParser::ProcessInterpolations(const std::string &text, LuaUIState &state)
{
    // Static regex - compiled once, reused for all calls (Phase 1 optimization)
    static const std::regex interpolationRegex(R"(\{\{\s*(.+?)\s*\}\})");

    std::string result;
    result.reserve(text.size()); // Reserve approximate size

    std::smatch match;
    std::string searchStr = text;

    while (std::regex_search(searchStr, match, interpolationRegex))
    {
        result += searchStr.substr(0, match.position());

        std::string expression = match[1].str();

        // Evaluate the expression with state context
        std::string value = state.EvaluateAsString(expression);
        result += value;

        searchStr = match.suffix();
    }

    result += searchStr;
    return result;
}

std::string TemplateParser::ProcessBindClass(const std::string &expr, LuaUIState &state)
{
    std::string result;

    // Handle object syntax: {active: condition, disabled: otherCondition}
    if (!expr.empty() && expr[0] == '{')
    {
        // Parse object syntax: extract key-value pairs
        // Format: {className: condition, className2: condition2}
        std::regex pairRegex(R"((\w+)\s*:\s*([^,}]+))");
        std::smatch match;
        std::string searchStr = expr;

        while (std::regex_search(searchStr, match, pairRegex))
        {
            std::string className = match[1].str();
            std::string condition = match[2].str();

            // Trim whitespace from condition
            size_t start = condition.find_first_not_of(" \t");
            size_t end = condition.find_last_not_of(" \t");
            if (start != std::string::npos)
            {
                condition = condition.substr(start, end - start + 1);
            }

            // Evaluate condition
            bool conditionResult = state.EvaluateCondition(condition);
            if (conditionResult)
            {
                if (!result.empty())
                {
                    result += " ";
                }
                result += className;
            }

            searchStr = match.suffix();
        }
    }
    // Handle string concatenation syntax (Lua): 'base-class' .. (condition and ' active' or '')
    else if (expr.find("..") != std::string::npos)
    {
        // Evaluate as Lua expression
        result = state.EvaluateAsString(expr);
    }
    // Handle simple expression
    else
    {
        result = state.EvaluateAsString(expr);
    }

    return result;
}

// Unused helper methods (kept for potential future use)

std::string TemplateParser::ProcessVFor(const std::string &html, LuaUIState &state)
{
    // Deprecated - now using gumbo-based approach
    return html;
}

std::string TemplateParser::ProcessVIf(const std::string &html, LuaUIState &state)
{
    // Deprecated - now using gumbo-based approach
    return html;
}

std::string TemplateParser::ExtractTagName(const std::string &tag)
{
    std::regex tagNameRegex(R"(<\s*(\w+))");
    std::smatch match;
    if (std::regex_search(tag, match, tagNameRegex))
    {
        return match[1].str();
    }
    return "";
}

std::string TemplateParser::GetAttribute(const std::string &tag, const std::string &attrName)
{
    std::regex attrRegex(attrName + R"(\s*=\s*['"]([^'"]*)['"])");
    std::smatch match;
    if (std::regex_search(tag, match, attrRegex))
    {
        return match[1].str();
    }
    return "";
}

std::string TemplateParser::RemoveAttribute(const std::string &tag, const std::string &attrName)
{
    std::regex attrRegex(R"(\s+)" + attrName + R"(\s*=\s*['"][^'"]*['"])");
    return std::regex_replace(tag, attrRegex, "");
}

size_t TemplateParser::FindClosingTag(const std::string &html, const std::string &tagName, size_t startPos)
{
    int depth = 1;
    size_t pos = startPos;

    std::string openTag = "<" + tagName;
    std::string closeTag = "</" + tagName + ">";

    while (depth > 0 && pos < html.length())
    {
        size_t nextOpen = html.find(openTag, pos);
        size_t nextClose = html.find(closeTag, pos);

        if (nextClose == std::string::npos)
        {
            return std::string::npos;
        }

        if (nextOpen != std::string::npos && nextOpen < nextClose)
        {
            depth++;
            pos = nextOpen + openTag.length();
        }
        else
        {
            depth--;
            if (depth == 0)
            {
                return nextClose;
            }
            pos = nextClose + closeTag.length();
        }
    }

    return std::string::npos;
}
