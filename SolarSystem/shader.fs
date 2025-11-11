#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform bool isSun;

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    if(isSun)
    {
        // Matahari memancarkan cahaya
        FragColor = texColor;
    }
    else
    {
        // --- REVISI ---
        // Kita buat planet terlihat terang agar mudah dilihat
        // (Sebelumnya 0.3, yang membuatnya terlalu gelap)
        float ambient = 1.0; 
        FragColor = vec4(texColor.rgb * ambient, texColor.a);
    }
}