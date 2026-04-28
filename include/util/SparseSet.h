/**
 * @file SparseSet.h
 * @brief Cache-friendly sparse-set storage for ECS components
 *
 * Generic, header-only data structure used by the Registry to store one
 * SparseSet<T> per component type. Extracted from Registry.h so it can be
 * unit-tested without pulling in the engine's GL/scripting headers.
 *
 * Concept:
 * - `sparse[entity]` → dense index, or `(size_t)-1` if absent.
 * - `dense[index]` holds the contiguous component data (good cache behavior
 *   for systems that iterate every component of a kind).
 * - `entities[index]` is parallel to `dense`, mapping back to the owning
 *   entity (needed for swap-and-pop on remove).
 *
 * Performance:
 * - AddComponent / GetComponent / HasComponent / RemoveComponent: O(1).
 * - The sparse array auto-grows by doubling when an entity id exceeds the
 *   current capacity (initial 100). Growth is amortised; never shrinks.
 *
 * Tests: see tests/EngineUnitTests.cpp::RunSparseSetTests().
 */

#pragma once

#include <cstddef>
#include <vector>

/// Entity unique identifier (size_t index).
typedef size_t EntityID;

/// Null entity ID constant (used for invalid/unselected entities).
#ifndef IMHOTEP_ENTITY_NULL_DEFINED
#define IMHOTEP_ENTITY_NULL_DEFINED
static const EntityID ENTITY_NULL = static_cast<EntityID>(-1);
#endif

/**
 * @brief Cache-friendly sparse set for component storage.
 *
 * @tparam T Component type to store.
 */
template <typename T>
struct SparseSet
{
public:
    ~SparseSet<T>()
    {
        sparse.clear();
        dense.clear();
        entities.clear();
    }

    /**
     * @brief Add component to entity. Auto-grows the sparse array as needed.
     */
    void AddComponent(EntityID entity, T &component)
    {
        while (entity >= maxEntities)
        {
            maxEntities *= 2;
            sparse.resize(maxEntities, static_cast<size_t>(-1));
        }

        dense.push_back(component);
        sparse[entity] = dense.size() - 1;
        entities.push_back(entity);
    }

    /**
     * @brief Get component for entity. No bounds check — call HasComponent first.
     */
    T &GetComponent(EntityID entity)
    {
        return dense[sparse[entity]];
    }

    /**
     * @brief Remove component using swap-and-pop (O(1)).
     */
    void RemoveComponent(EntityID entity)
    {
        if (sparse.size() <= entity || sparse[entity] >= dense.size())
            return;

        size_t indexToRemove = sparse[entity];
        EntityID lastEntity = entities.back();

        dense[indexToRemove] = std::move(dense.back());
        entities[indexToRemove] = lastEntity;

        sparse[lastEntity] = indexToRemove;
        sparse[entity] = static_cast<size_t>(-1);

        dense.pop_back();
        entities.pop_back();
    }

    /**
     * @brief Check if entity has a component stored.
     */
    bool HasComponent(EntityID entity) const
    {
        return entity < sparse.size() &&
               sparse[entity] != static_cast<size_t>(-1) &&
               sparse[entity] < dense.size();
    }

    /**
     * @brief Get all entities with this component (insertion order minus removals).
     */
    std::vector<EntityID> GetEntities() { return entities; }

private:
    size_t maxEntities = 100;

    std::vector<size_t> sparse = std::vector<size_t>(maxEntities, static_cast<size_t>(-1));
    std::vector<T> dense;
    std::vector<size_t> entities;
};
