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
    gl_Position = projection * view * vec4(worldPos, 1.0);
}

#shader fragment
#version 330 core

in vec2 vUV;
flat in float vLayer;
flat in float vHasData;
in vec2 vWorldXZ;

out vec4 FragColor;

uniform sampler2DArray u_tiles;
uniform vec4 u_fallbackColor;
uniform float u_gridSpacing;
uniform int u_majorEvery;
uniform float u_lineWidth;
uniform vec4 u_minorLineColor;
uniform vec4 u_majorLineColor;

float lineMask(float dist, float halfWidth)
{
    if (halfWidth <= 0.0)
    {
        return 0.0;
    }

    // Anti-alias but clamp to avoid excessive blurring at grazing angles.
    float aa = fwidth(dist);
    float aaMax = max(halfWidth * 0.75, 0.0001);
    aa = clamp(aa, 0.0001, aaMax);

    return 1.0 - smoothstep(halfWidth - aa, halfWidth + aa, dist);
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
    float halfW = max(u_lineWidth, 0.0);

    float mx = mod(vWorldXZ.x, grid);
    float mz = mod(vWorldXZ.y, grid);
    float distMinorX = min(mx, grid - mx);
    float distMinorZ = min(mz, grid - mz);
    float minorDist = min(distMinorX, distMinorZ);

    float Mx = mod(vWorldXZ.x, majorGrid);
    float Mz = mod(vWorldXZ.y, majorGrid);
    float distMajorX = min(Mx, majorGrid - Mx);
    float distMajorZ = min(Mz, majorGrid - Mz);
    float majorDist = min(distMajorX, distMajorZ);

    float minorAlpha = lineMask(minorDist, halfW);
    float majorAlpha = lineMask(majorDist, halfW * 1.5);

    vec4 color = baseColor;
    color = mix(color, u_minorLineColor, minorAlpha);
    color = mix(color, u_majorLineColor, majorAlpha);

    FragColor = color;
}
