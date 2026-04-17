#include "Kinematics.h"
#include <cmath>
#include <algorithm>

void Kinematics::calcProfile(float distance, float v_max, float a_max, 
                             float& t_a, float& t_cruise, float& v_max_actual) {
    if (std::abs(distance) < 1e-6f || v_max <= 0.0f || a_max <= 0.0f) {
        t_a = 0.0f;
        t_cruise = 0.0f;
        v_max_actual = 0.0f;
        return;
    }

    float s = std::abs(distance);
    t_a = v_max / a_max;
    float s_a = 0.5f * a_max * t_a * t_a;

    if (2.0f * s_a > s) {
        // Dreiecksprofil (v_max wird nicht erreicht)
        s_a = s / 2.0f;
        t_a = std::sqrt(2.0f * s_a / a_max);
        v_max_actual = a_max * t_a;
        t_cruise = 0.0f;
    } else {
        // Trapezprofil
        float s_cruise = s - 2.0f * s_a;
        t_cruise = s_cruise / v_max;
        v_max_actual = v_max;
    }
}

MotionState Kinematics::evalProfile(float t, float start_pos, float distance, 
                                    float t_a, float t_cruise, float v_max_actual, float a_max) {
    MotionState state;
    state.t = t;
    float dir = distance >= 0.0f ? 1.0f : -1.0f;
    float t_total = 2.0f * t_a + t_cruise;

    if (t < 0.0f) {
        state.s = start_pos;
        state.v = 0.0f;
        state.a = 0.0f;
    } else if (t < t_a) {
        // Beschleunigungsphase
        state.a = a_max * dir;
        state.v = state.a * t;
        state.s = start_pos + 0.5f * state.a * t * t;
    } else if (t < t_a + t_cruise) {
        // Konstantfahrphase
        state.a = 0.0f;
        state.v = v_max_actual * dir;
        float s_a = 0.5f * (a_max * dir) * t_a * t_a;
        state.s = start_pos + s_a + state.v * (t - t_a);
    } else if (t <= t_total) {
        // Verzögerungsphase
        float t_dec = t - t_a - t_cruise;
        state.a = -a_max * dir;
        state.v = (v_max_actual * dir) + state.a * t_dec;
        // Restdistanz rückwärts berechnen
        float dt_end = t_total - t;
        state.s = start_pos + distance - 0.5f * (a_max * dir) * dt_end * dt_end;
    } else {
        // Ziel erreicht
        state.s = start_pos + distance;
        state.v = 0.0f;
        state.a = 0.0f;
    }
    return state;
}

SimulationResult Kinematics::calculate(const AppConfig& config, float dt) {
    SimulationResult res;

    // 1. Z-Achse isoliert berechnen (fährt zuerst)
    float z_dist = config.z.target - config.z.start;
    float z_ta = 0.0f, z_tcruise = 0.0f, z_vmax = 0.0f;
    calcProfile(z_dist, config.z.v_max, config.z.a_max, z_ta, z_tcruise, z_vmax);
    float t_total_z = 2.0f * z_ta + z_tcruise;

    // 2. X/Y-Achse interpoliert berechnen (fahren gleichzeitig nach Z)
    float dx = config.x.target - config.x.start;
    float dy = config.y.target - config.y.start;
    float L = std::sqrt(dx * dx + dy * dy);

    float xy_ta = 0.0f, xy_tcruise = 0.0f, xy_vmax_path = 0.0f;
    float nx = 0.0f, ny = 0.0f;

    if (L > 1e-6f) {
        nx = dx / L;
        ny = dy / L;
        
        // Finde die maximal zulässige Pfad-Geschwindigkeit und -Beschleunigung,
        // sodass keine der Einzelachsen ihr Limit überschreitet.
        // v_path * |nx| <= v_max_x  =>  v_path <= v_max_x / |nx|
        float path_v_max = std::numeric_limits<float>::max();
        float path_a_max = std::numeric_limits<float>::max();

        if (std::abs(nx) > 1e-6f) {
            path_v_max = std::min(path_v_max, config.x.v_max / std::abs(nx));
            path_a_max = std::min(path_a_max, config.x.a_max / std::abs(nx));
        }
        if (std::abs(ny) > 1e-6f) {
            path_v_max = std::min(path_v_max, config.y.v_max / std::abs(ny));
            path_a_max = std::min(path_a_max, config.y.a_max / std::abs(ny));
        }

        calcProfile(L, path_v_max, path_a_max, xy_ta, xy_tcruise, xy_vmax_path);
    }
    float t_total_xy = 2.0f * xy_ta + xy_tcruise;

    // Gesamtzeit
    res.totalTime = t_total_z + t_total_xy;

    // 3. Trajektorien abtasten
    int num_steps = static_cast<int>(std::ceil(res.totalTime / dt)) + 1;
    for (int i = 0; i <= num_steps; ++i) {
        float t = static_cast<float>(i) * dt;
        if (t > res.totalTime) t = res.totalTime;

        // Z-Achse Bewegung (t = 0 bis t_total_z)
        float t_z = std::min(t, t_total_z);
        MotionState st_z = evalProfile(t_z, config.z.start, z_dist, z_ta, z_tcruise, z_vmax, config.z.a_max);

        // Z bleibt nach t_total_z stehen, a und v sollten dann 0 sein
        if (t > t_total_z) {
            st_z.a = 0.0f;
            st_z.v = 0.0f;
        }

        res.z.t.push_back(t);
        res.z.s.push_back(st_z.s);
        res.z.v.push_back(st_z.v);
        res.z.a.push_back(st_z.a);

        // X/Y-Achse Bewegung (t = t_total_z bis res.totalTime)
        float t_xy = t - t_total_z;
        if (t_xy < 0.0f) {
            // XY warten noch
            res.x.t.push_back(t);
            res.x.s.push_back(config.x.start);
            res.x.v.push_back(0.0f);
            res.x.a.push_back(0.0f);

            res.y.t.push_back(t);
            res.y.s.push_back(config.y.start);
            res.y.v.push_back(0.0f);
            res.y.a.push_back(0.0f);
        } else {
            // XY bewegen sich
            // Profil für den Pfad L auswerten
            MotionState st_path = evalProfile(t_xy, 0.0f, L, xy_ta, xy_tcruise, xy_vmax_path, 
                                              (xy_ta > 0.0f ? xy_vmax_path / xy_ta : 0.0f)); 
                                              // a_max_actual = vmax_actual / t_a

            // Zurück auf X und Y projizieren
            res.x.t.push_back(t);
            res.x.s.push_back(config.x.start + st_path.s * nx);
            res.x.v.push_back(st_path.v * nx);
            res.x.a.push_back(st_path.a * nx);

            res.y.t.push_back(t);
            res.y.s.push_back(config.y.start + st_path.s * ny);
            res.y.v.push_back(st_path.v * ny);
            res.y.a.push_back(st_path.a * ny);
        }
    }

    return res;
}
