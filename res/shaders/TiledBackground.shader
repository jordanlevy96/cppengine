#shader vertex
#version 330 core

layout (location = 0) in vec2 aLocalXZ;   // [0..1] quad in XZ
layout (location = 1) in vec4 aUVSkirtEdge; // (u,v,skirtFlag,edgeId)

layout (location = 2) in vec2 iOriginXZ;  // world-space origin (x,z)
layout (location = 3) in float iLayer;    // texture array layer index
layout (location = 4) in float iHasData;  // 1.0 if valid, else 0.0
layout (location = 5) in float iTileWorldSize;
layout (location = 6) in float iSkirtMask; // edge bitmask (0=west,1=east,2=south,3=north)

out vec2 vUV;
flat out float vLayer;
flat out float vHasData;
out vec2 vWorldXZ;
out vec3 vWorldPos;
out float vScreenY;
out float vForwardDist;
out float vSkirt;

uniform mat4 view;
uniform mat4 projection;
uniform float u_planeY;

uniform vec2 u_cameraPosXZ;
uniform vec2 u_cameraForwardXZ;

uniform sampler2DArray u_heights;
uniform float u_farDistance;
uniform float u_mountainExtraDistance;
uniform float u_mountainHeight;
uniform float u_mountainFadeDistance;
uniform float u_mountainRiseExponent;
uniform float u_skirtDepth;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main()
{
    vec2 uv = aUVSkirtEdge.xy;
    float skirtFlag = aUVSkirtEdge.z;
    float edgeId = aUVSkirtEdge.w;

    float edgeEnabled = 1.0;
    if (edgeId >= 0.0)
    {
        int mask = int(iSkirtMask + 0.5);
        int eid = int(edgeId + 0.5);
        edgeEnabled = ((mask & (1 << eid)) != 0) ? 1.0 : 0.0;
    }

    float skirt = skirtFlag * edgeEnabled;
    vec3 worldPos = vec3(
        iOriginXZ.x + aLocalXZ.x * iTileWorldSize,
        u_planeY,
        iOriginXZ.y + aLocalXZ.y * iTileWorldSize
    );

    float forwardDist = dot(worldPos.xz - u_cameraPosXZ, u_cameraForwardXZ);

    // Displace vertices beyond the horizon (u_farDistance) to form distant mountains.
    float mountainT = 0.0;
    if (u_mountainExtraDistance > 0.0 && u_mountainHeight > 0.0)
    {
        // Clamp fade length so a small mountainExtraDistance can still reach full amplitude.
        float fadeLen = max(min(u_mountainFadeDistance, u_mountainExtraDistance), 0.0001);
        mountainT = saturate((forwardDist - u_farDistance) / fadeLen);

        // Shape the rise curve so mountains read earlier near the horizon. Linear ramps can look like a flat "band"
        // behind the horizon unless fadeLen is very small.
        // (Exponent < 1.0 => faster rise near the horizon.)
        float exp = max(u_mountainRiseExponent, 0.0001);
        mountainT = pow(max(mountainT, 0.0), exp);
    }

    // Skirts are only needed where we have displacement (mountainT > 0). On the flat grid, the extra skirt triangles
    // can become a visible "band" at LOD boundaries, so we scale the skirt depth by mountainT.
    worldPos.y -= skirt * u_skirtDepth * mountainT;

    if (mountainT > 0.0 && iHasData > 0.5)
    {
        float h01 = texture(u_heights, vec3(uv, iLayer)).r;
        worldPos.y += h01 * u_mountainHeight * mountainT;
    }

    vec4 viewPos = view * vec4(worldPos, 1.0);
    vec4 clip = projection * viewPos;

    vUV = uv;
    vLayer = iLayer;
    vHasData = iHasData;
    vWorldXZ = worldPos.xz;
    vWorldPos = worldPos;
    vScreenY = clip.y / max(1e-6, clip.w) * 0.5 + 0.5;
    vForwardDist = forwardDist;
    vSkirt = skirt * mountainT;
    gl_Position = clip;
}

#shader fragment
#version 330 core

in vec2 vUV;
flat in float vLayer;
flat in float vHasData;
in vec2 vWorldXZ;
in vec3 vWorldPos;
in float vScreenY;
in float vForwardDist;
in float vSkirt;

out vec4 FragColor;

uniform sampler2DArray u_tiles;
uniform float u_mountainExtraDistance;
uniform vec4 u_fallbackColor;
uniform float u_gridSpacing;
uniform int u_majorEvery;
uniform float u_minorLineWidth;
uniform float u_majorLineWidth;
uniform vec4 u_minorLineColor;
uniform vec4 u_majorLineColor;

uniform float u_farDistance;
uniform float u_horizonLinePixels;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

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

    // Fade grid lines on skirts so any seam fill reads as "solid shadow" instead of vertical grid walls.
    float skirtFade = 1.0 - saturate(vSkirt);
    minorAlpha *= skirtFade;
    majorAlpha *= skirtFade;

    // Clip to a maximum distance so the background does not extend infinitely.
    // The flat grid ends at u_farDistance; mountains (if enabled) can extend beyond it.
    float farD = max(u_farDistance, 0.0);
    float maxD = farD + max(u_mountainExtraDistance, 0.0);
    if (vForwardDist > maxD)
    {
        discard;
    }

    vec4 color = baseColor;
    color = mix(color, u_minorLineColor, minorAlpha);
    color = mix(color, u_majorLineColor, majorAlpha);

    // Horizon termination line: force the last visible line (flat grid only) to be magenta.
    // When mountains are enabled, drawing this across displaced geometry reads as a "slab".
    if (vForwardDist <= farD)
    {
        float aa = max(fwidth(vForwardDist), 1e-6);
        float halfW = aa * max(u_horizonLinePixels, 0.0) * 0.5;
        float d = abs(vForwardDist - farD);
        float horizonAlpha = 1.0 - smoothstep(halfW, halfW + aa, d);
        color = mix(color, u_majorLineColor, horizonAlpha);
    }

    FragColor = vec4(color.rgb, 1.0);
}
