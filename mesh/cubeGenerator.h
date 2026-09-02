#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include "../Linking/include/glad/glad.h"
#include "../objects/vertex.h"
#include "../objects/cube.h"

inline void generateCubes(
    const CubeParams& params,
    std::vector<Vertex>& vertices,
    std::vector<GLuint>& indices)
{
    vertices.clear();
    indices.clear();

    if (params.count <= 0)
        return;

    const size_t totalVertices = static_cast<size_t>(params.count) * 24;
    const size_t totalIndices = static_cast<size_t>(params.count) * 36;
    vertices.reserve(totalVertices);
    indices.reserve(totalIndices);

    const unsigned int baseSeed = (params.seed != 0 ? params.seed : 1337);

    for (int k = 0; k < params.count; ++k)
    {
        std::mt19937 rng(baseSeed + static_cast<unsigned int>(k) * 2654435761u);
        std::uniform_real_distribution<float> distNeg1To1(-1.0f, 1.0f);
        std::uniform_real_distribution<float> dist0To1(0.0f, 1.0f);

        // Random spatial placement (X, Z) within scaleOfRandomness range
        float posX = distNeg1To1(rng) * params.scaleOfRandomness * 0.5f;
        float posZ = distNeg1To1(rng) * params.scaleOfRandomness * 0.5f;

        // Dimension randomization scale factors
        float randW = 1.0f + distNeg1To1(rng) * params.dimensionRandomness;
        float randH = 1.0f + distNeg1To1(rng) * params.dimensionRandomness;
        float randD = 1.0f + distNeg1To1(rng) * params.dimensionRandomness;

        float w = std::max(0.1f, params.width * randW);
        float h = std::max(0.1f, params.height * randH);
        float d = std::max(0.1f, params.depth * randD);

        float posY = dist0To1(rng) * 2.0f;

        // Procedural color variation per cube (warm orange/brownish tint)
        glm::vec3 cubeColor(
            0.8f + dist0To1(rng) * 0.2f,
            0.3f + dist0To1(rng) * 0.4f,
            0.1f + dist0To1(rng) * 0.3f
        );

        float halfW = w * 0.5f;
        float halfH = h * 0.5f;
        float halfD = d * 0.5f;

        GLuint baseIndex = static_cast<GLuint>(vertices.size());

        // ----------------------------------------------------
        // Front (+Z)
        // ----------------------------------------------------
        vertices.push_back({ {posX - halfW, posY - halfH, posZ + halfD}, cubeColor, {0.f, 0.f, 1.f}, {0.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY - halfH, posZ + halfD}, cubeColor, {0.f, 0.f, 1.f}, {1.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY + halfH, posZ + halfD}, cubeColor, {0.f, 0.f, 1.f}, {1.f, 1.f} });
        vertices.push_back({ {posX - halfW, posY + halfH, posZ + halfD}, cubeColor, {0.f, 0.f, 1.f}, {0.f, 1.f} });

        // ----------------------------------------------------
        // Back (-Z)
        // ----------------------------------------------------
        vertices.push_back({ {posX + halfW, posY - halfH, posZ - halfD}, cubeColor, {0.f, 0.f, -1.f}, {0.f, 0.f} });
        vertices.push_back({ {posX - halfW, posY - halfH, posZ - halfD}, cubeColor, {0.f, 0.f, -1.f}, {1.f, 0.f} });
        vertices.push_back({ {posX - halfW, posY + halfH, posZ - halfD}, cubeColor, {0.f, 0.f, -1.f}, {1.f, 1.f} });
        vertices.push_back({ {posX + halfW, posY + halfH, posZ - halfD}, cubeColor, {0.f, 0.f, -1.f}, {0.f, 1.f} });

        // ----------------------------------------------------
        // Left (-X)
        // ----------------------------------------------------
        vertices.push_back({ {posX - halfW, posY - halfH, posZ - halfD}, cubeColor, {-1.f, 0.f, 0.f}, {0.f, 0.f} });
        vertices.push_back({ {posX - halfW, posY - halfH, posZ + halfD}, cubeColor, {-1.f, 0.f, 0.f}, {1.f, 0.f} });
        vertices.push_back({ {posX - halfW, posY + halfH, posZ + halfD}, cubeColor, {-1.f, 0.f, 0.f}, {1.f, 1.f} });
        vertices.push_back({ {posX - halfW, posY + halfH, posZ - halfD}, cubeColor, {-1.f, 0.f, 0.f}, {0.f, 1.f} });

        // ----------------------------------------------------
        // Right (+X)
        // ----------------------------------------------------
        vertices.push_back({ {posX + halfW, posY - halfH, posZ + halfD}, cubeColor, {1.f, 0.f, 0.f}, {0.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY - halfH, posZ - halfD}, cubeColor, {1.f, 0.f, 0.f}, {1.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY + halfH, posZ - halfD}, cubeColor, {1.f, 0.f, 0.f}, {1.f, 1.f} });
        vertices.push_back({ {posX + halfW, posY + halfH, posZ + halfD}, cubeColor, {1.f, 0.f, 0.f}, {0.f, 1.f} });

        // ----------------------------------------------------
        // Top (+Y)
        // ----------------------------------------------------
        vertices.push_back({ {posX - halfW, posY + halfH, posZ + halfD}, cubeColor, {0.f, 1.f, 0.f}, {0.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY + halfH, posZ + halfD}, cubeColor, {0.f, 1.f, 0.f}, {1.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY + halfH, posZ - halfD}, cubeColor, {0.f, 1.f, 0.f}, {1.f, 1.f} });
        vertices.push_back({ {posX - halfW, posY + halfH, posZ - halfD}, cubeColor, {0.f, 1.f, 0.f}, {0.f, 1.f} });

        // ----------------------------------------------------
        // Bottom (-Y)
        // ----------------------------------------------------
        vertices.push_back({ {posX - halfW, posY - halfH, posZ - halfD}, cubeColor, {0.f, -1.f, 0.f}, {0.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY - halfH, posZ - halfD}, cubeColor, {0.f, -1.f, 0.f}, {1.f, 0.f} });
        vertices.push_back({ {posX + halfW, posY - halfH, posZ + halfD}, cubeColor, {0.f, -1.f, 0.f}, {1.f, 1.f} });
        vertices.push_back({ {posX - halfW, posY - halfH, posZ + halfD}, cubeColor, {0.f, -1.f, 0.f}, {0.f, 1.f} });

        // Indices (12 triangles = 36 indices per cube)
        const GLuint quadIndices[] = {
            0, 1, 2,  2, 3, 0,       // Front
            4, 5, 6,  6, 7, 4,       // Back
            8, 9, 10, 10, 11, 8,     // Left
            12, 13, 14, 14, 15, 12,  // Right
            16, 17, 18, 18, 19, 16,  // Top
            20, 21, 22, 22, 23, 20   // Bottom
        };

        for (GLuint idx : quadIndices)
        {
            indices.push_back(baseIndex + idx);
        }
    }
}

// ----------------------------------------------------------------------------
// Legacy Single Unit Cube Class (kept for backward compatibility)
// ----------------------------------------------------------------------------
class Cube
{
public:
    Cube() = default;

    ~Cube()
    {
        releaseGpu();
    }

    void uploadToGpu()
    {
        if (VAO != 0)
            return;

        const float vertices[] =
        {
            -0.5f, -0.5f,  0.5f,     0.f, 0.f, 1.f,     0.8f, 0.8f, 0.8f,
             0.5f, -0.5f,  0.5f,     0.f, 0.f, 1.f,     0.8f, 0.8f, 0.8f,
             0.5f,  0.5f,  0.5f,     0.f, 0.f, 1.f,     0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f,  0.5f,     0.f, 0.f, 1.f,     0.8f, 0.8f, 0.8f,

            -0.5f, -0.5f, -0.5f,     0.f, 0.f,-1.f,     0.8f, 0.8f, 0.8f,
             0.5f, -0.5f, -0.5f,     0.f, 0.f,-1.f,     0.8f, 0.8f, 0.8f,
             0.5f,  0.5f, -0.5f,     0.f, 0.f,-1.f,     0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f, -0.5f,     0.f, 0.f,-1.f,     0.8f, 0.8f, 0.8f,

            -0.5f, -0.5f, -0.5f,    -1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,
            -0.5f, -0.5f,  0.5f,    -1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f,  0.5f,    -1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f, -0.5f,    -1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,

             0.5f, -0.5f,  0.5f,     1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f, -0.5f, -0.5f,     1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f,  0.5f, -0.5f,     1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f,  0.5f,  0.5f,     1.f, 0.f, 0.f,     0.8f, 0.8f, 0.8f,

            -0.5f,  0.5f,  0.5f,     0.f, 1.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f,  0.5f,  0.5f,     0.f, 1.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f,  0.5f, -0.5f,     0.f, 1.f, 0.f,     0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f, -0.5f,     0.f, 1.f, 0.f,     0.8f, 0.8f, 0.8f,

            -0.5f, -0.5f, -0.5f,     0.f,-1.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f, -0.5f, -0.5f,     0.f,-1.f, 0.f,     0.8f, 0.8f, 0.8f,
             0.5f, -0.5f,  0.5f,     0.f,-1.f, 0.f,     0.8f, 0.8f, 0.8f,
            -0.5f, -0.5f,  0.5f,     0.f,-1.f, 0.f,     0.8f, 0.8f, 0.8f
        };

        const GLuint indices[] =
        {
            0, 1, 2,  2, 3, 0,
            4, 6, 5,  6, 4, 7,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw() const
    {
        if (VAO == 0)
            return;

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, INDEX_COUNT, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void releaseGpu()
    {
        if (EBO != 0)
        {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }

        if (VBO != 0)
        {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }

        if (VAO != 0)
        {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
    }

    GLuint getVAO() const
    {
        return VAO;
    }

    static constexpr GLsizei getIndexCount()
    {
        return INDEX_COUNT;
    }

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    static constexpr GLsizei INDEX_COUNT = 36;
};
