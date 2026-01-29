#shader vertex
#version 330 core

layout (location = 0) in vec2 aLocalXZ;   // [0..1] quad in XZ
layout (location = 1) in vec2 aUV;

layout (location = 2) in vec2 iOriginXZ;  // world-space origin (x,z)
layout (location = 3) in float iLayer;    // texture array layer index
layout (location = 4) in float iHasData;  // 1.0 if valid, else 0.0
layout (location = 5) in float iTileWorldSize;

out vec2 vUV;
flat out float vLayer;
flat out float vHasData;
out vec2 vWorldXZ;
out vec3 vWorldPos;

uniform mat4 view;
uniform mat4 projection;
uniform float u_planeY;

void main()
{
    vec3 worldPos = vec3(
        iOriginXZ.x + aLocalXZ.x * iTileWorldSize,
        u_planeY,
        iOriginXZ.y + aLocalXZ.y * iTileWorldSize
    );

    vUV = aUV;
    vLayer = iLayer;
    vHasData = iHasData;
    vWorldXZ = worldPos.xz;
    vWorldPos = worldPos;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}

#shader fragment
#version 330 core

in vec2 vUV;
flat in float vLayer;
flat in float vHasData;
in vec2 vWorldXZ;
in vec3 vWorldPos;

out vec4 FragColor;

uniform sampler2DArray u_tiles;
uniform vec4 u_fallbackColor;
uniform float u_gridSpacing;
uniform int u_majorEvery;
uniform float u_minorLineWidth;
uniform float u_majorLineWidth;
uniform vec4 u_minorLineColor;
uniform vec4 u_majorLineColor;

uniform vec3 u_cameraPos;
uniform float u_horizonBlendStart;
uniform float u_horizonBlendEnd;
uniform vec4 u_horizonBlendColor;

float gridLine1D(float coord, float spacing, float halfWidth)
{
    if (halfWidth <= 0.0 || spacing <= 0.0)
    {
        return 0.0;
    }

    // Work in normalized cell space to keep derivatives stable.
    float x = coord / spacing;
    float w = halfWidth / spacing;

    // Distance to nearest integer boundary (0 at grid line).
    float f = fract(x);
    float d = min(f, 1.0 - f);

    // Anti-alias width based on screen-space derivatives of the *continuous* coordinate.
    float aa = max(fwidth(x), 0.0001);

    // Keep lines from collapsing into subpixel flicker: enforce a minimum screen-space thickness.
    // (This also reduces the "every other line looks thicker" aliasing you can get on dense grids.)
    w = max(w, aa * 0.5);

    // Clamp to half a cell; beyond that we’re effectively averaging multiple lines anyway.
    aa = min(aa, 0.5);

    return 1.0 - smoothstep(w - aa, w + aa, d);
}

void main()
{
    vec4 baseColor = u_fallbackColor;
    if (vHasData < 0.5)
    {
        baseColor = u_fallbackColor;
    }
    else
    {
        baseColor = texture(u_tiles, vec3(vUV, vLayer));
    }

    float grid = max(0.0001, u_gridSpacing);
    float majorGrid = grid * max(u_majorEvery, 1);
    float minorHalfW = max(u_minorLineWidth, 0.0);
    float majorHalfW = max(u_majorLineWidth, 0.0);

    float minorAlpha = max(
        gridLine1D(vWorldXZ.x, grid, minorHalfW),
        gridLine1D(vWorldXZ.y, grid, minorHalfW)
    );

    float majorAlpha = max(
        gridLine1D(vWorldXZ.x, majorGrid, majorHalfW),
        gridLine1D(vWorldXZ.y, majorGrid, majorHalfW)
    );

    // Ground->sky blending near horizon:
    // Fade both base color and line intensity to avoid a hard seam and reduce distant aliasing.
    float distXZ = length(vWorldPos.xz - u_cameraPos.xz);
    float startD = max(u_horizonBlendStart, 0.0);
    float endD = max(u_horizonBlendEnd, startD + 0.0001);
    float blendT = smoothstep(startD, endD, distXZ);

    // Fade lines out slightly faster than the base color to reduce high-frequency shimmer.
    float lineFade = 1.0 - blendT;
    minorAlpha *= lineFade;
    majorAlpha *= lineFade;

    vec4 color = baseColor;
    color = mix(color, u_minorLineColor, minorAlpha);
    color = mix(color, u_majorLineColor, majorAlpha);

    color = mix(color, u_horizonBlendColor, blendT);

    FragColor = color;
}
