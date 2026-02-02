/**
 * @file ExpressionCache.cpp
 * @brief Lua expression compilation cache for UI template system
 * @lines ~245
 *
 * Purpose: Eliminate repeated Lua compilation overhead (~1-2ms per expression)
 * by caching compiled functions and reusing them for subsequent evaluations.
 *
 * Key functions:
 * - GetOrCompile() - Cache lookup or compile new expression (line 14, ~45 lines)
 * - CompileExpression() - Wrap expression in Lua function (line 79, ~55 lines)
 * - Evaluate() - Execute cached function, return sol::object (line 136, ~25 lines)
 * - EvaluateAsString() - Execute and convert to string (line 163, ~35 lines)
 * - EvaluateAsBool() - Execute and convert to bool (line 200, ~35 lines)
 * - LogStatsIfNeeded() - Periodic cache statistics (line 59, ~20 lines)
 *
 * Performance:
 * - Hit rate: 99.8% (12 expressions, 6288+ evaluations in Tetris)
 * - Logs stats every 100 operations
 * - Warns if hit rate drops below 80% after warm-up
 *
 * Integration: Used by LuaUIState for all expression evaluation
 */

#include "systems/ExpressionCache.h"
#include "util/Logger.h"
#include <regex>

// Static empty set for invalid lookups
const std::set<std::string> ExpressionCache::s_emptyDeps;

void ExpressionCache::Initialize(sol::state* lua)
{
    m_lua = lua;
    LOG_INFO("[ExpressionCache] Initialized");
}

uint32_t ExpressionCache::GetOrCompile(const std::string& expression)
{
    if (!m_lua)
    {
        LOG_ERROR("[ExpressionCache] Not initialized");
        return UINT32_MAX;
    }

    // Check if already compiled
    auto it = m_sourceToId.find(expression);
    if (it != m_sourceToId.end())
    {
        m_cacheHits++;
        LogStatsIfNeeded();
        return it->second;
    }

    // Cache miss - compile new expression
    m_cacheMisses++;

    sol::protected_function compiled = CompileExpression(expression);
    if (!compiled.valid())
    {
        LOG_ERROR("[ExpressionCache] Failed to compile expression: {}", expression);
        return UINT32_MAX;
    }

    // Store in cache
    uint32_t id = static_cast<uint32_t>(m_expressions.size());
    CompiledExpression entry;
    entry.id = id;
    entry.source = expression;
    entry.compiled = compiled;
    entry.dependencies = AnalyzeDependencies(expression);

    m_expressions.push_back(std::move(entry));
    m_sourceToId[expression] = id;

    LOG_DEBUG("[ExpressionCache] Compiled expression #{}: '{}' (deps: {})",
              id, expression, entry.dependencies.size());

    LogStatsIfNeeded();
    return id;
}

void ExpressionCache::LogStatsIfNeeded()
{
    uint64_t totalOps = m_cacheHits + m_cacheMisses;

    // Log periodically (debug-only): avoid spamming logs on hot paths.
    if (totalOps > 0 && totalOps % 1000 == 0)
    {
        float hitRate = totalOps > 0 ? (static_cast<float>(m_cacheHits) / totalOps) * 100.0f : 0.0f;
        LOG_DEBUG("[ExpressionCache] Stats: {} hits, {} misses ({:.1f}% hit rate), {} cached expressions",
                  m_cacheHits, m_cacheMisses, hitRate, m_expressions.size());

        // Warn if hit rate is low after warm-up period
        if (totalOps > 200 && hitRate < 80.0f)
        {
            LOG_WARNING("[ExpressionCache] Low hit rate ({:.1f}%) - possible template changes or expression churn",
                        hitRate);
        }
    }
}

sol::protected_function ExpressionCache::CompileExpression(const std::string& expression)
{
    // Wrap expression in a function that can be called with an environment
    // The function returns the expression result
    std::string luaCode = "return function() return " + expression + " end";

    sol::load_result loadResult = m_lua->load(luaCode);
    if (!loadResult.valid())
    {
        sol::error err = loadResult;
        LOG_ERROR("[ExpressionCache] Load error for '{}': {}", expression, err.what());
        return sol::protected_function();
    }

    // Execute to get the function
    sol::protected_function_result result = loadResult();
    if (!result.valid())
    {
        sol::error err = result;
        LOG_ERROR("[ExpressionCache] Execution error for '{}': {}", expression, err.what());
        return sol::protected_function();
    }

    sol::object obj = result.get<sol::object>();
    if (!obj.valid() || obj.get_type() != sol::type::function)
    {
        LOG_ERROR("[ExpressionCache] Execution result for '{}' was not a function (type: {})",
                  expression, static_cast<int>(obj.get_type()));
        return sol::protected_function();
    }

    return obj.as<sol::protected_function>();
}

std::set<std::string> ExpressionCache::AnalyzeDependencies(const std::string& expression)
{
    std::set<std::string> deps;

    // Match patterns like: data.score, data.items, computed.totalScore
    // This is a simple regex approach; Phase 2 can use more sophisticated analysis
    std::regex pathRegex(R"(([a-zA-Z_]\w*(?:\.[a-zA-Z_]\w*)+))");
    std::smatch match;
    std::string searchStr = expression;

    while (std::regex_search(searchStr, match, pathRegex))
    {
        std::string path = match[1].str();
        deps.insert(path);

        // Also add parent paths for prefix matching
        // e.g., "data.items.length" -> also add "data.items", "data"
        size_t dotPos = path.find('.');
        while (dotPos != std::string::npos)
        {
            std::string parentPath = path.substr(0, dotPos);
            deps.insert(parentPath);
            dotPos = path.find('.', dotPos + 1);
        }

        searchStr = match.suffix();
    }

    return deps;
}

sol::object ExpressionCache::Evaluate(uint32_t exprId, sol::table& env)
{
    if (exprId >= m_expressions.size())
    {
        LOG_ERROR("[ExpressionCache] Invalid expression ID: {}", exprId);
        return sol::nil;
    }

    const CompiledExpression& expr = m_expressions[exprId];

    // Set the environment for the function
    sol::environment funcEnv(*m_lua, sol::create, env);
    sol::set_environment(funcEnv, expr.compiled);

    // Call the function
    sol::protected_function_result result = expr.compiled();

    if (!result.valid())
    {
        sol::error err = result;
        LOG_ERROR("[ExpressionCache] Evaluation error for '{}': {}", expr.source, err.what());
        return sol::nil;
    }

    return result.get<sol::object>();
}

std::string ExpressionCache::EvaluateAsString(uint32_t exprId, sol::table& env)
{
    sol::object obj = Evaluate(exprId, env);

    if (!obj.valid() || obj.get_type() == sol::type::lua_nil)
    {
        return "";
    }

    if (obj.is<std::string>())
    {
        return obj.as<std::string>();
    }
    else if (obj.is<int>())
    {
        return std::to_string(obj.as<int>());
    }
    else if (obj.is<double>())
    {
        // Format doubles without excessive precision
        double val = obj.as<double>();
        if (val == static_cast<int>(val))
        {
            return std::to_string(static_cast<int>(val));
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", val);
        return buf;
    }
    else if (obj.is<bool>())
    {
        return obj.as<bool>() ? "true" : "false";
    }

    return "<object>";
}

bool ExpressionCache::EvaluateAsBool(uint32_t exprId, sol::table& env)
{
    sol::object obj = Evaluate(exprId, env);

    if (!obj.valid() || obj.get_type() == sol::type::lua_nil)
    {
        return false;
    }

    if (obj.is<bool>())
    {
        return obj.as<bool>();
    }
    else if (obj.is<int>() || obj.is<double>())
    {
        // Lua truthiness: non-zero is true
        return obj.as<double>() != 0.0;
    }
    else if (obj.is<std::string>())
    {
        // Non-empty string is true
        return !obj.as<std::string>().empty();
    }

    // Any other non-nil value is truthy
    return true;
}

const std::set<std::string>& ExpressionCache::GetDependencies(uint32_t exprId) const
{
    if (exprId >= m_expressions.size())
    {
        return s_emptyDeps;
    }
    return m_expressions[exprId].dependencies;
}

void ExpressionCache::Clear()
{
    m_expressions.clear();
    m_sourceToId.clear();
    m_cacheHits = 0;
    m_cacheMisses = 0;
    LOG_INFO("[ExpressionCache] Cache cleared");
}
