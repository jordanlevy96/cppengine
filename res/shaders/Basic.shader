#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

#shader fragment
#version 330 core
out vec4 FragColor;
  
in vec2 TexCoord;

// uniform sampler2D texture_diffuse1;
// uniform sampler2D texture2;

void main()
{
    // FragColor = texture(texture_diffuse1, TexCoord);
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}