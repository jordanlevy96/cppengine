/**
 * @file ServiceRegistry.h
 * @brief Registry for high-performance C++ game services
 *
 * Services are game-agnostic heavy lifting systems that:
 * - Own large datasets (collision grids, pathfinding graphs, etc.)
 * - Expose narrow, declarative Lua APIs
 * - Never call back into Lua during execution
 * - Are optional and scene-scoped
 */

#pragma once

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

/**
 * @brief Base interface for all engine services
 *
 * Services are:
 * - Game-agnostic heavy lifting systems
 * - Own large datasets (collision grids, pathfinding graphs, etc.)
 * - Expose narrow, declarative Lua APIs
 * - Never call back into Lua during execution
 * - Optional and scene-scoped
 * - Testable in headless mode (no UI required)
 *
 * **Lifecycle:**
 * 1. Service is created via ServiceRegistry::Create()
 * 2. Initialize() is called to allocate resources
 * 3. RegisterLuaBindings() exposes Lua API
 * 4. Service is used during scene execution
 * 5. Shutdown() is called when scene unloads
 *
 * **Example:**
 * @code
 * class GridService : public IEngineService {
 * public:
 *     bool Initialize() override {
 *         m_grid.resize(width * height);
 *         return true;
 *     }
 *
 *     void RegisterLuaBindings(sol::state& lua, const std::string& name) override {
 *         sol::table svc = lua.create_table();
 *         svc["setCell"] = [this](int x, int y, int val) { SetCell(x, y, val); };
 *         svc["getCell"] = [this](int x, int y) { return GetCell(x, y); };
 *         lua[name] = svc;
 *     }
 *
 *     // ...
 * };
 * @endcode
 */
class IEngineService
{
public:
    virtual ~IEngineService() = default;

    /**
     * @brief Initialize service resources
     * @return true if initialization succeeded
     * @note Called after construction, before Lua bindings
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Clean up service resources
     * @note Called when scene unloads or engine shuts down
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Register Lua bindings for this service
     * @param lua Lua state to register in
     * @param serviceName Name to expose service under
     * @note Service becomes accessible as lua[serviceName]
     */
    virtual void RegisterLuaBindings(sol::state &lua, const std::string &serviceName) = 0;

    /**
     * @brief Get service type name for logging/debugging
     * @return Human-readable service type name
     */
    virtual const char *GetTypeName() const = 0;
};

/**
 * @brief Registry for engine services
 *
 * Services are registered at engine startup and made available to scenes
 * based on capability declarations. Each service type is registered with
 * a factory function that creates instances on demand.
 *
 * **Usage:**
 * @code
 * // At engine startup
 * ServiceRegistry::GetInstance().Register<GridService>("grid");
 * ServiceRegistry::GetInstance().Register<PathfindingService>("pathfinding");
 *
 * // During scene load (if capability declared)
 * auto service = ServiceRegistry::GetInstance().Create("grid");
 * if (service && service->Initialize()) {
 *     service->RegisterLuaBindings(lua, "grid");
 * }
 * @endcode
 */
class ServiceRegistry
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to ServiceRegistry singleton
     */
    static ServiceRegistry &GetInstance()
    {
        static ServiceRegistry instance;
        return instance;
    }

    ServiceRegistry(ServiceRegistry const &) = delete;
    void operator=(ServiceRegistry const &) = delete;

    /**
     * @brief Register a service type with factory
     * @tparam T Service implementation type (must derive from IEngineService)
     * @param name Service name (used in scene capabilities)
     *
     * @code
     * ServiceRegistry::GetInstance().Register<MyService>("myservice");
     * @endcode
     */
    template <typename T>
    void Register(const std::string &name)
    {
        static_assert(std::is_base_of<IEngineService, T>::value,
                      "T must derive from IEngineService");
        m_factories[name] = []() -> std::unique_ptr<IEngineService>
        {
            return std::make_unique<T>();
        };
    }

    /**
     * @brief Create service instance
     * @param name Service name (as registered)
     * @return Unique pointer to service, or nullptr if unknown
     * @note Caller owns the returned service
     */
    std::unique_ptr<IEngineService> Create(const std::string &name)
    {
        auto it = m_factories.find(name);
        if (it != m_factories.end())
        {
            return it->second();
        }
        return nullptr;
    }

    /**
     * @brief Check if service type is registered
     * @param name Service name
     * @return true if service type is available
     */
    bool HasService(const std::string &name) const
    {
        return m_factories.find(name) != m_factories.end();
    }

    /**
     * @brief Get list of registered service names
     * @return Vector of service names
     */
    std::vector<std::string> GetRegisteredServices() const
    {
        std::vector<std::string> names;
        for (const auto &[name, factory] : m_factories)
        {
            names.push_back(name);
        }
        return names;
    }

private:
    ServiceRegistry() = default;

    /// Factory functions for creating service instances
    std::unordered_map<std::string, std::function<std::unique_ptr<IEngineService>()>> m_factories;
};
