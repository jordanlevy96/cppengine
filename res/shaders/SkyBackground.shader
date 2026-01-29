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

float saturate(float x) { return clamp(x, 0.0, 1.0); }

void main()
{
    float t = saturate(vUV.y);
    vec3 color = mix(u_bottomColor, u_topColor, t);

    // Horizon glow (Gaussian-like)
    float h = abs(t - u_horizonY);
    float hg = (u_horizonGlow <= 0.0) ? 0.0 : exp(- (h * h) / max(1e-6, u_horizonGlow * u_horizonGlow));
    color = mix(color, u_horizonColor, saturate(hg) * 0.35);

    // Sun disc + glow
    float d = length(vUV - u_sunPos);
    float disc = 1.0 - smoothstep(u_sunRadius * 0.98, u_sunRadius * 1.02, d);
    float glowR = u_sunRadius * (1.0 + max(u_sunGlow, 0.0));
    float glow = 1.0 - smoothstep(glowR * 0.5, glowR, d);
    color += u_sunColor * (disc * 0.9 + glow * 0.25);

    FragColor = vec4(saturate(color.r), saturate(color.g), saturate(color.b), 1.0);
}

