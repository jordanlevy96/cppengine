
#define TINYOBJLOADER_USE_CPP11
#define TINYOBJLOADER_IMPLEMENTATION

#include <Model.h>

#include <stb_image.h>
#include <glad/glad.h>

#include <iostream>

/*
void Model::ExtractIndices(std::vector<tinyobj::shape_t> shapes, tinyobj::attrib_t attrib)
{
    std::unordered_map<glm::vec3, int> vertexMap;

    for (tinyobj::shape_t shape : shapes)
    {
        const std::vector<tinyobj::index_t> &indices = shape.mesh.indices;
        const std::vector<int> &material_ids = shape.mesh.material_ids;

        for (const tinyobj::index_t &index : indices)
        {
            glm::vec3 vertex = glm::vec3(
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]);

            auto it = vertexMap.find(vertex);
            int vertexIndex;

            if (it == vertexMap.end())
            {
                // Vertex not found in the map, add it and assign a new index
                vertexIndex = vertexMap.size();
                vertexMap[vertex] = vertexIndex;
            }
            else
            {
                // Vertex already exists in the map, use its index
                vertexIndex = it->second;
            }

            // Use vertexIndex as needed (e.g., store it in your extractedIndices)
            extractedIndices.push_back(vertexIndex);

            // Other processing for normals, texture coordinates, etc.
        }
    }
}

void Model::InterleaveVertices(tinyobj::attrib_t attrib)
{
    for (size_t i = 0; i < attrib.vertices.size() / 3; i++)
    {
        // Position
        interleavedData.push_back(attrib.vertices[3 * i]);
        interleavedData.push_back(attrib.vertices[3 * i + 1]);
        interleavedData.push_back(attrib.vertices[3 * i + 2]);

        // Normal
        if (attrib.normals.size() > 0)
        {
            interleavedData.push_back(attrib.normals[3 * i]);
            interleavedData.push_back(attrib.normals[3 * i + 1]);
            interleavedData.push_back(attrib.normals[3 * i + 2]);
        }

        // Texture coordinate
        if (attrib.texcoords.size() > 0)
        {
            interleavedData.push_back(attrib.texcoords[2 * i]);
            interleavedData.push_back(attrib.texcoords[2 * i + 1]);
        }
    }
}

*/

Model::Model(const char *modelSrc)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    std::cout << "Constructing Model..." << std::endl;

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

    std::cout << "Loaded " << modelSrc << std::endl;

    for (const auto &shape : shapes)
    {
        std::cout << "Reading shape" << std::endl;
        numFaces += shape.mesh.material_ids.size();
        std::cout << numFaces << std::endl;
        for (const auto &index : shape.mesh.indices)
        {
            glm::vec4 pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
                1};

            // glm::vec3 normal = {
            //     attrib.normals[3 * index.normal_index + 0],
            //     attrib.normals[3 * index.normal_index + 1],
            //     attrib.normals[3 * index.normal_index + 2],
            // };

            // glm::vec2 texCoord = {
            //     attrib.texcoords[2 * index.texcoord_index + 0],
            //     attrib.texcoords[2 * index.texcoord_index + 1],
            // };

            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            // vertices.push_back(normal.x);
            // vertices.push_back(normal.y);
            // vertices.push_back(normal.z);
            // vertices.push_back(texCoord.x);
            // vertices.push_back(texCoord.y);
        }
    }

    std::cout << "Extracted vertices" << std::endl;

    // ExtractIndices(shapes, attrib);
    // InterleaveVertices(attrib);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tinyobj::index_t) * extractedIndices.size(), extractedIndices.data(), GL_STATIC_DRAW);

    // position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)0);
    glEnableVertexAttribArray(0);

    // // normal attribute (3 floats)
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void *)(3 * sizeof(float)));
    // glEnableVertexAttribArray(1);

    // // texture coord attribute (2 floats)
    // glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void *)(6 * sizeof(float)));
    // glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    std::cout << "Created Model" << std::endl;
}

Model::~Model()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    // glDeleteBuffers(1, &EBO);

    // extractedIndices.clear();
    vertices.clear();

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