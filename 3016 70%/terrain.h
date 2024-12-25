#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <FastNoiseLite.h>
#include "Vertex.h"

class Terrain {
public:
    Terrain(int gridSize, float scale, FastNoiseLite& noise);
    void generateTerrain(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    glm::vec3 getBiomeColor(float noiseValue);

private:
    int gridSize;
    float scale;
    FastNoiseLite& noise;
};

#endif // TERRAIN_H
