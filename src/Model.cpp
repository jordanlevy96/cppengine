
#define TINYOBJLOADER_USE_CPP11
#define TINYOBJLOADER_IMPLEMENTATION

#include <Model.h>

#include <stb_image.h>
#include <glad/glad.h>

#include <iostream>

void Model::ExtractIndices(std::vector<tinyobj::shape_t> shapes)
{
    for (tinyobj::shape_t shape : shapes)
    {
        const std::vector<tinyobj::index_t> &indices = shape.mesh.indices;
        const std::vector<int> &material_ids = shape.mesh.material_ids;

        for (tinyobj::index_t index : indices)
        {
            int vval = index.vertex_index > 0 ? index.vertex_index : -1;
            extractedIndices.Vertex.push_back(vval);
            int nval = index.vertex_index > 0 ? index.vertex_index : -1;
            extractedIndices.Normal.push_back(nval);
            int tval = index.vertex_index > 0 ? index.vertex_index : -1;
            extractedIndices.Texture.push_back(tval);
        }
    }
    numVertices = extractedIndices.Vertex.size() / 3;
}

Model::Model(const char *modelSrc)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelSrc))
    {
        if (!warn.empty())
        {
            std::cerr << "Warning: " << warn << std::endl;
        }
        if (!err.empty())
        {
            std::cerr << "Error: " << err << std::endl;
        }
    }

    ExtractIndices(shapes);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, attrib.vertices.size() * sizeof(float), attrib.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * extractedIndices.Vertex.size(), extractedIndices.Vertex.data(), GL_STATIC_DRAW);

    // position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
    glEnableVertexAttribArray(0);

    // normal attribute (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture coord attribute (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

Model::~Model()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    extractedIndices.Vertex.clear();
    extractedIndices.Normal.clear();
    extractedIndices.Texture.clear();

    for (GLuint texture : textures)
    {
        glDeleteTextures(1, &texture);
    }
    textures.clear();
}

static GLuint loadTexture(const char *filepath, bool alpha)
{
    GLuint texture;
    int width, height, numChannels;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    unsigned char *data = stbi_load(filepath, &width, &height, &numChannels, 0);
    GLenum format = alpha ? GL_RGBA : GL_RGB;
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

void Model::AddTexture(const char *textureSrc, bool alpha)
{
    textures.push_back(loadTexture(textureSrc, alpha));
}