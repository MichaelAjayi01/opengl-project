#include "Sword.h"
#include <random>
#include <filesystem>
#include <glew.h>
#include <stb_image.h>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <assimp/Importer.hpp> // Include Assimp importer header
#include <assimp/scene.h> // Include Assimp scene header
#include <assimp/postprocess.h> // Include Assimp postprocess header

Sword::Sword(const std::string& modelPath1, const std::string& modelPath2) {
    loadSwordModel(modelPath1, importer1, swordScene1, textureID1);
    loadSwordModel(modelPath2, importer2, swordScene2, textureID2);
}

void Sword::loadSwordModel(const std::string& filePath, Assimp::Importer& importer, const aiScene*& scene, GLuint& textureID) {
    scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
    } else {
        std::cout << "Successfully loaded model: " << filePath << std::endl;
        std::string texturePath = std::filesystem::current_path().string() + "/models/Swords/texture/Texture_MAp_sword.png";
        textureID = loadTexture(texturePath);
    }
}


void Sword::scatterSwords(int numSwords, int gridSize, float scale, float scaleFactor, float offset, FastNoiseLite& noise, std::vector<glm::mat4>& swordTransforms1, std::vector<glm::mat4>& swordTransforms2) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(0, gridSize);
    std::uniform_int_distribution<> zDist(0, gridSize);
    std::uniform_real_distribution<float> angleDist(-10.0f, 10.0f); // Random tilt angle between -10 and 10 degrees
    std::uniform_real_distribution<float> rotationDist(0.0f, 360.0f); // Random rotation angle between 0 and 360 degrees

    for (int i = 0; i < numSwords; ++i) {
        int x = xDist(gen);
        int z = zDist(gen);

        float noiseValue = noise.GetNoise((float)x, (float)z);
        float terrainHeight = noiseValue * scale;

        // Adjust the y-coordinate to embed the sword into the terrain with an offset
        float y = terrainHeight + offset;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        transform = glm::rotate(transform, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate to make the blade point downwards
        transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate around y-axis

        // Apply random tilt and rotation
        float tiltAngleX = angleDist(gen);
        float tiltAngleZ = angleDist(gen);
        float rotationAngleY = rotationDist(gen);

        transform = glm::rotate(transform, glm::radians(tiltAngleX), glm::vec3(1.0f, 0.0f, 0.0f)); // Random tilt around x-axis
        transform = glm::rotate(transform, glm::radians(rotationAngleY), glm::vec3(0.0f, 1.0f, 0.0f)); // Random rotation around y-axis
        transform = glm::rotate(transform, glm::radians(tiltAngleZ), glm::vec3(0.0f, 0.0f, 1.0f)); // Random tilt around z-axis

        transform = glm::scale(transform, glm::vec3(scaleFactor)); // Apply scaling

        // Alternate between sword models
        if (i % 2 == 0) {
            swordTransforms1.push_back(transform);
        } else {
            swordTransforms2.push_back(transform);
        }
    }
}


void Sword::renderSwords(const std::vector<glm::mat4>& swordTransforms1, const std::vector<glm::mat4>& swordTransforms2, GLuint shaderProgram) {
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint textureLoc = glGetUniformLocation(shaderProgram, "texture1");

    // Render first sword model
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID1);
    glUniform1i(textureLoc, 0);  // Set the texture uniform to use texture unit 0

    for (const auto& transform : swordTransforms1) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

        for (unsigned int i = 0; i < swordScene1->mNumMeshes; i++) {
            aiMesh* mesh = swordScene1->mMeshes[i];

            // Generate buffers for mesh data
            std::vector<float> vertices;
            std::vector<unsigned int> indices;

            // Add positions, UVs, and normals to the vertex array
            for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                aiVector3D pos = mesh->mVertices[j];
                aiVector3D uv = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][j] : aiVector3D(0.0f, 0.0f, 0.0f);  // Handle missing UVs

                // Add position
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);

                // Add UV coordinates
                vertices.push_back(uv.x);
                vertices.push_back(uv.y);

                // Add normal
                aiVector3D normal = mesh->mNormals[j];
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
            }

            // Add indices
            for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }

            GLuint VAO, VBO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Set up position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Set up UV attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // Set up normal attribute
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }
    }

    // Render second sword model
    glBindTexture(GL_TEXTURE_2D, textureID2);
    glUniform1i(textureLoc, 0);  // Set the texture uniform to use texture unit 0

    for (const auto& transform : swordTransforms2) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

        for (unsigned int i = 0; i < swordScene2->mNumMeshes; i++) {
            aiMesh* mesh = swordScene2->mMeshes[i];

            // Generate buffers for mesh data
            std::vector<float> vertices;
            std::vector<unsigned int> indices;

            // Add positions, UVs, and normals to the vertex array
            for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
                aiVector3D pos = mesh->mVertices[j];
                aiVector3D uv = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][j] : aiVector3D(0.0f, 0.0f, 0.0f);  // Handle missing UVs

                // Add position
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);

                // Add UV coordinates
                vertices.push_back(uv.x);
                vertices.push_back(uv.y);

                // Add normal
                aiVector3D normal = mesh->mNormals[j];
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
            }

            // Add indices
            for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
                aiFace face = mesh->mFaces[j];
                for (unsigned int k = 0; k < face.mNumIndices; k++) {
                    indices.push_back(face.mIndices[k]);
                }
            }

            GLuint VAO, VBO, EBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Set up position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // Set up UV attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // Set up normal attribute
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }
    }
}
GLuint Sword::loadTexture(const std::string& texturePath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
    }

    stbi_image_free(data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}
