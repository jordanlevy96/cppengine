#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
// layout (location = 2) in vec2 aTexCoord;

// out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = aNormal;
    // TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0f);
}

#shader fragment
#version 330 core
out vec4 FragColor;
  
in vec2 TexCoord;
// in vec3 Normal;

uniform vec3 objectColor;
uniform vec3 lightColor;

// uniform sampler2D texture_diffuse1;
// uniform sampler2D texture2;

void main()
{
    // FragColor = texture(texture_diffuse1, TexCoord);
    FragColor = vec4(lightColor * objectColor, 1.0);
}