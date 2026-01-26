#shader vertex
#version 330 core
layout (location = 0) in vec2 aGridPos;
layout (location = 1) in float aRingIndex;

// Matrices
uniform mat4 u_view;
uniform mat4 u_projection;

// Camera/world
uniform vec3 u_cameraPos;
uniform vec3 u_worldOrigin;

// Terrain config
uniform float u_heightScale;
uniform float u_heightOffset;  // Vertical offset to push terrain up/down
uniform vec2 u_mapSize;
uniform int u_tileSize;

// Per-ring data (up to 8 rings)
uniform vec2 u_ringOffset[8];
uniform float u_ringTexelSize[8];

// Textures
uniform sampler2DArray u_heightTiles;
uniform isampler2D u_heightSlotMap;

// Fog
uniform float u_fogStart;
uniform float u_fogEnd;

out vec3 v_worldPos;
out vec3 v_normal;
out float v_fogFactor;
out float v_height01;

float SampleHeight(vec2 sampleXZ)
{
    // Wrap X (horizontal) and clamp Z to world bounds.
    sampleXZ.x = mod(sampleXZ.x, u_mapSize.x);
    sampleXZ.y = clamp(sampleXZ.y, 0.0, u_mapSize.y - 1.0);

    ivec2 tileCoord = ivec2(int(floor(sampleXZ.x / float(u_tileSize))),
                            int(floor(sampleXZ.y / float(u_tileSize))));
    int slot = texelFetch(u_heightSlotMap, tileCoord, 0).r;
    if (slot < 0)
    {
        return 0.0;
    }

    vec2 tileUV = fract(sampleXZ / float(u_tileSize));
    return texture(u_heightTiles, vec3(tileUV, float(slot))).r * u_heightScale;
}

void main()
{
    int ring = int(aRingIndex);

    // Compute world position from ring offset and grid position
    vec2 worldXZ = u_ringOffset[ring] + aGridPos * u_ringTexelSize[ring];
    vec2 sampleXZ = worldXZ;

    // Sample height from the streamed tile set (via slot map)
    float height = SampleHeight(sampleXZ);

    // Compute normal via central differences
    float worldTexelSize = u_ringTexelSize[ring];

    float hL = SampleHeight(sampleXZ + vec2(-worldTexelSize, 0.0));
    float hR = SampleHeight(sampleXZ + vec2(+worldTexelSize, 0.0));
    float hD = SampleHeight(sampleXZ + vec2(0.0, -worldTexelSize));
    float hU = SampleHeight(sampleXZ + vec2(0.0, +worldTexelSize));

    // Normal from height differences (Y is up)
    vec3 normal = normalize(vec3(hL - hR, 2.0 * worldTexelSize, hD - hU));

    // Build final position relative to world origin (with vertical offset for layering)
    vec3 localPos = vec3(worldXZ.x, height + u_heightOffset, worldXZ.y) - u_worldOrigin;

    gl_Position = u_projection * u_view * vec4(localPos, 1.0);

    // Pass to fragment shader
    v_worldPos = localPos + u_worldOrigin;
    v_normal = normal;
    v_height01 = (u_heightScale > 0.0) ? (height / u_heightScale) : 0.0;

    // Compute fog factor (linear fog)
    vec3 cameraLocal = u_cameraPos - u_worldOrigin;
    float dist = length(localPos - cameraLocal);
    v_fogFactor = clamp((dist - u_fogStart) / (u_fogEnd - u_fogStart), 0.0, 1.0);
}

#shader fragment
#version 330 core
in vec3 v_worldPos;
in vec3 v_normal;
in float v_fogFactor;
in float v_height01;

out vec4 FragColor;

// Lighting
uniform vec3 u_lightDir;

// Fog
uniform vec3 u_fogColor;

void main()
{
    // VAPORWAVE COLOR PALETTE: Deep purple → Pink → Cyan gradient
    vec3 color1 = vec3(0.4, 0.1, 0.6);   // Deep purple (low elevation)
    vec3 color2 = vec3(0.9, 0.3, 0.6);   // Hot pink (mid-low)
    vec3 color3 = vec3(0.95, 0.6, 0.4);  // Coral/orange (mid-high)
    vec3 color4 = vec3(0.3, 0.8, 0.95);  // Cyan (high elevation)

    // Multi-stop gradient for richer vaporwave aesthetic
    float h = clamp(v_height01, 0.0, 1.0);
    vec3 baseColor;
    if (h < 0.33) {
        float t = smoothstep(0.0, 0.33, h);
        baseColor = mix(color1, color2, t);
    } else if (h < 0.66) {
        float t = smoothstep(0.33, 0.66, h);
        baseColor = mix(color2, color3, t);
    } else {
        float t = smoothstep(0.66, 1.0, h);
        baseColor = mix(color3, color4, t);
    }

    // Simple Lambert diffuse lighting
    vec3 normal = normalize(v_normal);
    // u_lightDir points FROM light TO surface, negate for dot product
    vec3 toLight = normalize(-u_lightDir);
    float NdotL = max(dot(normal, toLight), 0.0);

    // Ambient + diffuse
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * baseColor;
    vec3 diffuse = NdotL * baseColor;

    vec3 litColor = ambient + diffuse;

    // Apply distance fog
    vec3 finalColor = mix(litColor, u_fogColor, v_fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
