#pragma once
#include "Config.h"
#include "Kinematics.h"
#include <imgui.h>

class Gui {
public:
    Gui();
    ~Gui();
    void render();
    bool shouldClose() const { return m_shouldClose; }

private:
    void renderConfigPanel();
    void renderPlots();
    void renderAnimation();
    void render3DView(int currentIndex, int count, ImVec2 canvasPos, ImVec2 canvasSize);

    AppConfig m_config;
    SimulationResult m_result;
    bool m_shouldClose = false;
    bool m_hasResult = false;
    int m_mode = 0; // 0=PTP, 1=Pick&Place

    // Animation
    float m_animationTime = 0.0f;
    bool m_isPlaying = false;

    // 3D View
    float m_rotX = 0.45f;
    float m_rotZ = 0.78f;
    float m_zoom = 1.0f;
    bool m_isDragging3D = false;
};
