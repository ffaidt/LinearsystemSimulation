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
    void renderPickPopup();
    void renderPickPreview3D(ImVec2 canvasPos, ImVec2 canvasSize);

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

    // Pick-Positionen Popup
    bool m_showPickPopup = false;
    Vec3 m_firstPick = {0, 0, 50};
    Vec3 m_lastPick = {100, 0, -50};
    float m_pitchHoriz = 25.0f;  // Abstand horizontal [mm]
    float m_pitchZ = 25.0f;      // Abstand Z (Ebenen) [mm]
    std::vector<Vec3> m_previewPicks;
    bool m_previewReady = false;
    float m_prevRotX = 0.45f;
    float m_prevRotZ = 0.78f;
    float m_prevZoom = 1.0f;
    char m_detectedPlane[64] = "";
};
