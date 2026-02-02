#shader vertex
#version 330 core

layout (location = 0) in vec2 aUV; // (u,v) in [0..1]

out vec2 vWorldXZ;
out float vForwardDist;

uniform mat4 view;
uniform mat4 projection;

uniform vec2 u_cameraPosXZ;
uniform vec2 u_cameraForwardXZ;
uniform vec2 u_cameraRightXZ;

uniform float u_startDistance;
uniform float u_depth;
uniform float u_halfWidth;

uniform float u_baseY;
uniform float u_height;
uniform float u_noiseScale;
uniform float u_detail;
uniform float u_scrollSpeed;
uniform float u_time;

float hash21(vec2 p)
{
    // 2D hash (GLSL 330 friendly)
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise2(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm2(vec2 p)
{
    float v = 0.0;
    float a = 0.5;
    float f = 1.0;
    for (int i = 0; i < 5; i++)
    {
        v += a * noise2(p * f);
        f *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main()
{
    float u = aUV.x;
    float v = aUV.y;

    float x = (u - 0.5) * 2.0 * u_halfWidth;
    float z = u_startDistance + v * max(u_depth, 0.0);

    vec2 worldXZ = u_cameraPosXZ + u_cameraForwardXZ * z + u_cameraRightXZ * x;
    vWorldXZ = worldXZ;
    vForwardDist = z;

    float scroll = u_time * u_scrollSpeed;
    vec2 p = (worldXZ + vec2(scroll, 0.0)) * u_noiseScale;

    float n0 = fbm2(p);
    float n1 = fbm2(p * 3.0 + vec2(12.3, 4.7));
    float n = mix(n0, n1, clamp(u_detail, 0.0, 1.0));

    // Shape: strong ridge at horizon (v=0), slightly diminishing with depth.
    float ridge = (0.35 + 0.65 * n) * (1.0 - 0.55 * v);
    float y = u_baseY + u_height * ridge;

    vec3 worldPos = vec3(worldXZ.x, y, worldXZ.y);
    gl_Position = projection * view * vec4(worldPos, 1.0);
}

#shader fragment
#version 330 core

in vec2 vWorldXZ;
in float vForwardDist;

out vec4 FragColor;

uniform vec4 u_fillColor;
uniform float u_gridSpacing;
uniform int u_majorEvery;
uniform float u_minorLineWidth;
uniform float u_majorLineWidth;
uniform vec4 u_minorLineColor;
uniform vec4 u_majorLineColor;

float gridLine1D(float coord, float spacing, float halfWidth)
{
    if (halfWidth <= 0.0 || spacing <= 0.0)
    {
        return 0.0;
    }

    float x = coord / spacing;
    float w = halfWidth / spacing;

    float f = fract(x);
    float d = min(f, 1.0 - f);

    float aa = max(fwidth(x), 0.0001);
    w = max(w, aa * 0.5);
    aa = min(aa, 0.5);

    return 1.0 - smoothstep(w - aa, w + aa, d);
}

void main()
{
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

    vec4 color = u_fillColor;
    color = mix(color, u_minorLineColor, minorAlpha);
    color = mix(color, u_majorLineColor, majorAlpha);
    FragColor = color;
}

