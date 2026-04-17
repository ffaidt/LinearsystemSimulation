#pragma once
#include "Config.h"
#include "Kinematics.h"
#include <imgui.h>

class Gui {
public:
    Gui();
    ~Gui();

    // Rendert einen GUI-Frame
    void render();

    // Gibt zurück, ob die Anwendung geschlossen werden soll
    bool shouldClose() const { return m_shouldClose; }

private:
    void renderConfigPanel();
    void renderPlots();

    AppConfig m_config;
    SimulationResult m_result;
    bool m_shouldClose = false;
    bool m_hasResult = false;
    
    // Animations-State
    void renderAnimation();
    void render3DView(int currentIndex, int count, ImVec2 canvasPos, ImVec2 canvasSize);
    float m_animationTime = 0.0f;
    bool m_isPlaying = false;
    
    // 3D-Rotation und Zoom (per Maus)
    float m_rotX = 0.45f;  // Elevation (~25°)
    float m_rotZ = 0.78f;  // Azimut (~45°)
    float m_zoom = 1.0f;   // Zoom-Faktor (Scrollrad)
    bool m_isDragging3D = false;
};
