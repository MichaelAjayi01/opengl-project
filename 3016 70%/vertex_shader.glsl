#version 460 core

layout(location = 0) in vec3 position;  // Vertex position
layout(location = 1) in vec3 color;     // Vertex color
layout(location = 2) in vec3 normal;    // Vertex normal
layout(location = 3) in vec2 texCoord;  // Vertex texture coordinates

out vec3 vertexColor;  // Pass color to fragment shader
out vec3 fragNormal;   // Pass normal to fragment shader
out vec2 TexCoord;     // Pass texture coordinates to fragment shader

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vertexColor = color;  // Pass the color
    fragNormal = normalize(mat3(transpose(inverse(model))) * normal); // Transform normal correctly
    TexCoord = texCoord;  // Pass the texture coordinates to fragment shader
}
