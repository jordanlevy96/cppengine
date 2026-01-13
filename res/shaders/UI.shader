#shader vertex
#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}

#shader fragment
#version 330 core
out vec4 FragColor;

uniform vec4 color;

void main()
{
    FragColor = color;
}
