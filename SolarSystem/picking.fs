#version 330 core
out vec4 FragColor;

uniform int objectID;

void main()
{
    // Kita menggambar planet dengan warna solid unik berdasarkan ID-nya
    // ID 0 = Latar belakang (hitam)
    // ID 1 = Matahari
    // ID 2 = Merkurius, dst.
    int r = (objectID & 0x000000FF) >> 0;
    int g = (objectID & 0x0000FF00) >> 8;
    int b = (objectID & 0x00FF0000) >> 16;

    FragColor = vec4(r / 255.0, g / 255.0, b / 255.0, 1.0);
}