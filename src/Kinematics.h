#pragma once
#include "Config.h"
#include <vector>

// Zustand zu einem bestimmten Zeitpunkt
struct MotionState {
    float t; // Zeit
    float s; // Position
    float v; // Geschwindigkeit
    float a; // Beschleunigung
};

// Verlauf (Trajektorie) für eine Achse über die Zeit, optimiert für ImPlot
struct AxisTrajectory {
    std::vector<float> t;
    std::vector<float> s;
    std::vector<float> v;
    std::vector<float> a;
};

// Gesamtergebnis der Simulation
struct SimulationResult {
    AxisTrajectory x;
    AxisTrajectory y;
    AxisTrajectory z;
    
    // Die Gesamtzeit der Simulation (für das Rendern der X-Achse im Plot)
    float totalTime = 0.0f;
};

class Kinematics {
public:
    // Hauptfunktion: Berechnet die gesamte PTP-Trajektorie
    static SimulationResult calculate(const AppConfig& config, float dt = 0.01f);

private:
    // Berechnet die Parameter eines 1D-Trapezprofils
    static void calcProfile(float distance, float v_max, float a_max, 
                            float& t_a, float& t_cruise, float& v_max_actual);

    // Wertet das Profil an einem bestimmten Zeitpunkt t aus
    static MotionState evalProfile(float t, float start_pos, float distance, 
                                   float t_a, float t_cruise, float v_max_actual, float a_max);
};
