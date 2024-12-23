#version 460 core

in vec3 vertexColor;  // Incoming vertex color
in vec3 fragNormal;   // Incoming normal
in vec2 TexCoord;     // Incoming texture coordinates

out vec4 FragColor;   // Final color output

uniform sampler2D texture1;  // Texture sampler

void main() {
    vec3 ambientLight = vec3(0.1, 0.1, 0.1);  // Basic ambient light
    vec3 lightDir = normalize(vec3(0.0, 1.0, -1.0));  // Simple directional light
    float diff = max(dot(fragNormal, lightDir), 0.0);
    
    vec3 diffuse = diff * vertexColor;  // Diffuse shading effect
    vec3 color = ambientLight + diffuse;  // Final color (ambient + diffuse)

    // Sample the texture and blend it with the color
    vec4 textureColor = texture(texture1, TexCoord);  // Sample texture
    FragColor = vec4(color * textureColor.rgb, textureColor.a);  // Combine texture color with lighting color
}
