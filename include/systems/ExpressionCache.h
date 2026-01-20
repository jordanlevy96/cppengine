/**
 * @file ExpressionCache.h
 * @brief Compiled Lua expression cache for fast repeated evaluation
 *
 * Part of the incremental UI update architecture (Phase 1).
 * Eliminates repeated Lua compilation overhead by caching compiled functions.
 */

#pragma once

#include <sol/sol.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

/**
 * @brief Cache for precompiled Lua expressions
 *
 * Expressions like "data.score" or "data.fps > 30" are compiled once
 * and reused for subsequent evaluations. This eliminates the ~1-2ms
 * overhead of m_lua->load() on every expression evaluation.
 *
 * **Usage:**
 * @code
 * ExpressionCache cache;
 * cache.Initialize(luaState);
 *
 * // First call compiles and caches
 * uint32_t id = cache.GetOrCompile("data.score");
 *
 * // Subsequent evaluations use cached function
 * std::string value = cache.EvaluateAsString(id, stateTable);
 * bool condition = cache.EvaluateAsBool(id, stateTable);
 * @endcode
 *
 * @note Not thread-safe - all operations must be on main thread
 */
class ExpressionCache
{
public:
    /**
     * @brief Compiled expression entry
     */
    struct CompiledExpression
    {
        uint32_t id;                        ///< Unique identifier
        std::string source;                 ///< Original expression text
        sol::protected_function compiled;   ///< Precompiled Lua function
        std::set<std::string> dependencies; ///< State paths this expression reads (for Phase 2)
    };

    ExpressionCache() = default;
    ~ExpressionCache() = default;

    /**
     * @brief Initialize cache with Lua state reference
     * @param lua Pointer to sol::state (from ScriptManager)
     * @note Must be called before any compile/evaluate operations
     */
    void Initialize(sol::state* lua);

    /**
     * @brief Get or compile an expression, returning its cache ID
     * @param expression Lua expression string (e.g., "data.score", "data.fps > 30")
     * @return Cache ID for subsequent evaluation calls
     * @note First call compiles; subsequent calls return cached ID
     */
    uint32_t GetOrCompile(const std::string& expression);

    /**
     * @brief Evaluate cached expression and return result as sol::object
     * @param exprId Cache ID from GetOrCompile()
     * @param env Environment table (state table) for expression evaluation
     * @return Evaluation result as sol::object
     * @note Returns sol::nil on error
     */
    sol::object Evaluate(uint32_t exprId, sol::table& env);

    /**
     * @brief Evaluate cached expression and return result as string
     * @param exprId Cache ID from GetOrCompile()
     * @param env Environment table for expression evaluation
     * @return String representation of result
     * @note Returns empty string on error or nil result
     */
    std::string EvaluateAsString(uint32_t exprId, sol::table& env);

    /**
     * @brief Evaluate cached expression and return result as boolean
     * @param exprId Cache ID from GetOrCompile()
     * @param env Environment table for expression evaluation
     * @return Boolean result (Lua truthiness rules)
     * @note Returns false on error
     */
    bool EvaluateAsBool(uint32_t exprId, sol::table& env);

    /**
     * @brief Get dependencies for an expression (for Phase 2 dependency tracking)
     * @param exprId Cache ID
     * @return Set of state paths this expression reads
     */
    const std::set<std::string>& GetDependencies(uint32_t exprId) const;

    /**
     * @brief Get cache statistics for debugging/profiling
     * @return Pair of (cache hits, cache misses)
     */
    std::pair<uint64_t, uint64_t> GetStats() const
    {
        return {m_cacheHits, m_cacheMisses};
    }

    /**
     * @brief Clear all cached expressions
     * @note Use when template changes require full recompilation
     */
    void Clear();

    /**
     * @brief Check if cache is initialized
     * @return true if Initialize() has been called
     */
    bool IsInitialized() const { return m_lua != nullptr; }

    /**
     * @brief Get number of cached expressions
     * @return Number of compiled expressions in cache
     */
    size_t Size() const { return m_expressions.size(); }

private:
    /**
     * @brief Compile a Lua expression into a callable function
     * @param expression Expression string
     * @return Compiled function, or invalid function on error
     */
    sol::protected_function CompileExpression(const std::string& expression);

    /**
     * @brief Analyze expression to extract state path dependencies
     * @param expression Expression string
     * @return Set of state paths referenced by expression
     * @note Uses simple regex matching; may have false positives
     */
    std::set<std::string> AnalyzeDependencies(const std::string& expression);

    /**
     * @brief Log cache statistics periodically
     * @note Called internally every 100 operations
     */
    void LogStatsIfNeeded();

    sol::state* m_lua = nullptr;                                    ///< Lua state reference
    std::vector<CompiledExpression> m_expressions;                  ///< Compiled expressions by ID
    std::unordered_map<std::string, uint32_t> m_sourceToId;         ///< Expression source → ID lookup
    static const std::set<std::string> s_emptyDeps;                 ///< Empty set for invalid lookups

    // Statistics
    mutable uint64_t m_cacheHits = 0;
    mutable uint64_t m_cacheMisses = 0;
};
