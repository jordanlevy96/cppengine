/**
 * @file TemplateParser.h
 * @brief HTML template parser with Vue-style directive support
 * @lines ~342
 *
 * Quick-stats (Public API):
 * - Parse() - Load HTML template with directives (line 98)
 * - Evaluate() - Render template using Lua state (line 107)
 * - GetEventHandlers() - Retrieve @click/@keydown handlers (line 121)
 * - GetPerfStats() - Performance metrics (line ~195)
 * - ResetPerfStats() - Clear statistics (line ~205)
 *
 * Directives supported: v-if, v-for, {{ }}, @click, :class
 * Performance: ~3.2ms avg (Tetris), 10ms budget
 * Implementation: See src/systems/TemplateParser.cpp
 */

#pragma once

#include "systems/LuaUIState.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <climits>

// Forward declare gumbo types
struct GumboInternalNode;
typedef struct GumboInternalNode GumboNode;
struct GumboInternalOutput;
typedef struct GumboInternalOutput GumboOutput;

/**
 * @brief HTML5 template parser with Vue.js-inspired directives
 *
 * Parses HTML templates and evaluates declarative directives using Lua state.
 * Uses Gumbo HTML5 parser for robust DOM processing.
 *
 * **Supported directives:**
 * - `v-if="condition"` - Conditional rendering (boolean expressions)
 * - `v-for="item in collection"` - List iteration (array/table loops)
 * - `{{ expression }}` - Text interpolation (Lua expressions)
 *
 * **Implementation:**
 * - Parse() builds DOM tree using Gumbo parser
 * - Evaluate() walks DOM, processes directives depth-first
 * - Supports nested directives (v-for inside v-if, etc.)
 * - Handles scoped variables in v-for iterations
 *
 * **Usage example:**
 * @code
 * TemplateParser parser;
 * LuaUIState state;
 * state.LoadStateFile("../res/ui/state/game.lua");
 *
 * std::string html = R"(
 *   <div v-if="showPlayers">
 *     <div v-for="player in players" class="player">
 *       {{player.name}}: {{player.score}} points
 *     </div>
 *   </div>
 * )";
 *
 * parser.Parse(html);
 * std::string rendered = parser.Evaluate(state);
 * @endcode
 *
 * @note Thread-safe for parsing different templates, not thread-safe for concurrent Evaluate()
 * @see docs/architecture/UI_SYSTEM.md for directive syntax reference
 */
class TemplateParser {
public:
    /**
     * @brief Directive types supported by parser
     */
    enum class DirectiveType {
        TEXT_INTERPOLATION,  ///< {{ expression }} - Lua expression in text
        V_IF,                ///< v-if="condition" - Conditional rendering
        V_FOR,               ///< v-for="item in collection" - List iteration
        NONE                 ///< Regular HTML content (no directive)
    };

    /**
     * @brief Parsed directive node (legacy, kept for compatibility)
     * @note Currently unused - Gumbo parser processes directives directly
     */
    struct DirectiveNode {
        DirectiveType type;      ///< Directive type
        std::string expression;  ///< Directive expression (e.g., "data.showDebug")
        std::string content;     ///< Inner HTML content
        std::string tag;         ///< HTML tag name (e.g., "div")
        std::string attributes;  ///< Other attributes (non-directive)
        int startPos;            ///< Position in original template
        int endPos;              ///< End position in original template

        DirectiveNode()
            : type(DirectiveType::NONE), startPos(0), endPos(0) {}
    };

    TemplateParser();
    ~TemplateParser();

    /**
     * @brief Parse HTML template and build DOM tree
     * @param html Raw HTML string with directives
     * @note Stores template internally for Evaluate() calls
     * @note Safe to call multiple times to change template
     */
    void Parse(const std::string& html);

    /**
     * @brief Evaluate all directives using Lua state
     * @param state LuaUIState containing data for expressions
     * @return Rendered HTML string with directives evaluated
     * @note Must call Parse() first
     * @note Walks DOM tree depth-first, processing directives
     */
    std::string Evaluate(LuaUIState& state);

    /**
     * @brief Get parsed directives (legacy compatibility)
     * @return Vector of DirectiveNode structs
     * @note Currently returns empty vector - Gumbo processes directives directly
     */
    const std::vector<DirectiveNode>& GetParsedDirectives() const { return m_directives; }

    /**
     * @brief Get event handlers extracted from @event directives
     * @return Map of element ID → {eventType → handlerExpression}
     * @note Populated during Evaluate() when @click, @mouseover, etc. are found
     */
    const std::map<std::string, std::map<std::string, std::string>>& GetEventHandlers() const {
        return m_eventHandlers;
    }

    /**
     * @brief Reset event handlers and ID counter
     * @note Call when re-parsing template to clear old handlers
     */
    void ResetEventHandlers() {
        m_eventHandlers.clear();
        m_nextEventId = 0;
    }

private:
    // === Gumbo-based DOM processing ===

    /**
     * @brief Process Gumbo DOM node recursively
     * @param node Gumbo node to process
     * @param state Lua state for directive evaluation
     * @return Rendered HTML for this node and children
     * @note Handles element, text, and comment nodes
     */
    std::string ProcessNode(GumboNode* node, LuaUIState& state);

    /**
     * @brief Serialize HTML element with directive processing
     * @param node Gumbo element node
     * @param state Lua state for evaluation
     * @return Rendered HTML element
     * @note Checks for v-if, v-for directives before serialization
     */
    std::string SerializeElement(GumboNode* node, LuaUIState& state);

    /**
     * @brief Process v-for directive on element
     * @param node Gumbo element with v-for attribute
     * @param expression v-for expression (e.g., "item in items")
     * @param state Lua state containing collection
     * @return Concatenated HTML for all iterations
     * @note Creates scoped variables for each iteration
     */
    std::string ProcessVForElement(GumboNode* node, const std::string& expression, LuaUIState& state);

    /**
     * @brief Serialize element with scoped iteration variables
     * @param node Gumbo element node
     * @param state Lua state
     * @param itemVar Variable name (e.g., "player")
     * @param item Current iteration value
     * @return Rendered HTML for this iteration
     * @note Replaces {{itemVar.field}} with actual values
     */
    std::string SerializeElementForIteration(GumboNode* node, LuaUIState& state,
                                             const std::string& itemVar, sol::object& item);

    /**
     * @brief Process text interpolations scoped to iteration
     * @param text Text content with {{}} markers
     * @param itemVar Iteration variable name
     * @param item Current iteration object
     * @return Text with interpolations replaced
     * @note Only replaces {{itemVar.*}} patterns, leaves global {{}} alone
     */
    std::string ProcessIterationInterpolations(const std::string& text,
                                                const std::string& itemVar,
                                                sol::object& item);

    /**
     * @brief Process :class / v-bind:class directive
     * @param expr Class binding expression (object syntax or Lua expression)
     * @param state Lua state for evaluation
     * @return Space-separated class names to apply
     * @note Supports object syntax: {active: condition, disabled: !enabled}
     * @note Supports Lua expressions: 'base' .. (cond and ' active' or '')
     */
    std::string ProcessBindClass(const std::string& expr, LuaUIState& state);

    // === Deprecated regex-based methods (kept for fallback) ===

    /**
     * @brief Process v-for directives using regex (deprecated)
     * @param html HTML string
     * @param state Lua state
     * @return HTML with v-for expanded
     * @note Legacy implementation, Gumbo method preferred
     */
    std::string ProcessVFor(const std::string& html, LuaUIState& state);

    /**
     * @brief Process v-if directives using regex (deprecated)
     * @param html HTML string
     * @param state Lua state
     * @return HTML with v-if evaluated
     * @note Legacy implementation, Gumbo method preferred
     */
    std::string ProcessVIf(const std::string& html, LuaUIState& state);

    // === Text interpolation helpers ===

    /**
     * @brief Process {{ expression }} interpolations in text
     * @param text Text content with interpolation markers
     * @param state Lua state for evaluation
     * @return Text with {{}} replaced by Lua expression results
     * @note Evaluates expressions using LuaUIState::EvaluateAsString()
     */
    std::string ProcessInterpolations(const std::string& text, LuaUIState& state);

    // === HTML parsing utilities ===

    /**
     * @brief Find matching closing tag for opening tag
     * @param html HTML string
     * @param tagName Tag name to match (e.g., "div")
     * @param startPos Position after opening tag
     * @return Position of closing tag start, or std::string::npos if not found
     * @note Handles nested tags of same name (counts depth)
     */
    size_t FindClosingTag(const std::string& html, const std::string& tagName, size_t startPos);

    /**
     * @brief Extract tag name from opening tag
     * @param tag Opening tag string (e.g., "<div class='foo'>")
     * @return Tag name (e.g., "div")
     */
    std::string ExtractTagName(const std::string& tag);

    /**
     * @brief Get attribute value from HTML tag
     * @param tag HTML tag string
     * @param attrName Attribute name (e.g., "v-if")
     * @return Attribute value, or empty string if not found
     * @note Example: GetAttribute("<div v-if='show'>", "v-if") → "show"
     */
    std::string GetAttribute(const std::string& tag, const std::string& attrName);

    /**
     * @brief Remove attribute from HTML tag
     * @param tag HTML tag string
     * @param attrName Attribute name to remove
     * @return Tag with attribute removed
     * @note Example: RemoveAttribute("<div v-if='x' class='y'>", "v-if") → "<div class='y'>"
     */
    std::string RemoveAttribute(const std::string& tag, const std::string& attrName);

    std::string m_template;                  ///< Original HTML template
    std::vector<DirectiveNode> m_directives; ///< Parsed directives (legacy, unused)

    // === Template parse caching (Phase 1) ===

    GumboOutput* m_cachedGumboOutput = nullptr;  ///< Cached parsed DOM tree
    std::string m_cachedTemplateHash;            ///< Hash of template for change detection
    std::string m_cachedHTML;                    ///< Last rendered HTML output
    bool m_gumboOwned = false;                   ///< Whether we own m_cachedGumboOutput

    /**
     * @brief Simple hash function for template change detection
     * @param str Template string
     * @return Hash string
     */
    static std::string HashTemplate(const std::string& str);

    /**
     * @brief Free cached Gumbo output if owned
     */
    void FreeCachedGumbo();

    // === Event handling support ===

    /**
     * @brief Event handlers extracted from @event directives
     * @note Map structure: elementId → {eventType → handlerExpression}
     * @note Example: {"event_0": {"click": "onStart", "mouseover": "onHover"}}
     */
    std::map<std::string, std::map<std::string, std::string>> m_eventHandlers;

    /**
     * @brief Counter for generating unique element IDs
     * @note Increments for each element with @event directives
     * @note Reset to 0 on ResetEventHandlers()
     */
    uint32_t m_nextEventId = 0;

    // === Performance metrics ===

    uint64_t m_totalEvaluations = 0;                        ///< Total number of Evaluate() calls
    int64_t m_totalEvaluationTimeUs = 0;                    ///< Cumulative evaluation time in microseconds
    int64_t m_maxEvaluationTimeUs = 0;                      ///< Slowest evaluation in microseconds
    int64_t m_minEvaluationTimeUs = INT64_MAX;              ///< Fastest evaluation in microseconds

public:
    /**
     * @brief Get performance statistics
     * @return Struct with evaluation counts and timings
     */
    struct PerfStats {
        uint64_t totalEvaluations;
        int64_t avgTimeUs;
        int64_t minTimeUs;
        int64_t maxTimeUs;
    };

    PerfStats GetPerfStats() const {
        return {
            m_totalEvaluations,
            m_totalEvaluations > 0 ? m_totalEvaluationTimeUs / static_cast<int64_t>(m_totalEvaluations) : 0,
            m_minEvaluationTimeUs == INT64_MAX ? 0 : m_minEvaluationTimeUs,
            m_maxEvaluationTimeUs
        };
    }

    /**
     * @brief Reset performance statistics
     */
    void ResetPerfStats() {
        m_totalEvaluations = 0;
        m_totalEvaluationTimeUs = 0;
        m_maxEvaluationTimeUs = 0;
        m_minEvaluationTimeUs = INT64_MAX;
    }
};
