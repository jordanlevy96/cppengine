#shader vertex
#version 330 core

out vec2 vUV;

void main()
{
    // Fullscreen triangle in NDC:
    // gl_VertexID: 0 -> (-1,-1), 1 -> (3,-1), 2 -> (-1,3)
    vec2 pos;
    if (gl_VertexID == 0) pos = vec2(-1.0, -1.0);
    else if (gl_VertexID == 1) pos = vec2(3.0, -1.0);
    else pos = vec2(-1.0, 3.0);

    gl_Position = vec4(pos, 0.0, 1.0);
    vUV = pos * 0.5 + 0.5;
}

#shader fragment
#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec3 u_topColor;
uniform vec3 u_bottomColor;

uniform float u_horizonY;
uniform float u_horizonGlow;
uniform vec3 u_horizonColor;

uniform vec2 u_sunPos;
uniform float u_sunRadius;
uniform float u_sunGlow;
uniform vec3 u_sunColor;

uniform float u_time;
uniform float u_aspect;

uniform int u_mountainsEnabled;
uniform int u_mountainsOccludeSun;
uniform vec3 u_mountainColor;
uniform float u_mountainBaseY;
uniform float u_mountainHeight;
uniform float u_mountainScale;
uniform float u_mountainDetail;
uniform float u_mountainScrollSpeed;
uniform float u_mountainEdgePixels;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

float hash11(float x)
{
    // GLSL 330-friendly hash.
    return fract(sin(x * 127.1) * 43758.5453123);
}

float noise1(float x)
{
    float i = floor(x);
    float f = fract(x);
    float a = hash11(i);
    float b = hash11(i + 1.0);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u);
}

float fbm1(float x)
{
    float v = 0.0;
    float a = 0.5;
    float f = 1.0;
    for (int i = 0; i < 5; i++)
    {
        v += a * noise1(x * f);
        f *= 2.0;
        a *= 0.5;
    }
    return v;
}

float mountainMask(vec2 uv)
{
    if (u_mountainsEnabled == 0)
    {
        return 0.0;
    }

    // Use aspect to keep mountain features consistent on wide screens.
    float x = (uv.x * 2.0 - 1.0) * u_aspect;
    float scroll = u_time * u_mountainScrollSpeed;
    float n0 = fbm1((x + scroll) * u_mountainScale);

    // Add a small amount of higher-frequency detail.
    float n1 = fbm1((x + scroll * 1.7) * u_mountainScale * 3.0);
    float n = mix(n0, n1, saturate(u_mountainDetail));

    // Mountain line in UV space.
    float yLine = u_mountainBaseY + u_mountainHeight * n;

    // Anti-aliased edge (in pixels, not a blur).
    float aa = max(fwidth(uv.y), 1e-6);
    float edge = aa * max(u_mountainEdgePixels, 0.0);
    return 1.0 - smoothstep(yLine - edge, yLine + edge, uv.y);
}

void main()
{
    const float kHorizonTint = 0.65;

    float t = saturate(vUV.y);
    vec3 color = mix(u_bottomColor, u_topColor, t);

    // Horizon blend (short-tail, not "foggy")
    float h = abs(t - u_horizonY);
    float hg = (u_horizonGlow <= 0.0) ? 0.0 : (1.0 - smoothstep(0.0, u_horizonGlow, h));
    color = mix(color, u_horizonColor, saturate(hg) * kHorizonTint);

    // Sun disc + glow
    float d = length(vUV - u_sunPos);
    float disc = 1.0 - smoothstep(u_sunRadius * 0.98, u_sunRadius * 1.02, d);
    float glowR = u_sunRadius * (1.0 + max(u_sunGlow, 0.0));
    float glow = 1.0 - smoothstep(glowR * 0.5, glowR, d);
    float sunTerm = (disc * 0.9 + glow * 0.25);

    float m = mountainMask(vUV);
    if (u_mountainsOccludeSun != 0)
    {
        sunTerm *= (1.0 - m);
    }
    color += u_sunColor * sunTerm;

    // Mountains overlay (in front of gradient + sun)
    color = mix(color, u_mountainColor, m);

    FragColor = vec4(saturate(color.r), saturate(color.g), saturate(color.b), 1.0);
}
