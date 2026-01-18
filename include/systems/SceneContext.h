/**
 * @file SceneContext.h
 * @brief Dependency injection context for scene scripts
 *
 * SceneContext provides explicit access to engine systems and
 * role-provided modules for Lua scripts. Capabilities are only
 * present if declared in scene YAML.
 *
 * @see docs/architecture/SCENE_CONTRACTS.md
 */

#pragma once

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>

// Forward declarations
class Camera;
class Registry;
class HTMLRendererMT;
class WindowManager;
class ReactiveUI;

/**
 * @brief Dependency injection context for scene scripts
 *
 * SceneContext provides explicit access to:
 * - Engine systems (always present): ctx.engine
 * - Scene information (always present): ctx.scene
 * - Optional capabilities: ctx.ui, ctx.input, ctx.audio, ctx.world
 * - Role-provided modules: ctx.constants, ctx.game, etc.
 *
 * Capabilities are only populated if:
 * 1. Scene YAML declares them in `capabilities:` section
 * 2. Engine actually supports them
 *
 * Scripts access context in Lua:
 * @code
 * function Module:init(ctx)
 *     -- Always available
 *     local delta = ctx.engine.getDelta()
 *     local sceneName = ctx.scene.name
 *
 *     -- Optional - check for nil
 *     if ctx.ui then
 *         ctx.ui.setValue("score", 0)
 *     end
 *
 *     -- From other modules via role
 *     local gridWidth = ctx.constants.GRID_WIDTH
 * end
 * @endcode
 */
class SceneContext
{
public:
    /**
     * @brief Create context for a scene
     * @param sceneName Name of the scene being loaded
     * @param capabilities Set of declared capabilities from YAML
     */
    explicit SceneContext(const std::string &sceneName,
                          const std::unordered_set<std::string> &capabilities);

    /**
     * @brief Get the Lua table representation of this context
     * @param lua Lua state to create table in
     * @return sol::table that can be passed to init(ctx)
     */
    sol::table ToLuaTable(sol::state &lua);

    /**
     * @brief Update the context table with a newly registered role
     * @param lua Lua state
     * @param ctxTable Existing context table to update
     * @param role Role name
     * @param value Value to add at ctx[role]
     */
    void UpdateContextWithRole(sol::state &lua, sol::table &ctxTable,
                               const std::string &role, sol::object value);

    /**
     * @brief Register a module's exported value under its role
     * @param role Role name (e.g., "constants", "game")
     * @param value Lua value to expose at ctx.{role}
     */
    void RegisterRole(const std::string &role, sol::object value);

    /**
     * @brief Check if a capability is enabled
     * @param capability Capability name (e.g., "ui", "input")
     * @return true if capability was declared and available
     */
    bool HasCapability(const std::string &capability) const;

    /**
     * @brief Get list of missing capabilities from required set
     * @param required Set of capability names
     * @return Vector of missing capability names
     */
    std::vector<std::string> GetMissingCapabilities(
        const std::vector<std::string> &required) const;

    /**
     * @brief Get scene name
     */
    const std::string &GetSceneName() const { return m_sceneName; }

    /**
     * @brief Check if strict mode is enabled
     */
    bool IsStrictMode() const { return m_strictMode; }

    /**
     * @brief Set strict mode (errors instead of warnings)
     */
    void SetStrictMode(bool strict) { m_strictMode = strict; }

private:
    std::string m_sceneName;
    std::unordered_set<std::string> m_capabilities;
    std::unordered_map<std::string, sol::object> m_roleValues;
    bool m_strictMode = false;

    /**
     * @brief Build ctx.engine subtable
     */
    sol::table BuildEngineTable(sol::state &lua);

    /**
     * @brief Build ctx.scene subtable
     */
    sol::table BuildSceneTable(sol::state &lua);

    /**
     * @brief Build capability subtable (ui, input, etc.)
     * @param lua Lua state
     * @param capability Capability name
     * @return Table with capability API, or nil object if not enabled
     */
    sol::object BuildCapabilityTable(sol::state &lua, const std::string &capability);
};
