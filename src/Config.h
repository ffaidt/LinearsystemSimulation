#pragma once
#include <string>

// Konfiguration für eine einzelne Achse
struct AxisConfig {
    float start = 0.0f;
    float target = 100.0f;
    float v_max = 50.0f;
    float a_max = 20.0f;
};

// Gesamtkonfiguration der App
struct AppConfig {
    AxisConfig x;
    AxisConfig y;
    AxisConfig z;
};

class ConfigManager {
public:
    // Lädt die Konfiguration aus einer JSON-Datei
    static AppConfig loadConfig(const std::string& filename);
    
    // Speichert die Konfiguration in eine JSON-Datei
    static void saveConfig(const AppConfig& config, const std::string& filename);
};
