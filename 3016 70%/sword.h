#pragma once
#ifndef SWORD_H
#define SWORD_H

#include <vector>
#include <string>
#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <FastNoiseLite.h>
#include <glew.h> // Include GLEW for GLuint

class Sword {
public:
    Sword(const std::string& modelPath);
    void scatterSwords(int numSwords, int gridSize, float scale, float scaleFactor, float offset, FastNoiseLite& noise, std::vector<glm::mat4>& swordTransforms);
    void renderSwords(const std::vector<glm::mat4>& swordTransforms, GLuint shaderProgram);

private:
    void loadSwordModel(const std::string& filePath);
    GLuint loadTexture(const std::string& texturePath);

    Assimp::Importer importer;
    const aiScene* swordScene;
    GLuint textureID;
};

#endif // SWORD_H
