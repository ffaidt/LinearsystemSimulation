#pragma once
#include "Config.h"
#include <vector>

struct MotionState {
    float t; float s; float v; float a;
};

struct AxisTrajectory {
    std::vector<float> t, s, v, a;
};

struct SimulationResult {
    AxisTrajectory x, y, z;
    float totalTime = 0.0f;
};

class Kinematics {
public:
    // PTP-Trajektorie
    static SimulationResult calculate(const AppConfig& config, float dt = 0.01f);
    // Pick & Place Trajektorie
    static SimulationResult calculatePickPlace(const PickPlaceConfig& config, float dt = 0.01f);

private:
    static void calcProfile(float distance, float v_max, float a_max,
                            float& t_a, float& t_cruise, float& v_max_actual);
    static MotionState evalProfile(float t, float start_pos, float distance,
                                   float t_a, float t_cruise, float v_max_actual, float a_max);
    // Hilfsfunktion: Berechnet eine Z-Fahrt mit optionaler sanfter Rampe am Ende
    static float appendZMove(SimulationResult& res, float startX, float startY,
                             float zFrom, float zTo, float vmax_z, float amax_z,
                             float softDist, float softAccel, float tOffset, float dt);
    // Hilfsfunktion: XY-Fahrt (Z bleibt konstant)
    static float appendXYMove(SimulationResult& res, float xFrom, float yFrom, float zConst,
                              float xTo, float yTo, float vmax_x, float vmax_y,
                              float amax_x, float amax_y, float tOffset, float dt);
    // Hilfsfunktion: XYZ gleichzeitig (oberhalb sicherer Hoehe)
    static float appendXYZMove(SimulationResult& res,
                               float xFrom, float yFrom, float zFrom,
                               float xTo, float yTo, float zTo,
                               float vmax_x, float vmax_y, float vmax_z,
                               float amax_x, float amax_y, float amax_z,
                               float tOffset, float dt);
    // Hilfsfunktion: Verweildauer
    static float appendDwell(SimulationResult& res, float x, float y, float z,
                             float duration, float tOffset, float dt);
};
