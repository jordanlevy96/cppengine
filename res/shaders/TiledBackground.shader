#shader vertex
#version 330 core

layout (location = 0) in vec2 aLocalXZ;   // [0..1] quad in XZ
layout (location = 1) in vec2 aUV;

layout (location = 2) in vec2 iOriginXZ;  // world-space origin (x,z)
layout (location = 3) in float iLayer;    // texture array layer index

out vec2 vUV;
out float vLayer;

uniform mat4 view;
uniform mat4 projection;
uniform float u_planeY;
uniform float u_tileWorldSize;

void main()
{
    vec3 worldPos = vec3(
        iOriginXZ.x + aLocalXZ.x * u_tileWorldSize,
        u_planeY,
        iOriginXZ.y + aLocalXZ.y * u_tileWorldSize
    );

    vUV = aUV;
    vLayer = iLayer;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}

#shader fragment
#version 330 core

in vec2 vUV;
in float vLayer;

out vec4 FragColor;

uniform sampler2DArray u_tiles;

void main()
{
    FragColor = texture(u_tiles, vec3(vUV, vLayer));
}
