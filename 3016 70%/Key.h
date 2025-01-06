#ifndef KEY_H
#define KEY_H

#include <glm.hpp>
#include <vector>
#include <string>
#include <assimp/scene.h>
#include <glew.h>

class Key {
public:
    Key(const std::string& modelPath);
    void render(const glm::mat4& view, const glm::mat4& projection, GLuint shaderProgram);
    void addKeyTransform(const glm::mat4& transform);

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    void processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<glm::mat4> keyTransforms;
    const aiScene* keyScene;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;
};

#endif // KEY_H
