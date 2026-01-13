/**
 * @file Lighting.h
 * @brief Lighting component for marking entities as light sources
 */

#pragma once

#include "components/RenderComponent.h"
#include "components/Transform.h"

/**
 * @brief Component marking an entity as a light source
 *
 * Associates an entity with lighting data. The entity's Transform
 * defines the light's position/direction, while this component
 * links to the light's parameters (color, intensity, type, etc.).
 *
 * **Light Types (implementation-dependent):**
 * - Point lights: Omnidirectional from a position
 * - Directional lights: Parallel rays (e.g., sun)
 * - Spot lights: Cone-shaped illumination
 *
 * **Typical Setup:**
 * @code
 * // Create light entity with position
 * EntityID lightEntity = registry.RegisterEntity("sun_light");
 * auto& transform = registry.GetComponent<Transform>(lightEntity);
 * transform.Pos = glm::vec3(0.0f, 10.0f, 0.0f);
 * transform.Color = glm::vec3(1.0f, 0.9f, 0.7f);  // Warm white
 *
 * // Attach lighting component
 * Lighting lightComp(lightEntity);
 * registry.RegisterComponent(lightEntity, lightComp);
 * @endcode
 *
 * **Current Implementation:**
 * - LightID typically references self (the entity's own ID)
 * - Light parameters come from Transform.Color and Transform.Pos
 * - Advanced properties (attenuation, shadows) not yet implemented
 *
 * **Planned Features (TODO):**
 * - Light intensity parameter
 * - Attenuation coefficients (constant, linear, quadratic)
 * - Shadow casting flags
 * - Light type enum (point/directional/spot)
 * - Spotlight cone angle/cutoff
 *
 * @note Entities with Lighting component should NOT have RenderComponent
 * @note Light visibility/culling is handled by RenderSystem
 * @see Transform for light position and base color
 * @see RenderSystem for lighting calculations
 */
struct Lighting
{
    EntityID LightID;  ///< Associated light entity ID (typically self-referential)

    /**
     * @brief Construct lighting component
     * @param lightID Entity ID of the light (usually the entity this component is attached to)
     * @note Light properties are read from the entity's Transform component
     */
    Lighting(EntityID lightID) : LightID(lightID) {}
};
