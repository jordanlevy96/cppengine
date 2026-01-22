/**
 * @file SceneLoader.h
 * @brief Scene loading with contract validation and dependency resolution
 *
 * SceneLoader orchestrates:
 * 1. Script loading with contract extraction
 * 2. Role uniqueness validation
 * 3. Capability requirements validation
 * 4. Dependency resolution via topological sort
 * 5. Module initialization in correct order
 *
 * @see docs/architecture/SCENE_CONTRACTS.md
 */

#pragma once

#include "systems/SceneModule.h"
#include "systems/SceneContext.h"
#include <sol/sol.hpp>
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <queue>
#include <functional>

/**
 * @brief Result of dependency resolution
 */
struct DependencyResolutionResult
{
    bool success = false;                     ///< True if resolution succeeded
    std::vector<SceneModule *> initOrder;     ///< Topologically sorted modules
    std::vector<std::string> errors;          ///< Fatal errors
    std::vector<std::string> warnings;        ///< Non-fatal warnings
    std::vector<SceneModule *> legacyModules; ///< Modules without dependencies (init last)
};

/**
 * @brief Scene loading with contract validation and dependency resolution
 *
 * SceneLoader provides a new scene loading flow that:
 * - Extracts contracts from Lua scripts
 * - Validates role uniqueness and capability requirements
 * - Resolves dependencies using topological sort
 * - Initializes modules in correct order
 * - Maintains backward compatibility with legacy scripts
 *
 * **Usage:**
 * @code
 * SceneLoader loader;
 * if (loader.LoadScripts(scenePath)) {
 *     // Scripts loaded and initialized
 *     // Entity creation can proceed
 * }
 * @endcode
 */
class SceneLoader
{
public:
    /**
     * @brief Load scripts from scene YAML with full contract validation
     * @param scenePath Path to scene YAML file (relative to resource path)
     * @return true if all scripts loaded and validated successfully
     *
     * This method:
     * 1. Parses YAML scene file
     * 2. Extracts capabilities from YAML
     * 3. Creates SceneContext
     * 4. Loads all scripts from `scripts:` section
     * 5. Validates roles and capabilities
     * 6. Resolves dependencies
     * 7. Initializes modules in order
     */
    bool LoadScripts(const std::string &scenePath);

    /**
     * @brief Get loaded modules (for inspection/debugging)
     */
    const std::vector<SceneModule> &GetModules() const { return m_modules; }

    /**
     * @brief Get scene context
     */
    SceneContext *GetContext() { return m_context.get(); }

    /**
     * @brief Get the parsed YAML node (for entity creation by Registry)
     */
    const YAML::Node &GetSceneYAML() const { return m_sceneYAML; }

    /**
     * @brief Check if scene was loaded successfully
     */
    bool IsLoaded() const { return m_isLoaded; }

private:
    std::vector<SceneModule> m_modules;
    std::unique_ptr<SceneContext> m_context;
    std::unordered_map<std::string, SceneModule *> m_roleToModule;
    YAML::Node m_sceneYAML;
    bool m_isLoaded = false;

    /**
     * @brief Load all scripts and extract contracts
     * @param lua Lua state
     * @param scriptPaths Full paths to script files
     * @return true if all scripts loaded (warnings may still exist)
     */
    bool LoadAllScripts(sol::state &lua, const std::vector<std::string> &scriptPaths);

    /**
     * @brief Validate role uniqueness across modules
     * @return Vector of warning messages (duplicates)
     */
    std::vector<std::string> ValidateRoles();

    /**
     * @brief Validate capability requirements for all modules
     * @return Vector of error messages (missing capabilities)
     */
    std::vector<std::string> ValidateCapabilities();

    /**
     * @brief Resolve dependencies and compute initialization order
     * @return Resolution result with sorted order or errors
     */
    DependencyResolutionResult ResolveDependencies();

    /**
     * @brief Initialize modules in dependency order
     * @param lua Lua state
     * @return true if all modules initialized successfully
     */
    bool InitializeModules(sol::state &lua);

    /**
     * @brief Topological sort using Kahn's algorithm
     * @param modules Modules with dependency information
     * @param roleMap Mapping from role name to module
     * @return Sorted order, or empty if cycle detected
     */
    static std::vector<SceneModule *> TopologicalSort(
        std::vector<SceneModule> &modules,
        const std::unordered_map<std::string, SceneModule *> &roleMap);

    /**
     * @brief Detect cycles in dependency graph using DFS
     * @param modules All modules
     * @param roleMap Role to module mapping
     * @return Vector of cycle descriptions (empty if no cycles)
     */
    static std::vector<std::string> DetectCycles(
        const std::vector<SceneModule> &modules,
        const std::unordered_map<std::string, SceneModule *> &roleMap);
};
