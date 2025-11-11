#include <glad/glad.h>
#include <GLFW/glfw3.h>

// --- REVISI --- Tambahkan define ini SEBELUM include glm
#define GLM_ENABLE_EXPERIMENTAL
// --- AKHIR REVISI ---

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp> // --- REVISI --- Untuk glm::mix (interpolasi)
#include <glm/gtx/compatibility.hpp> // --- REVISI --- Untuk glm::distance2

// --- REVISI --- Tambahkan header untuk ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// --- AKHIR REVISI ---

// Tambahkan ini untuk memuat stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

// --- REVISI --- Memindahkan definisi struct Planet ke ATAS ---
// --- Struktur Data Planet ---
struct Planet
{
    std::string name;
    std::string info;
    unsigned int textureID;
    float size;
    float orbitRadius;
    float orbitSpeedFactor;
    int objectID;

    Planet(std::string n, std::string i, unsigned int tex, float s, float o, float os, int id)
        : name(n), info(i), textureID(tex), size(s), orbitRadius(o), orbitSpeedFactor(os), objectID(id) {
    }
};
// --- AKHIR REVISI ---


// --- Deklarasi Global ---

// Pengaturan Jendela Awal
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// --- REVISI --- Konstanta untuk posisi kamera awal
const glm::vec3 ORBIT_CAMERA_POS = glm::vec3(0.0f, 40.0f, 120.0f);
const glm::vec3 ORBIT_CAMERA_FRONT = glm::vec3(0.0f, -0.25f, -1.0f);
// --- AKHIR REVISI ---

// Kamera
glm::vec3 cameraPos = ORBIT_CAMERA_POS;
glm::vec3 cameraFront = ORBIT_CAMERA_FRONT;
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = -15.0f;
float fov = 45.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Waktu
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float orbitSpeed = 0.1f;

// Picking (Deteksi Klik)
double mouseClickX, mouseClickY;
bool performPicking = false;

// --- Variabel Global untuk UI & Zoom ---
GLFWwindow* g_window = NULL;
bool isPaused = false;
bool spacePressed = false;
float totalTime = 0.0f;
const float g_uiScaleFactor = 2.0f;

Planet* g_selectedPlanet = NULL;
bool g_showInfoWindow = false;

// --- REVISI --- Variabel untuk Zoom Kamera ---
bool g_isCameraLocked = false;
glm::vec3 g_currentPlanetPosition = glm::vec3(0.0f);
bool g_isReturningToOrbit = false; // --- REVISI BARU --- Status kamera kembali
// --- AKHIR REVISI ---

int g_framebufferWidth = SCR_WIDTH;
int g_framebufferHeight = SCR_HEIGHT;


// --- Kelas Shader Sederhana ---
// (Tidak ada perubahan)
class Shader
{
public:
    unsigned int ID;
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            vShaderFile.close();
            fShaderFile.close();
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            if (g_window) {
                glfwSetWindowTitle(g_window, "ERROR: File shader (.vs/.fs) tidak ditemukan!");
            }
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        unsigned int vertex, fragment;
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    void use() { glUseProgram(ID); }
    void setBool(const std::string& name, bool value) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); }
    void setInt(const std::string& name, int value) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), value); }
    void setFloat(const std::string& name, float value) const { glUniform1f(glGetUniformLocation(ID, name.c_str()), value); }
    void setMat4(const std::string& name, const glm::mat4& mat) const { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]); }

private:
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

// --- Kelas Sphere Sederhana ---
// (Tidak ada perubahan)
class Sphere
{
public:
    unsigned int vao = 0;
    unsigned int vbo, ebo;
    unsigned int indexCount;

    Sphere() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow)
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = (unsigned int)indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    }

    void Draw()
    {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// --- REVISI --- Kelas baru untuk Cincin (sekarang geometris) ---
class Ring
{
public:
    // --- REVISI --- Hapus ebo, kita pakai glDrawArrays
    unsigned int vao = 0, vbo = 0;
    unsigned int vertexCount; // --- REVISI --- ganti nama dari indexCount

    // --- REVISI --- Constructor diganti total untuk membuat annulus
    Ring(int segments = 64) // Radius di-hardcode ke 1.0 (luar) dan 0.6 (dalam)
    {
        const float PI = 3.14159265359f;
        const float innerRadius = 0.6f; // Radius dalam (0.6 * skala)
        const float outerRadius = 1.0f; // Radius luar (1.0 * skala)

        std::vector<float> data; // Format: X, Y, Z, U, V

        for (int i = 0; i <= segments; ++i)
        {
            float angle = (float)i / (float)segments * 2.0f * PI;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);

            // --- Vertex Luar ---
            float outerX = cosA * outerRadius;
            float outerZ = sinA * outerRadius;
            data.push_back(outerX);             // pos.x
            data.push_back(0.0f);               // pos.y
            data.push_back(outerZ);             // pos.z
            // Petakan tex coord berdasarkan posisi (mengasumsikan tekstur .png menutupi -1..1)
            data.push_back(outerX * 0.5f + 0.5f); // tex.u (dari -1..1 -> 0..1)
            data.push_back(outerZ * 0.5f + 0.5f); // tex.v (dari -1..1 -> 0..1)

            // --- Vertex Dalam ---
            float innerX = cosA * innerRadius;
            float innerZ = sinA * innerRadius;
            data.push_back(innerX);             // pos.x
            data.push_back(0.0f);               // pos.y
            data.push_back(innerZ);             // pos.z
            data.push_back(innerX * 0.5f + 0.5f); // tex.u
            data.push_back(innerZ * 0.5f + 0.5f); // tex.v
        }

        // Kita punya (segments + 1) * 2 total vertex
        vertexCount = (segments + 1) * 2;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        // --- REVISI --- Tidak perlu EBO
        // glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);

        // --- REVISI --- Hapus EBO
        /*
        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };
        indexCount = 6;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        */


        // Atribut Posisi (layout = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        // Atribut TexCoord (layout = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindVertexArray(0);
    }

    void Draw()
    {
        glBindVertexArray(vao);
        // --- REVISSI --- Gunakan glDrawArrays dengan GL_TRIANGLE_STRIP
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
        // glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};
// --- AKHIR REVISI ---

// --- Kelas OrbitLine ---
// (Tidak ada perubahan)
class OrbitLine
{
public:
    unsigned int vao = 0, vbo = 0;
    int numSegments;

    OrbitLine(int segments = 100)
    {
        numSegments = segments;
        const float PI = 3.14159265359f;
        std::vector<float> vertices;

        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = 2.0f * PI * float(i) / float(numSegments);
            float x = cos(angle);
            float z = sin(angle);

            vertices.push_back(x);
            vertices.push_back(0.0f);
            vertices.push_back(z);
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    void Draw()
    {
        glBindVertexArray(vao);
        glDrawArrays(GL_LINE_LOOP, 0, numSegments + 1);
        glBindVertexArray(0);
    }
};

// --- Prototipe Fungsi ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);
// --- REVISI --- Tambahkan Ring& dan texture ID cincin
void renderScene(Shader& shader, Shader& pickingShader, Shader& orbitShader, Sphere& sphere, Ring& ring, OrbitLine& orbitLine, std::vector<Planet>& planets, Planet& sun, unsigned int texSaturnRing, bool isPicking);
// --- AKHIR REVISI ---
void processPicking(std::vector<Planet>& planets, Planet& sun);
void RenderUI();


// --- Main ---
int main()
{
    // Inisialisasi GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    // Buat Jendela
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    g_window = window;
    glfwSetWindowTitle(g_window, "Tata Surya | Klik Kanan: Info | Spasi: Pause/Play");

    // Setel Callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Inisialisasi GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Aktifkan Uji Kedalaman, MSAA, dan Blending
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND); // --- REVISI --- Pastikan blending aktif untuk cincin
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Buat Shader
    Shader mainShader("shader.vs", "shader.fs");
    Shader pickingShader("picking.vs", "picking.fs");
    Shader orbitShader("orbit.vs", "orbit.fs");

    // Buat Mesh
    Sphere sphere;
    Ring ring; // --- REVISI --- Buat objek cincin
    OrbitLine orbitLine;

    // --- Inisialisasi ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // --- Perbesar skala font dan UI
    io.FontGlobalScale = g_uiScaleFactor;
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScaleAllSizes(g_uiScaleFactor);

    // Atur ImGui untuk GLFW dan OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Muat Tekstur
    unsigned int texSun = loadTexture("sun.jpg");
    unsigned int texMercury = loadTexture("merkurius.jpg");
    unsigned int texVenus = loadTexture("venus.jpg");
    unsigned int texEarth = loadTexture("bumi.jpg");
    unsigned int texMars = loadTexture("mars.jpg");
    unsigned int texJupiter = loadTexture("jupiter.jpg");
    unsigned int texSaturn = loadTexture("saturnus.jpg");
    unsigned int texSaturnRing = loadTexture("saturnus_cincin.png"); // --- REVISI --- Muat tekstur cincin
    unsigned int texUranus = loadTexture("uranus.jpg");
    unsigned int texNeptune = loadTexture("neptun.jpg");

    // Data Tata Surya
    Planet sun("Matahari", "Bintang di pusat Tata Surya.", texSun, 3.5f, 0.0f, 0.0f, 1);

    std::vector<Planet> planets;
    planets.push_back(Planet("Merkurius", "Planet terkecil dan terdekat dengan Matahari.", texMercury, 0.5f, 10.0f, 4.7f, 2));
    planets.push_back(Planet("Venus", "Planet terpanas di Tata Surya.", texVenus, 0.9f, 15.0f, 3.5f, 3));
    planets.push_back(Planet("Bumi", "Satu-satunya planet yang diketahui memiliki kehidupan.", texEarth, 1.0f, 21.0f, 2.9f, 4));
    planets.push_back(Planet("Mars", "Planet merah, memiliki gunung tertinggi.", texMars, 0.7f, 28.0f, 2.4f, 5));
    planets.push_back(Planet("Jupiter", "Planet terbesar, memiliki Bintik Merah Raksasa.", texJupiter, 3.0f, 40.0f, 1.3f, 6));
    planets.push_back(Planet("Saturnus", "Dikenal karena sistem cincinnya.", texSaturn, 2.5f, 55.0f, 0.9f, 7));
    planets.push_back(Planet("Uranus", "Raksasa es yang berotasi miring.", texUranus, 1.5f, 68.0f, 0.6f, 8));
    planets.push_back(Planet("Neptunus", "Planet terjauh, raksasa es berwarna biru.", texNeptune, 1.4f, 80.0f, 0.5f, 9));


    // --- Render Loop Utama ---
    while (!glfwWindowShouldClose(window))
    {
        // Logika Waktu
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (!isPaused)
        {
            totalTime += deltaTime;
        }

        // Proses Input
        processInput(window);

        // --- Mulai Frame ImGui Baru
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- REVISI --- Logika Zoom Kamera ---
        float lerpSpeed = 5.0f * deltaTime; // Kecepatan gerak kamera

        if (g_isCameraLocked && g_selectedPlanet != NULL)
        {
            // --- LOGIKA ZOOM IN ---
            g_isReturningToOrbit = false; // Pastikan kita tidak mencoba kembali

            // Hitung posisi target di belakang planet
            float offsetDistance = g_selectedPlanet->size * 5.0f + 5.0f; // Jarak dari planet
            glm::vec3 upOffset = glm::vec3(0.0f, g_selectedPlanet->size * 2.0f, 0.0f); // Sedikit di atas

            // Dapatkan arah dari matahari (pusat) ke planet
            glm::vec3 directionFromSun = (g_currentPlanetPosition == glm::vec3(0.0f)) ?
                glm::vec3(0.0f, 0.0f, 1.0f) : // Jika matahari, lihat dari Z
                glm::normalize(g_currentPlanetPosition);

            glm::vec3 targetPos = g_currentPlanetPosition + (directionFromSun * offsetDistance) + upOffset;

            // Hitung arah "depan" target (melihat ke planet)
            glm::vec3 targetFront = glm::normalize(g_currentPlanetPosition - targetPos);

            // Interpolasi (Lerp) kamera ke posisi dan arah target
            cameraPos = glm::mix(cameraPos, targetPos, lerpSpeed);
            cameraFront = glm::normalize(glm::mix(cameraFront, targetFront, lerpSpeed));
        }
        else if (g_isReturningToOrbit)
        {
            // --- LOGIKA ZOOM OUT ---
            // Interpolasi (Lerp) kamera KEMBALI ke posisi orbit awal
            cameraPos = glm::mix(cameraPos, ORBIT_CAMERA_POS, lerpSpeed);
            cameraFront = glm::normalize(glm::mix(cameraFront, ORBIT_CAMERA_FRONT, lerpSpeed));

            // Cek jika sudah dekat dengan posisi awal
            if (glm::distance2(cameraPos, ORBIT_CAMERA_POS) < 0.1f)
            {
                cameraPos = ORBIT_CAMERA_POS; // Paskan ke posisi awal
                cameraFront = ORBIT_CAMERA_FRONT;
                g_isReturningToOrbit = false; // Selesai kembali
            }
        }
        // --- AKHIR REVISI ---

        // --- Logika Picking (jika diminta) ---
        if (performPicking)
        {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // --- REVISI --- Tambahkan ring, texSaturnRing
            renderScene(mainShader, pickingShader, orbitShader, sphere, ring, orbitLine, planets, sun, texSaturnRing, true);

            processPicking(planets, sun);

            performPicking = false;
        }

        // --- Render Normal ---
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- REVISI --- Tambahkan ring, texSaturnRing
        renderScene(mainShader, pickingShader, orbitShader, sphere, ring, orbitLine, planets, sun, texSaturnRing, false);

        // --- Render UI di atas segalanya
        RenderUI();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Shutdown ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Selesai
    glfwTerminate();
    return 0;
}


// --- Implementasi Fungsi ---

// --- REVISI --- Tambahkan Ring& ring, unsigned int texSaturnRing
void renderScene(Shader& mainShader, Shader& pickingShader, Shader& orbitShader, Sphere& sphere, Ring& ring, OrbitLine& orbitLine, std::vector<Planet>& planets, Planet& sun, unsigned int texSaturnRing, bool isPicking)
{
    Shader& activeShader = isPicking ? pickingShader : mainShader;
    activeShader.use();

    // --- REVISI --- Kita perlu menghitung view matrix secara manual di sini
    // karena cameraPos dan cameraFront di-update oleh logika zoom
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)g_framebufferWidth / (float)g_framebufferHeight, 0.1f, 200.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    // --- AKHIR REVISI ---

    activeShader.setMat4("projection", projection);
    activeShader.setMat4("view", view);

    // --- Gambar Matahari ---
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(sun.size));
    activeShader.setMat4("model", model);

    if (g_isCameraLocked && g_selectedPlanet && g_selectedPlanet->objectID == sun.objectID)
    {
        g_currentPlanetPosition = glm::vec3(0.0f);
    }

    if (isPicking) {
        activeShader.setInt("objectID", sun.objectID);
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sun.textureID);
        activeShader.setInt("texture_diffuse1", 0);
        activeShader.setBool("isSun", true);
    }
    sphere.Draw();


    // --- Gambar Planet-Planet ---
    if (!isPicking) {
        activeShader.setBool("isSun", false);
    }

    for (const auto& planet : planets)
    {
        float time = totalTime;
        float orbitX = sin(time * orbitSpeed * planet.orbitSpeedFactor) * planet.orbitRadius;
        float orbitZ = cos(time * orbitSpeed * planet.orbitSpeedFactor) * planet.orbitRadius;

        glm::vec3 currentPos = glm::vec3(orbitX, 0.0f, orbitZ);

        if (g_isCameraLocked && g_selectedPlanet && g_selectedPlanet->objectID == planet.objectID)
        {
            g_currentPlanetPosition = currentPos;
        }

        // --- Gambar Planet ---
        model = glm::mat4(1.0f);
        model = glm::translate(model, currentPos);
        model = glm::scale(model, glm::vec3(planet.size));
        activeShader.setMat4("model", model);

        if (isPicking) {
            activeShader.setInt("objectID", planet.objectID);
        }
        else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, planet.textureID);
            activeShader.setInt("texture_diffuse1", 0);
        }

        sphere.Draw();

        // --- REVISI --- Gambar Cincin Saturnus ---
        if (planet.name == "Saturnus" && !isPicking)
        {
            // Matikan penulisan ke depth buffer agar transparansi bekerja
            glDepthMask(GL_FALSE);

            // Gunakan tekstur cincin
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texSaturnRing);
            activeShader.setInt("texture_diffuse1", 0);

            // Buat model matrix baru untuk cincin
            glm::mat4 modelRing = glm::mat4(1.0f);
            modelRing = glm::translate(modelRing, currentPos); // Posisi sama dengan planet
            //// Miringkan cincin sedikit
            //modelRing = glm::rotate(modelRing, glm::radians(25.0f), glm::normalize(glm::vec3(0.5f, 0.0f, 0.5f)));
            //// Buat skala cincin lebih besar dari planet
            //modelRing = glm::scale(modelRing, glm::vec3(planet.size * 2.2f));
            //activeShader.setMat4("model", modelRing);
            // Miringkan cincin sedikit
            modelRing = glm::rotate(modelRing, glm::radians(25.0f), glm::normalize(glm::vec3(0.5f, 0.0f, 0.5f)));
            // --- REVISI --- Buat skala cincin sedikit lebih kecil
            modelRing = glm::scale(modelRing, glm::vec3(planet.size * 1.8f)); // Sebelumnya 2.2f
            activeShader.setMat4("model", modelRing);

            // Gambar cincin
            ring.Draw();

            // Nyalakan kembali penulisan ke depth buffer
            glDepthMask(GL_TRUE);
        }
        // --- AKHIR REVISI ---
    }

    // --- Gambar Garis Orbit ---
    if (!isPicking)
    {
        orbitShader.use();
        orbitShader.setMat4("projection", projection);
        orbitShader.setMat4("view", view);

        for (const auto& planet : planets)
        {
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(planet.orbitRadius));
            orbitShader.setMat4("model", model);
            orbitLine.Draw();
        }
    }
}

// --- Fungsi UI Baru ---
void RenderUI()
{
    ImGuiIO& io = ImGui::GetIO();
    float padding = 10.0f * g_uiScaleFactor;

    // Jendela Info Planet (pojok kanan atas, stagnan)
    if (g_showInfoWindow && g_selectedPlanet != NULL)
    {
        // --- REVISI --- Ukuran disesuaikan sedikit untuk teks baru
        ImGui::SetNextWindowSize(ImVec2(500, 260), ImGuiCond_FirstUseEver);

        ImVec2 infoWindowPos = ImVec2(io.DisplaySize.x - padding, padding);
        ImGui::SetNextWindowPos(infoWindowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));

        ImGuiWindowFlags infoWindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("Informasi Planet", &g_showInfoWindow, infoWindowFlags);

        ImGui::Text("Nama: %s", g_selectedPlanet->name.c_str());
        ImGui::Separator();

        ImGui::TextWrapped("Info: %s", g_selectedPlanet->info.c_str());

        // --- REVISI DARI PERCAKAPAN SEBELUMNYA ---
        // Teks bantuan untuk melepas fokus
        if (g_isCameraLocked)
        {
            ImGui::Separator();
            ImGui::TextWrapped("Tutup jendela ini (X) untuk melepas fokus.");
        }
        // --- AKHIR REVISI ---

        ImGui::End();
    }

    // --- REVISI --- Jika pengguna menutup jendela info, lepas kunci kamera
    if (!g_showInfoWindow)
    {
        g_selectedPlanet = NULL;
        if (g_isCameraLocked) // Hanya jika sedang terkunci
        {
            g_isCameraLocked = false;
            g_isReturningToOrbit = true; // --- REVISI --- Mulai kembali ke orbit
        }
    }
    // --- AKHIR REVISI ---

    // Jendela Kontrol Simulasi (pojok kiri atas, bisa digeser)
    ImGui::SetNextWindowSize(ImVec2(500, 320), ImGuiCond_FirstUseEver);

    ImVec2 controlWindowPos = ImVec2(padding, padding);
    ImGui::SetNextWindowPos(controlWindowPos, ImGuiCond_FirstUseEver);

    ImGui::Begin("Kontrol Simulasi");
    ImGui::Text("Status: %s", isPaused ? "Dijeda" : "Berjalan");
    if (ImGui::Button(isPaused ? "Lanjutkan (Spasi)" : "Jeda (Spasi)"))
    {
        isPaused = !isPaused;
    }

    ImGui::Separator();
    ImGui::TextWrapped("Tombol Panah Atas/Bawah untuk kecepatan orbit.");
    ImGui::TextWrapped("Klik Kanan pada planet untuk info.");
    ImGui::TextWrapped("Klik Kiri + Geser Mouse untuk rotasi kamera.");
    ImGui::TextWrapped("WASD untuk gerak kamera.");
    ImGui::TextWrapped("Scroll wheel untuk zoom.");

    ImGui::End();
}


// --- processPicking sekarang mengatur kunci kamera
void processPicking(std::vector<Planet>& planets, Planet& sun)
{
    glFlush();
    glFinish();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned char data[4];
    glReadPixels((int)mouseClickX, g_framebufferHeight - 1 - (int)mouseClickY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

    int pickedID = data[0] + data[1] * 256 + data[2] * 256 * 256;

    g_selectedPlanet = NULL;
    g_showInfoWindow = false;
    g_isCameraLocked = false;

    if (pickedID == 0)
    {
        g_isReturningToOrbit = true; // --- REVISI --- Kembali ke orbit jika klik angkasa
        return;
    }
    else if (pickedID == sun.objectID)
    {
        g_selectedPlanet = &sun;
        g_showInfoWindow = true;
        g_isCameraLocked = true;
        g_isReturningToOrbit = false; // --- REVISI --- Zoom IN, jangan zoom out
    }
    else
    {
        for (auto& planet : planets)
        {
            if (planet.objectID == pickedID)
            {
                g_selectedPlanet = &planet;
                g_showInfoWindow = true;
                g_isCameraLocked = true;
                g_isReturningToOrbit = false; // --- REVISI --- Zoom IN, jangan zoom out
                break;
            }
        }
    }
}


// Fungsi untuk memuat tekstur
// (Tidak ada perubahan)
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA; // --- REVISI --- Ini penting untuk cincin (file .png)
        else {
            std::cout << "Format gambar tidak didukung untuk " << path << std::endl;
            stbi_image_free(data);
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Gagal memuat tekstur di: " << path << std::endl;
        if (g_window) {
            std::string errorTitle = "ERROR: Gagal memuat file gambar: " + std::string(path);
            glfwSetWindowTitle(g_window, errorTitle.c_str());
        }
    }

    return textureID;
}

// --- Fungsi Callback ---

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool isMoving = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (isMoving && g_isCameraLocked)
    {
        g_isCameraLocked = false;
        g_showInfoWindow = false;
        g_isReturningToOrbit = true; // --- REVISI --- Mulai kembali ke orbit
    }

    // Kontrol Kamera (WASD) - Hanya bergerak jika tidak terkunci ATAU sedang kembali
    float cameraSpeed = 2.5f * deltaTime * 10.0f;
    if (!g_isCameraLocked) // --- REVISI --- Izinkan WASD jika tidak terkunci (termasuk saat kembali)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        // Jika pengguna bergerak saat kembali, batalkan kembalinya
        if (isMoving && g_isReturningToOrbit)
            g_isReturningToOrbit = false;
    }


    // Kontrol Kecepatan Orbit (PANAH ATAS/BAWAH)
    float speedChangeRate = 0.1f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        orbitSpeed += speedChangeRate * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        orbitSpeed -= speedChangeRate * deltaTime;
    if (orbitSpeed < 0.0f) orbitSpeed = 0.0f;

    // Logika Tombol Pause (Spasi)
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed)
    {
        isPaused = !isPaused;
        spacePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        spacePressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    g_framebufferWidth = width;
    g_framebufferHeight = height;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        firstMouse = true;
        return;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        // --- REVISI --- Lepas kunci kamera jika pengguna mengklik & menyeret
        if (g_isCameraLocked)
        {
            g_isCameraLocked = false;
            g_showInfoWindow = false;
            g_isReturningToOrbit = true; // --- REVISI --- Mulai kembali ke orbit
        }
        // Jika sedang kembali, biarkan pengguna mengambil alih
        if (g_isReturningToOrbit)
        {
            g_isReturningToOrbit = false;
        }
        // --- AKHIR REVISI ---


        if (firstMouse)
        {
            lastX = (float)xpos;
            lastY = (float)ypos;
            firstMouse = false;
        }

        float xoffset = (float)xpos - lastX;
        float yoffset = lastY - (float)ypos;
        lastX = (float)xpos;
        lastY = (float)ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
    else
    {
        firstMouse = true;
    }

    mouseClickX = xpos;
    mouseClickY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 60.0f)
        fov = 60.0f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            performPicking = true;
        }
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        performPicking = true;
    }
}