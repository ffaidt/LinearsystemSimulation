#include "Gui.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>

int main() {
    // 1. GLFW Initialisieren
    if (!glfwInit()) {
        std::cerr << "Fehler beim Initialisieren von GLFW\n";
        return -1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 2. Fenster erstellen
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Linearsystem Simulation", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync aktivieren

    // 3. ImGui und ImPlot Kontexte erstellen
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Tastatur-Navigation
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Docking aktivieren

    ImGui::StyleColorsDark();

    // 4. Backends initialisieren
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 5. Hauptschleife
    {
        Gui app; // GUI und Anwendungslogik

        while (!glfwWindowShouldClose(window) && !app.shouldClose()) {
            glfwPollEvents();

            // Neuen Frame starten
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Vollbild-Dockspace
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

            // Anwendungs-UI rendern
            app.render();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.15f, 0.2f, 1.0f); // Dunkelblauer Hintergrund
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }

    // 6. Aufräumen
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
