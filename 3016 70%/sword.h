#pragma once
#ifndef SWORD_H
#define SWORD_H

#include <vector>
#include <string>
#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <FastNoiseLite.h>
#include <glew.h>

class Sword {
public:
    Sword(const std::string& modelPath1, const std::string& modelPath2);
    void scatterSwords(int numSwords, int gridSize, float scale, float scaleFactor, float offset, FastNoiseLite& noise, std::vector<glm::mat4>& swordTransforms1, std::vector<glm::mat4>& swordTransforms2);
    void renderSwords(const std::vector<glm::mat4>& swordTransforms1, const std::vector<glm::mat4>& swordTransforms2, GLuint shaderProgram);

private:
    void loadSwordModel(const std::string& filePath, Assimp::Importer& importer, const aiScene*& scene, GLuint& textureID);
    GLuint loadTexture(const std::string& texturePath);

    Assimp::Importer importer1;
    Assimp::Importer importer2;
    const aiScene* swordScene1;
    const aiScene* swordScene2;
    GLuint textureID1;
    GLuint textureID2;
};

#endif // SWORD_H
