#include "Terrain.h"
#include "Vertex.h" // Include the Vertex header file

Terrain::Terrain(int gridSize, float scale, FastNoiseLite& noise)
    : gridSize(gridSize), scale(scale), noise(noise) {}

glm::vec3 Terrain::getBiomeColor(float noiseValue) {
    if (noiseValue < -0.3f) {
        return glm::vec3(0.3f, 0.1f, 0.1f); // Dark red ground
    }
    else if (noiseValue < 0.0f) {
        return glm::vec3(0.2f, 0.1f, 0.1f); // Dark brown ground
    }
    else if (noiseValue < 0.3f) {
        return glm::vec3(0.1f, 0.1f, 0.1f); // Dark gray ground
    }
    else {
        return glm::vec3(0.2f, 0.2f, 0.2f); // Darker gray ground
    }
}

void Terrain::generateTerrain(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            float noiseValue = noise.GetNoise((float)x, (float)z);
            float height = noiseValue * scale;

            vertices.push_back({ glm::vec3((float)x, height, (float)z), getBiomeColor(noiseValue), glm::vec3(0.0f, 1.0f, 0.0f) });
        }
    }

    for (int z = 0; z < gridSize; ++z) {
        for (int x = 0; x < gridSize; ++x) {
            int topLeft = z * (gridSize + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridSize + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Calculate normals
    for (int i = 0; i < indices.size(); i += 3) {
        glm::vec3 v0 = vertices[indices[i]].position;
        glm::vec3 v1 = vertices[indices[i + 1]].position;
        glm::vec3 v2 = vertices[indices[i + 2]].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        vertices[indices[i]].normal += normal;
        vertices[indices[i + 1]].normal += normal;
        vertices[indices[i + 2]].normal += normal;
    }

    for (auto& vertex : vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }
}
