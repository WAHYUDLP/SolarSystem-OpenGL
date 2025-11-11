#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // UV dari Vertex Shader

uniform sampler2D skybox;

void main()
{
    // Cukup gambar tekstur galaksi menggunakan UV dari sphere
    FragColor = texture(skybox, TexCoords);
}