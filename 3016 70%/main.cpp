#include <iostream>
#include <vector>
#include <random>
#include <glew.h>
#include <glfw3.h>
#include <FastNoiseLite.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "shaders/LoadShaders.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Vertex structure for the terrain
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

// Function to get biome colors
glm::vec3 getBiomeColor(float noiseValue) {
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

// Function to generate terrain
void generateTerrain(int gridSize, float scale, FastNoiseLite& noise, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
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

GLuint loadTexture(const std::string& texturePath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    int width, height, nrChannels;
    // Use STB_image to load the texture
    unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
    }

    stbi_image_free(data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}




// Function to load sword model
void loadSwordModel(const std::string& filePath, Assimp::Importer& importer, const aiScene*& scene) {
    scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
    }
    else {
        std::cout << "Successfully loaded model: " << filePath << std::endl;
    }
}


void scatterSwords(int numSwords, int gridSize, float scale, float scaleFactor, FastNoiseLite& noise, std::vector<glm::mat4>& swordTransforms) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(0, gridSize);
    std::uniform_int_distribution<> zDist(0, gridSize);

    for (int i = 0; i < numSwords; ++i) {
        int x = xDist(gen);
        int z = zDist(gen);

        float noiseValue = noise.GetNoise((float)x, (float)z);
        float y = noiseValue * scale;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Embed blade into the ground
        transform = glm::scale(transform, glm::vec3(scaleFactor)); // Apply scaling

        swordTransforms.push_back(transform);
    }
}
// Render sword models with debug logs for textures
void renderSwords(const std::vector<glm::mat4>& swordTransforms, GLuint shaderProgram, const aiScene* scene) {
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint textureLoc = glGetUniformLocation(shaderProgram, "texture1");

    // Path to your textures
    std::string texturePath = std::filesystem::current_path().string() + "/models/Swords/texture/Texture_MAp_sword.png";
    GLuint textureID = loadTexture(texturePath);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(textureLoc, 0);  // Set the texture uniform to use texture unit 0

    for (const auto& transform : swordTransforms) {
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[i];

            // Debug vertex and index information
            std::cout << "[Info] Rendering Mesh " << i << ", Vertices: " << mesh->mNumVertices << std::endl;

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

            std::cout << "[Info] Mesh " << i << " rendered successfully." << std::endl;
        }
    }
}


int main() {
    // GLFW and GLEW Initialization
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Procedural Terrain", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Shader setup
    ShaderInfo shaders[] = {
        { GL_VERTEX_SHADER, "shaders/vertex_shader.glsl" },
        { GL_FRAGMENT_SHADER, "shaders/fragment_shader.glsl" },
        { GL_NONE, NULL }
    };
    GLuint shaderProgram = LoadShaders(shaders);
    glUseProgram(shaderProgram);

    // Terrain generation
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.05f);

    int gridSize = 100;
    float scale = 5.0f;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    generateTerrain(gridSize, scale, noise, vertices, indices);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(50.0f, 50.0f, 150.0f), glm::vec3(50.0f, 0.0f, 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 500.0f);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Sword scattering
    Assimp::Importer importer;
    const aiScene* swordScene = nullptr;
    std::string modelPath = std::filesystem::current_path().string() + "/models/Swords/fbx/_sword_1.fbx";
    loadSwordModel(modelPath, importer, swordScene);

    std::vector<glm::mat4> swordTransforms;
    float swordScaleFactor = 0.5f; // Example scale factor
    scatterSwords(5, gridSize, scale, swordScaleFactor, noise, swordTransforms);

    // Main rendering loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        renderSwords(swordTransforms, shaderProgram, swordScene);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
