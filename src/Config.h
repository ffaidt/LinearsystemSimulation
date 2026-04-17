#pragma once
#include <string>
#include <vector>

// 3D-Vektor
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// Konfiguration für eine einzelne Achse
struct AxisConfig {
    float start = 0.0f;
    float target = 100.0f;
    float v_max = 50.0f;
    float a_max = 20.0f;
};

// Pick & Place Konfiguration
struct PickPlaceConfig {
    std::vector<Vec3> pickPositions = { {0.0f, 0.0f, 0.0f} };
    Vec3 placePosition = {100.0f, 100.0f, 0.0f};
    float safeZ = 100.0f;          // Sichere Z-Höhe [mm]
    float dwellTime = 0.5f;        // Verweildauer [s]
    float softDistance = 50.0f;    // Sanfte Rampe Strecke [mm]
    float softAccel = 5.0f;        // Sanfte Rampe Beschleunigung [mm/s²]
    float vmax_x = 50.0f;
    float vmax_y = 50.0f;
    float vmax_z = 50.0f;
    float amax_x = 20.0f;
    float amax_y = 20.0f;
    float amax_z = 20.0f;
};

// Gesamtkonfiguration der App
struct AppConfig {
    AxisConfig x;
    AxisConfig y;
    AxisConfig z;
    PickPlaceConfig pp;
};

class ConfigManager {
public:
    // Lädt die Konfiguration aus einer JSON-Datei
    static AppConfig loadConfig(const std::string& filename);
    
    // Speichert die Konfiguration in eine JSON-Datei
    static void saveConfig(const AppConfig& config, const std::string& filename);
};
