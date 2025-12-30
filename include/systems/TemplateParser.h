#pragma once

#include "systems/LuaUIState.h"
#include <string>
#include <vector>
#include <memory>

// Forward declare gumbo types
struct GumboInternalNode;
typedef struct GumboInternalNode GumboNode;

// Parses HTML templates with Vue-style directives using Gumbo HTML5 parser
// Supports: v-if, v-for, {{ interpolation }}
class TemplateParser {
public:
    // Directive node types
    enum class DirectiveType {
        TEXT_INTERPOLATION,  // {{ expression }}
        V_IF,                // v-if="condition"
        V_FOR,               // v-for="item in collection"
        NONE                 // Regular HTML content
    };

    // Represents a parsed directive in the template
    struct DirectiveNode {
        DirectiveType type;
        std::string expression;  // The directive expression
        std::string content;     // Inner HTML content
        std::string tag;         // The HTML tag (e.g., "div")
        std::string attributes;  // Other attributes
        int startPos;            // Position in original template
        int endPos;

        DirectiveNode()
            : type(DirectiveType::NONE), startPos(0), endPos(0) {}
    };

    TemplateParser();
    ~TemplateParser() = default;

    // Parse HTML template and extract directives
    // Builds a list of directive nodes for evaluation
    void Parse(const std::string& html);

    // Evaluate all directives using the provided Lua state
    // Returns the final rendered HTML
    std::string Evaluate(LuaUIState& state);

    // Get the parsed directives (for caching)
    const std::vector<DirectiveNode>& GetParsedDirectives() const { return m_directives; }

private:
    // Gumbo-based DOM processing
    std::string ProcessNode(GumboNode* node, LuaUIState& state);
    std::string SerializeElement(GumboNode* node, LuaUIState& state);
    std::string ProcessVForElement(GumboNode* node, const std::string& expression, LuaUIState& state);
    std::string SerializeElementForIteration(GumboNode* node, LuaUIState& state,
                                             const std::string& itemVar, sol::object& item);
    std::string ProcessIterationInterpolations(const std::string& text,
                                                const std::string& itemVar,
                                                sol::object& item);

    // Deprecated regex-based methods (kept for compatibility)
    std::string ProcessVFor(const std::string& html, LuaUIState& state);
    std::string ProcessVIf(const std::string& html, LuaUIState& state);

    // Parse text interpolations {{ expression }}
    std::string ProcessInterpolations(const std::string& text, LuaUIState& state);

    // Find matching closing tag for an opening tag
    // Handles nested tags of the same type
    size_t FindClosingTag(const std::string& html, const std::string& tagName, size_t startPos);

    // Extract tag name from opening tag
    // E.g., "<div class='foo'>" -> "div"
    std::string ExtractTagName(const std::string& tag);

    // Extract attribute value from tag
    // E.g., GetAttribute("<div v-if='show'>", "v-if") -> "show"
    std::string GetAttribute(const std::string& tag, const std::string& attrName);

    // Remove an attribute from a tag
    // E.g., RemoveAttribute("<div v-if='show' class='foo'>", "v-if") -> "<div class='foo'>"
    std::string RemoveAttribute(const std::string& tag, const std::string& attrName);

    // Original template
    std::string m_template;

    // Parsed directive nodes
    std::vector<DirectiveNode> m_directives;
};
