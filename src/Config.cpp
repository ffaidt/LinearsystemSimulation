#include "Config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// Hilfsfunktionen für nlohmann::json zur Serialisierung von AxisConfig
void to_json(json& j, const AxisConfig& a) {
    j = json{{"start", a.start}, {"target", a.target}, {"v_max", a.v_max}, {"a_max", a.a_max}};
}

void from_json(const json& j, AxisConfig& a) {
    j.at("start").get_to(a.start);
    j.at("target").get_to(a.target);
    j.at("v_max").get_to(a.v_max);
    j.at("a_max").get_to(a.a_max);
}

// Hilfsfunktionen für AppConfig
void to_json(json& j, const AppConfig& a) {
    j = json{{"x", a.x}, {"y", a.y}, {"z", a.z}};
}

void from_json(const json& j, AppConfig& a) {
    j.at("x").get_to(a.x);
    j.at("y").get_to(a.y);
    j.at("z").get_to(a.z);
}

AppConfig ConfigManager::loadConfig(const std::string& filename) {
    AppConfig config; // Standardwerte, falls die Datei nicht existiert
    
    // Initialisiere sinnvolle Standardwerte
    config.x = {0.0f, 100.0f, 50.0f, 20.0f};
    config.y = {0.0f, 100.0f, 50.0f, 20.0f};
    config.z = {0.0f, 50.0f, 20.0f, 10.0f};

    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            json j;
            file >> j;
            config = j.get<AppConfig>();
            std::cout << "Konfiguration aus " << filename << " geladen.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Fehler beim Laden der Konfiguration: " << e.what() << "\nVerwende Standardwerte.\n";
    }

    return config;
}

void ConfigManager::saveConfig(const AppConfig& config, const std::string& filename) {
    try {
        json j = config;
        std::ofstream file(filename);
        if (file.is_open()) {
            file << j.dump(4); // Mit 4 Leerzeichen Einrückung formatiert speichern
            std::cout << "Konfiguration in " << filename << " gespeichert.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Fehler beim Speichern der Konfiguration: " << e.what() << "\n";
    }
}
