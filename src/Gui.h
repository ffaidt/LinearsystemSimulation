#pragma once
#include "Config.h"
#include "Kinematics.h"

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
    float m_animationTime = 0.0f;
    bool m_isPlaying = false;
    
    // Isometrische Pfad-Daten
    std::vector<float> m_isoU;
    std::vector<float> m_isoV;
};
