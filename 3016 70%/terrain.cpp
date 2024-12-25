#include "Terrain.h"
#include "Vertex.h" // Include the Vertex header file

Terrain::Terrain(int gridSize, float scale, FastNoiseLite& noise)
    : gridSize(gridSize), scale(scale), noise(noise) {}

glm::vec3 Terrain::getBiomeColor(float noiseValue) {
    if (noiseValue < -0.3f) {
        return glm::vec3(0.4f, 0.4f, 0.4f); // Ruins
    }
    else if (noiseValue < 0.3f) {
        return glm::vec3(0.6f, 0.4f, 0.2f); // Desolate plains
    }
    else {
        return glm::vec3(0.0f, 0.5f, 0.0f); // Forest
    }
}

void Terrain::generateTerrain(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    for (int z = 0; z <= gridSize; ++z) {
        for (int x = 0; x <= gridSize; ++x) {
            float noiseValue = noise.GetNoise((float)x, (float)z);
            float height = noiseValue * scale;

            vertices.push_back({ glm::vec3((float)x, height, (float)z), getBiomeColor(noiseValue) });
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
}
