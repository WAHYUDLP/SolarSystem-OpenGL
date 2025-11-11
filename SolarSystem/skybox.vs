#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords; // Kita dapat ini dari Sphere

out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 view; // Ini akan jadi matrix rotasi saja

void main()
{
    TexCoords = aTexCoords; // Teruskan UV bawaan dari Sphere
    
    // Trik Skybox:
    // 1. Ubah posisi normal
    vec4 pos = projection * view * vec4(aPos, 1.0);
    
    // 2. Paksa Z agar = W.
    // Ini memastikan bahwa setelah "perspective divide" (z/w),
    // nilainya akan 1.0 (kedalaman maksimum).
    // Ini membuat skybox selalu di belakang objek lain.
    gl_Position = pos.xyww;
}