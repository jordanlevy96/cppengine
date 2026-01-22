#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 LocalPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = aNormal;
    LocalPos = aPos;
    gl_Position = projection * view * vec4(FragPos, 1.0f);
}

#shader fragment
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec3 LocalPos;

uniform vec4 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

float ambientStrength = 0.1;
float specularStrength = 0.5;

void main()
{
    // 3 Components of Light: Ambient,
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse,
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // and Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor.rgb;

    // Cube outline: silhouette + explicit face-border outlines in object space.
    vec3 normView = normalize(Normal);
    vec3 viewDirNorm = normalize(viewPos - FragPos);
    float edgeFactor = abs(dot(normView, viewDirNorm));

    // Create sharp outline at edges (where surface faces away from camera)
    float outlineThreshold = 0.3;  // Adjust this to control outline thickness (lower = thicker)
    float outlineIntensity = smoothstep(outlineThreshold, outlineThreshold + 0.1, edgeFactor);

    vec3 absNorm = abs(norm);
    vec2 faceUV = vec2(LocalPos.x, LocalPos.y);
    if (absNorm.x > absNorm.y && absNorm.x > absNorm.z)
    {
        faceUV = vec2(LocalPos.y, LocalPos.z);
    }
    else if (absNorm.y > absNorm.z)
    {
        faceUV = vec2(LocalPos.x, LocalPos.z);
    }

    float edgeDist = min(1.0 - abs(faceUV.x), 1.0 - abs(faceUV.y));
    float faceOutlineThickness = 0.2;  // In object-space units; lower = thinner outline
    float faceOutlineIntensity = 1.0 - smoothstep(0.0, faceOutlineThickness, edgeDist);

    // Combine silhouette and face-edge outlines.
    float combinedOutline = max(outlineIntensity, faceOutlineIntensity);

    // Mix outline color (dark) with lit color
    vec3 outlineColor = vec3(0.0, 0.0, 0.0);  // Black outline
    result = mix(outlineColor, result, combinedOutline);

    FragColor = vec4(result, objectColor.a);
}
