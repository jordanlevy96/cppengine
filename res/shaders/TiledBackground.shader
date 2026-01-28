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
    gl_Position = projection * view * vec4(worldPos, 1.0);
}

#shader fragment
#version 330 core

in vec2 vUV;
flat in float vLayer;
flat in float vHasData;

out vec4 FragColor;

uniform sampler2DArray u_tiles;
uniform vec4 u_fallbackColor;

void main()
{
    if (vHasData < 0.5)
    {
        FragColor = u_fallbackColor;
    }
    else
    {
        FragColor = texture(u_tiles, vec3(vUV, vLayer));
    }
}
