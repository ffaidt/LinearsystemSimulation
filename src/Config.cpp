#include "Config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

void to_json(json& j, const Vec3& v) {
    j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}
void from_json(const json& j, Vec3& v) {
    j.at("x").get_to(v.x); j.at("y").get_to(v.y); j.at("z").get_to(v.z);
}

void to_json(json& j, const AxisConfig& a) {
    j = json{{"start", a.start}, {"target", a.target}, {"v_max", a.v_max}, {"a_max", a.a_max}};
}
void from_json(const json& j, AxisConfig& a) {
    j.at("start").get_to(a.start); j.at("target").get_to(a.target);
    j.at("v_max").get_to(a.v_max); j.at("a_max").get_to(a.a_max);
}

void to_json(json& j, const PickPlaceConfig& p) {
    j = json{
        {"pickPositions", p.pickPositions}, {"placePosition", p.placePosition},
        {"safeZ", p.safeZ}, {"dwellTime", p.dwellTime},
        {"softDistance", p.softDistance}, {"softAccel", p.softAccel},
        {"vmax_x", p.vmax_x}, {"vmax_y", p.vmax_y}, {"vmax_z", p.vmax_z},
        {"amax_x", p.amax_x}, {"amax_y", p.amax_y}, {"amax_z", p.amax_z}
    };
}
void from_json(const json& j, PickPlaceConfig& p) {
    if (j.contains("pickPositions")) j.at("pickPositions").get_to(p.pickPositions);
    if (j.contains("placePosition")) j.at("placePosition").get_to(p.placePosition);
    if (j.contains("safeZ")) j.at("safeZ").get_to(p.safeZ);
    if (j.contains("dwellTime")) j.at("dwellTime").get_to(p.dwellTime);
    if (j.contains("softDistance")) j.at("softDistance").get_to(p.softDistance);
    if (j.contains("softAccel")) j.at("softAccel").get_to(p.softAccel);
    if (j.contains("vmax_x")) j.at("vmax_x").get_to(p.vmax_x);
    if (j.contains("vmax_y")) j.at("vmax_y").get_to(p.vmax_y);
    if (j.contains("vmax_z")) j.at("vmax_z").get_to(p.vmax_z);
    if (j.contains("amax_x")) j.at("amax_x").get_to(p.amax_x);
    if (j.contains("amax_y")) j.at("amax_y").get_to(p.amax_y);
    if (j.contains("amax_z")) j.at("amax_z").get_to(p.amax_z);
}

void to_json(json& j, const AppConfig& a) {
    j = json{{"x", a.x}, {"y", a.y}, {"z", a.z}, {"pp", a.pp}};
}
void from_json(const json& j, AppConfig& a) {
    if (j.contains("x")) j.at("x").get_to(a.x);
    if (j.contains("y")) j.at("y").get_to(a.y);
    if (j.contains("z")) j.at("z").get_to(a.z);
    if (j.contains("pp")) j.at("pp").get_to(a.pp);
}

AppConfig ConfigManager::loadConfig(const std::string& filename) {
    AppConfig config;
    config.x = {0.0f, 100.0f, 50.0f, 20.0f};
    config.y = {0.0f, 100.0f, 50.0f, 20.0f};
    config.z = {0.0f, 50.0f, 20.0f, 10.0f};
    try {
        std::ifstream file(filename);
        if (file.is_open()) {
            json j; file >> j;
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
            file << j.dump(4);
            std::cout << "Konfiguration in " << filename << " gespeichert.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Fehler beim Speichern der Konfiguration: " << e.what() << "\n";
    }
}
