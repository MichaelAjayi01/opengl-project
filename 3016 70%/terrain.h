#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <FastNoiseLite.h>
#include "Vertex.h"
#include <glm.hpp>

class Terrain {
public:
    Terrain(int gridSize, float scale, FastNoiseLite& noise);
    void generateTerrain(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    float getHeightAt(float x, float z) const;

private:
    int gridSize;
    float scale;
    FastNoiseLite& noise;
    glm::vec3 getBiomeColor(float noiseValue);
    std::vector<Vertex> vertices; // Store vertices for height lookup
};

#endif // TERRAIN_H
