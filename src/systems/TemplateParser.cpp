#include "systems/TemplateParser.h"
#include <gumbo.h>
#include <iostream>
#include <regex>
#include <sstream>

TemplateParser::TemplateParser() {
}

void TemplateParser::Parse(const std::string& html) {
    m_template = html;
    m_directives.clear();

    std::cout << "[TemplateParser] Parsing template (" << html.size() << " bytes) with Gumbo" << std::endl;

    // Template is stored as-is, parsing happens during evaluation
}

std::string TemplateParser::Evaluate(LuaUIState& state) {
    if (!state.IsReady()) {
        std::cerr << "[TemplateParser] Lua state not ready" << std::endl;
        return "";
    }

    // Parse HTML with Gumbo
    GumboOptions options = kGumboDefaultOptions;
    GumboOutput* output = gumbo_parse_with_options(&options, m_template.data(), m_template.length());

    if (!output) {
        std::cerr << "[TemplateParser] Failed to parse HTML with Gumbo" << std::endl;
        return "";
    }

    // Process the tree
    std::string result = ProcessNode(output->root, state);

    // Cleanup
    gumbo_destroy_output(&options, output);

    return result;
}

std::string TemplateParser::ProcessNode(GumboNode* node, LuaUIState& state) {
    if (!node) return "";

    if (node->type == GUMBO_NODE_TEXT || node->type == GUMBO_NODE_WHITESPACE) {
        // Process text node - handle {{}} interpolations
        std::string text(node->v.text.text);
        return ProcessInterpolations(text, state);
    }

    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboElement& element = node->v.element;

        // Check for v-if attribute
        GumboAttribute* vif = gumbo_get_attribute(&element.attributes, "v-if");
        if (vif) {
            bool condition = state.EvaluateCondition(vif->value);
            if (!condition) {
                // Skip this element and its children
                return "";
            }
        }

        // Check for v-for attribute
        GumboAttribute* vfor = gumbo_get_attribute(&element.attributes, "v-for");
        if (vfor) {
            return ProcessVForElement(node, vfor->value, state);
        }

        // Regular element - serialize it
        return SerializeElement(node, state);
    }

    if (node->type == GUMBO_NODE_DOCUMENT) {
        // For document node, just process children
        std::ostringstream oss;
        GumboVector* children = &node->v.document.children;
        for (unsigned int i = 0; i < children->length; i++) {
            GumboNode* child = static_cast<GumboNode*>(children->data[i]);
            oss << ProcessNode(child, state);
        }
        return oss.str();
    }

    return "";
}

std::string TemplateParser::SerializeElement(GumboNode* node, LuaUIState& state) {
    if (!node || node->type != GUMBO_NODE_ELEMENT) return "";

    GumboElement& element = node->v.element;
    std::ostringstream oss;

    // Opening tag
    oss << "<" << gumbo_normalized_tagname(element.tag);

    // Attributes (skip v-if and v-for)
    for (unsigned int i = 0; i < element.attributes.length; i++) {
        GumboAttribute* attr = static_cast<GumboAttribute*>(element.attributes.data[i]);

        // Skip directive attributes
        if (std::string(attr->name) == "v-if" || std::string(attr->name) == "v-for") {
            continue;
        }

        oss << " " << attr->name;
        if (attr->value && strlen(attr->value) > 0) {
            oss << "=\"" << attr->value << "\"";
        }
    }

    oss << ">";

    // Children
    GumboVector* children = &element.children;
    for (unsigned int i = 0; i < children->length; i++) {
        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
        oss << ProcessNode(child, state);
    }

    // Closing tag (only for non-void elements)
    if (element.tag != GUMBO_TAG_BR && element.tag != GUMBO_TAG_IMG &&
        element.tag != GUMBO_TAG_INPUT && element.tag != GUMBO_TAG_META &&
        element.tag != GUMBO_TAG_LINK) {
        oss << "</" << gumbo_normalized_tagname(element.tag) << ">";
    }

    return oss.str();
}

std::string TemplateParser::ProcessVForElement(GumboNode* node, const std::string& expression, LuaUIState& state) {
    // Parse v-for expression: "item in collection"
    std::regex forExprRegex(R"(\s*(\w+)\s+in\s+(.+)\s*)");
    std::smatch match;

    if (!std::regex_match(expression, match, forExprRegex)) {
        std::cerr << "[TemplateParser] Invalid v-for expression: " << expression << std::endl;
        return "";
    }

    std::string itemVar = match[1].str();
    std::string collectionExpr = match[2].str();

    // Get the collection from Lua state
    sol::object collection = state.GetValue(collectionExpr);

    if (!collection.is<sol::table>()) {
        std::cerr << "[TemplateParser] v-for collection not found or not a table: " << collectionExpr << std::endl;
        return "";
    }

    sol::table collectionTable = collection.as<sol::table>();
    std::ostringstream result;

    // Iterate over collection
    for (auto& pair : collectionTable) {
        sol::object item = pair.second;

        // Clone the element for this iteration
        // For now, we'll serialize and re-process with item context
        std::string elementHTML = SerializeElementForIteration(node, state, itemVar, item);
        result << elementHTML;
    }

    return result.str();
}

std::string TemplateParser::SerializeElementForIteration(GumboNode* node, LuaUIState& state,
                                                          const std::string& itemVar, sol::object& item) {
    if (!node || node->type != GUMBO_NODE_ELEMENT) return "";

    GumboElement& element = node->v.element;
    std::ostringstream oss;

    // Opening tag
    oss << "<" << gumbo_normalized_tagname(element.tag);

    // Attributes (skip v-for)
    for (unsigned int i = 0; i < element.attributes.length; i++) {
        GumboAttribute* attr = static_cast<GumboAttribute*>(element.attributes.data[i]);

        if (std::string(attr->name) == "v-for") {
            continue;
        }

        oss << " " << attr->name;
        if (attr->value && strlen(attr->value) > 0) {
            oss << "=\"" << attr->value << "\"";
        }
    }

    oss << ">";

    // Process children with item context
    GumboVector* children = &element.children;
    for (unsigned int i = 0; i < children->length; i++) {
        GumboNode* child = static_cast<GumboNode*>(children->data[i]);

        if (child->type == GUMBO_NODE_TEXT || child->type == GUMBO_NODE_WHITESPACE) {
            std::string text(child->v.text.text);
            // Replace {{itemVar.property}} with values from item
            std::string processed = ProcessIterationInterpolations(text, itemVar, item);
            oss << processed;
        } else {
            // Recursively process child elements
            oss << SerializeElementForIteration(child, state, itemVar, item);
        }
    }

    // Closing tag
    if (element.tag != GUMBO_TAG_BR && element.tag != GUMBO_TAG_IMG &&
        element.tag != GUMBO_TAG_INPUT && element.tag != GUMBO_TAG_META &&
        element.tag != GUMBO_TAG_LINK) {
        oss << "</" << gumbo_normalized_tagname(element.tag) << ">";
    }

    return oss.str();
}

std::string TemplateParser::ProcessIterationInterpolations(const std::string& text,
                                                            const std::string& itemVar,
                                                            sol::object& item) {
    std::string result = text;

    // Find {{itemVar.property}} patterns
    std::regex itemRefRegex(R"(\{\{\s*)" + itemVar + R"(\.(\w+)\s*\}\})");
    std::smatch match;
    std::string searchStr = result;
    std::ostringstream output;

    while (std::regex_search(searchStr, match, itemRefRegex)) {
        output << searchStr.substr(0, match.position());

        std::string property = match[1].str();

        // Get value from item
        std::string valueStr = "";
        if (item.is<sol::table>()) {
            sol::table itemTable = item.as<sol::table>();
            sol::object value = itemTable[property];

            if (value.is<std::string>()) {
                valueStr = value.as<std::string>();
            } else if (value.is<int>()) {
                valueStr = std::to_string(value.as<int>());
            } else if (value.is<double>()) {
                valueStr = std::to_string(value.as<double>());
            }
        }

        output << valueStr;
        searchStr = match.suffix();
    }

    output << searchStr;
    return output.str();
}

std::string TemplateParser::ProcessInterpolations(const std::string& text, LuaUIState& state) {
    std::string result;
    std::regex interpolationRegex(R"(\{\{\s*(.+?)\s*\}\})");

    std::smatch match;
    std::string searchStr = text;

    while (std::regex_search(searchStr, match, interpolationRegex)) {
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

// Unused helper methods (kept for potential future use)

std::string TemplateParser::ProcessVFor(const std::string& html, LuaUIState& state) {
    // Deprecated - now using gumbo-based approach
    return html;
}

std::string TemplateParser::ProcessVIf(const std::string& html, LuaUIState& state) {
    // Deprecated - now using gumbo-based approach
    return html;
}

std::string TemplateParser::ExtractTagName(const std::string& tag) {
    std::regex tagNameRegex(R"(<\s*(\w+))");
    std::smatch match;
    if (std::regex_search(tag, match, tagNameRegex)) {
        return match[1].str();
    }
    return "";
}

std::string TemplateParser::GetAttribute(const std::string& tag, const std::string& attrName) {
    std::regex attrRegex(attrName + R"(\s*=\s*['"]([^'"]*)['"])");
    std::smatch match;
    if (std::regex_search(tag, match, attrRegex)) {
        return match[1].str();
    }
    return "";
}

std::string TemplateParser::RemoveAttribute(const std::string& tag, const std::string& attrName) {
    std::regex attrRegex(R"(\s+)" + attrName + R"(\s*=\s*['"][^'"]*['"])");
    return std::regex_replace(tag, attrRegex, "");
}

size_t TemplateParser::FindClosingTag(const std::string& html, const std::string& tagName, size_t startPos) {
    int depth = 1;
    size_t pos = startPos;

    std::string openTag = "<" + tagName;
    std::string closeTag = "</" + tagName + ">";

    while (depth > 0 && pos < html.length()) {
        size_t nextOpen = html.find(openTag, pos);
        size_t nextClose = html.find(closeTag, pos);

        if (nextClose == std::string::npos) {
            return std::string::npos;
        }

        if (nextOpen != std::string::npos && nextOpen < nextClose) {
            depth++;
            pos = nextOpen + openTag.length();
        } else {
            depth--;
            if (depth == 0) {
                return nextClose;
            }
            pos = nextClose + closeTag.length();
        }
    }

    return std::string::npos;
}
