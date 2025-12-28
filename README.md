# 🪐 Solar System Simulation - OpenGL

[![C++](https://img.shields.io/badge/Language-C++-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/Graphics-OpenGL-orange.svg)](https://www.opengl.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Simulasi sistem tata surya interaktif berbasis 3D yang dibangun menggunakan **C++** dan **OpenGL**. Proyek ini menampilkan visualisasi planet dengan tekstur realistik, kalkulasi orbit yang akurat, serta antarmuka pengguna (UI) untuk kontrol simulasi secara real-time.

---

## 🚀 Fitur Utama

* **Visualisasi Planet 3D:** Render objek bola (sphere) dengan pemetaan tekstur (texture mapping) untuk setiap planet dan matahari.
* **Sistem Orbit Dinamis:** Simulasi pergerakan planet mengelilingi matahari dengan kecepatan orbit yang dapat disesuaikan.
* **Antarmuka Kontrol (ImGui):** Panel kontrol untuk mengatur kecepatan simulasi, status jeda (pause), dan informasi detail planet.
* **Interaksi Kamera:** Dukungan kamera bebas (WASD) dan sistem *picking* untuk fokus pada planet tertentu dengan klik kanan.
* **Skybox Galaksi:** Latar belakang ruang angkasa yang imersif menggunakan teknik skybox.
* **Cincin Planet:** Render geometris khusus untuk cincin Saturnus menggunakan transparansi (alpha blending).

---

## 🛠️ Stack Teknologi

* **Bahasa Pemrograman:** C++
* **Graphics API:** OpenGL 3.3 (Core Profile)
* **Windowing & Input:** GLFW
* **OpenGL Loader:** GLAD
* **Matematika:** GLM (OpenGL Mathematics)
* **User Interface:** Dear ImGui
* **Texture Loading:** stb_image

---

## 📂 Struktur Proyek

```text
SolarSystem/
├── aset/               # Tekstur planet (.jpg, .png) dan font
├── SolarSystem.cpp      # Logika utama aplikasi
├── shader.vs / .fs     # Shader untuk objek utama
├── skybox.vs / .fs     # Shader untuk latar belakang galaksi
├── picking.vs / .fs    # Shader untuk deteksi klik objek
├── orbit.vs / .fs      # Shader untuk garis lintasan orbit
└── imgui.ini           # Konfigurasi layout UI
