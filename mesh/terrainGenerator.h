#pragma once

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include "../Linking/include/glad/glad.h"
#include "../objects/vertex.h"

// ----------------------------------------------------------------------------
// 1. Static Topology: Index Generation
// ----------------------------------------------------------------------------
inline void generateTerrainIndices(int m, int n, std::vector<GLuint>& indices)
{
    if (m < 2 || n < 2)
        return;

    const size_t indexCount =
        static_cast<size_t>(m - 1) * (n - 1) * 6;

    indices.resize(indexCount);

    size_t index = 0;
    for (int i = 0; i < m - 1; ++i)
    {
        for (int j = 0; j < n - 1; ++j)
        {
            const GLuint topLeft =
                static_cast<GLuint>(i * n + j);

            const GLuint topRight =
                topLeft + 1;

            const GLuint bottomLeft =
                static_cast<GLuint>((i + 1) * n + j);

            const GLuint bottomRight =
                bottomLeft + 1;

            indices[index++] = topLeft;
            indices[index++] = bottomLeft;
            indices[index++] = topRight;

            indices[index++] = topRight;
            indices[index++] = bottomLeft;
            indices[index++] = bottomRight;
        }
    }
}

// ----------------------------------------------------------------------------
// 2. Static Vertex Attributes Initialization (X, Z, TexCoords)
// ----------------------------------------------------------------------------
inline void initStaticVertices(int m, int n, std::vector<Vertex>& vertices)
{
    if (m <= 0 || n <= 0)
        return;

    const size_t vertexCount =
        static_cast<size_t>(m) * n;

    vertices.resize(vertexCount);

    const float invM = (m > 1) ? 1.0f / static_cast<float>(m - 1) : 0.0f;
    const float invN = (n > 1) ? 1.0f / static_cast<float>(n - 1) : 0.0f;

    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            const size_t index =
                static_cast<size_t>(i) * n + j;

            Vertex& v = vertices[index];

            v.position.x = static_cast<float>(i) - m * 0.5f;
            v.position.z = static_cast<float>(j) - n * 0.5f;

            v.texCoord = glm::vec2(
                static_cast<float>(i) * invM,
                static_cast<float>(j) * invN
            );
        }
    }
}

// ----------------------------------------------------------------------------
// 3. Terrain Height Generation & Smoothing (Independent of OpenGL)
// ----------------------------------------------------------------------------
inline void generateTerrainHeights(
    int m,
    int n,
    int numHills,
    float maxRadius,
    float heightScale,
    int smoothingPasses,
    unsigned int seed,
    std::vector<float>& heights,
    std::vector<float>& temp)
{
    if (m <= 0 || n <= 0)
        return;

    const size_t vertexCount =
        static_cast<size_t>(m) * n;

    heights.resize(vertexCount);
    std::fill(heights.begin(), heights.end(), 0.0f);

    temp.resize(vertexCount);

    const unsigned int baseSeed = (seed != 0 ? seed : 1337);
    constexpr float PI = 3.14159265358979323846f;

    // Generate hills using per-hill deterministic seeding based on baseSeed
    for (int k = 0; k < numHills; ++k)
    {
        std::mt19937 rng(baseSeed + static_cast<unsigned int>(k) * 2654435761u);
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

        const float cx =
            dist01(rng) * static_cast<float>(m - 1);

        const float cz =
            dist01(rng) * static_cast<float>(n - 1);

        const float radius =
            dist01(rng) * maxRadius + 2.0f;

        const float radiusSq =
            radius * radius;

        const float h =
            (dist01(rng) * 2.0f - 1.0f)
            * heightScale * 15.0f;

        const int minX =
            std::max(0, static_cast<int>(cx - radius));

        const int maxX =
            std::min(m - 1, static_cast<int>(cx + radius));

        const int minZ =
            std::max(0, static_cast<int>(cz - radius));

        const int maxZ =
            std::min(n - 1, static_cast<int>(cz + radius));

        const float invRadius =
            1.0f / radius;

        for (int i = minX; i <= maxX; ++i)
        {
            const float dx =
                static_cast<float>(i) - cx;

            for (int j = minZ; j <= maxZ; ++j)
            {
                const float dz =
                    static_cast<float>(j) - cz;

                const float distSq =
                    dx * dx + dz * dz;

                if (distSq >= radiusSq)
                    continue;

                const float dist =
                    std::sqrt(distSq);

                const float t =
                    dist * invRadius;

                const float falloff =
                    0.5f * (std::cos(PI * t) + 1.0f);

                heights[
                    static_cast<size_t>(i) * n + j
                ] += h * falloff;
            }
        }
    }

    // Smoothing passes
    for (int pass = 0; pass < smoothingPasses; ++pass)
    {
        // Horizontal pass
        for (int i = 0; i < m; ++i)
        {
            const size_t row =
                static_cast<size_t>(i) * n;

            for (int j = 0; j < n; ++j)
            {
                float sum = heights[row + j];
                int count = 1;

                if (j > 0)
                {
                    sum += heights[row + j - 1];
                    ++count;
                }

                if (j + 1 < n)
                {
                    sum += heights[row + j + 1];
                    ++count;
                }

                temp[row + j] =
                    sum / static_cast<float>(count);
            }
        }

        // Vertical pass
        for (int i = 0; i < m; ++i)
        {
            const size_t row =
                static_cast<size_t>(i) * n;

            for (int j = 0; j < n; ++j)
            {
                float sum = temp[row + j];
                int count = 1;

                if (i > 0)
                {
                    sum += temp[
                        static_cast<size_t>(i - 1) * n + j
                    ];
                    ++count;
                }

                if (i + 1 < m)
                {
                    sum += temp[
                        static_cast<size_t>(i + 1) * n + j
                    ];
                    ++count;
                }

                heights[row + j] =
                    sum / static_cast<float>(count);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// 4. Dynamic Vertex Attribute Update (Height, Normal, Color)
// ----------------------------------------------------------------------------
inline void updateTerrainVertices(
    int m,
    int n,
    const std::vector<float>& heights,
    std::vector<Vertex>& vertices)
{
    if (m <= 0 || n <= 0)
        return;

    const size_t vertexCount =
        static_cast<size_t>(m) * n;

    if (vertices.size() < vertexCount)
        vertices.resize(vertexCount);

    for (int i = 0; i < m; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            const size_t index =
                static_cast<size_t>(i) * n + j;

            const float height =
                heights[index];

            Vertex& v = vertices[index];

            v.position.y = height;

            const float colorFactor =
                glm::clamp(
                    (height + 5.0f) * 0.1f,
                    0.0f,
                    1.0f
                );

            v.color = glm::vec3(
                0.2f,
                0.5f + colorFactor * 0.5f,
                0.2f + (1.0f - colorFactor) * 0.3f
            );

            const float hL =
                (i > 0)
                    ? heights[index - n]
                    : height;

            const float hR =
                (i + 1 < m)
                    ? heights[index + n]
                    : height;

            const float hD =
                (j > 0)
                    ? heights[index - 1]
                    : height;

            const float hU =
                (j + 1 < n)
                    ? heights[index + 1]
                    : height;

            v.normal = glm::normalize(
                glm::vec3(
                    hL - hR,
                    2.0f,
                    hD - hU
                )
            );
        }
    }
}

// Legacy single-function wrapper (backward compatibility)
inline void generateTerrain(
    int m,
    int n,
    int numHills,
    float maxRadius,
    float heightScale,
    int smoothingPasses,
    std::vector<Vertex>& vertices,
    std::vector<GLuint>& indices,
    unsigned int seed = 1337)
{
    std::vector<float> heights;
    std::vector<float> temp;
    initStaticVertices(m, n, vertices);
    generateTerrainIndices(m, n, indices);
    generateTerrainHeights(m, n, numHills, maxRadius, heightScale, smoothingPasses, seed, heights, temp);
    updateTerrainVertices(m, n, heights, vertices);
}
