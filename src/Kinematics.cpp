#include "Kinematics.h"
#include <cmath>
#include <algorithm>
#include <limits>

void Kinematics::calcProfile(float distance, float v_max, float a_max,
                             float& t_a, float& t_cruise, float& v_max_actual) {
    if (std::abs(distance) < 1e-6f || v_max <= 0.0f || a_max <= 0.0f) {
        t_a = 0; t_cruise = 0; v_max_actual = 0; return;
    }
    float s = std::abs(distance);
    t_a = v_max / a_max;
    float s_a = 0.5f * a_max * t_a * t_a;
    if (2.0f * s_a > s) {
        s_a = s / 2.0f;
        t_a = std::sqrt(2.0f * s_a / a_max);
        v_max_actual = a_max * t_a;
        t_cruise = 0.0f;
    } else {
        t_cruise = (s - 2.0f * s_a) / v_max;
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
        state.s = start_pos; state.v = 0; state.a = 0;
    } else if (t < t_a) {
        state.a = a_max * dir;
        state.v = state.a * t;
        state.s = start_pos + 0.5f * state.a * t * t;
    } else if (t < t_a + t_cruise) {
        state.a = 0;
        state.v = v_max_actual * dir;
        float s_a = 0.5f * (a_max * dir) * t_a * t_a;
        state.s = start_pos + s_a + state.v * (t - t_a);
    } else if (t <= t_total) {
        float t_dec = t - t_a - t_cruise;
        state.a = -a_max * dir;
        state.v = (v_max_actual * dir) + state.a * t_dec;
        float dt_end = t_total - t;
        state.s = start_pos + distance - 0.5f * (a_max * dir) * dt_end * dt_end;
    } else {
        state.s = start_pos + distance; state.v = 0; state.a = 0;
    }
    return state;
}

// --- PTP (bestehend) ---
SimulationResult Kinematics::calculate(const AppConfig& config, float dt) {
    SimulationResult res;
    float z_dist = config.z.target - config.z.start;
    float z_ta = 0, z_tc = 0, z_vm = 0;
    calcProfile(z_dist, config.z.v_max, config.z.a_max, z_ta, z_tc, z_vm);
    float t_total_z = 2.0f * z_ta + z_tc;

    float dx = config.x.target - config.x.start;
    float dy = config.y.target - config.y.start;
    float L = std::sqrt(dx * dx + dy * dy);
    float xy_ta = 0, xy_tc = 0, xy_vm = 0, nx = 0, ny = 0;
    if (L > 1e-6f) {
        nx = dx / L; ny = dy / L;
        float pv = std::numeric_limits<float>::max(), pa = pv;
        if (std::abs(nx) > 1e-6f) { pv = std::min(pv, config.x.v_max/std::abs(nx)); pa = std::min(pa, config.x.a_max/std::abs(nx)); }
        if (std::abs(ny) > 1e-6f) { pv = std::min(pv, config.y.v_max/std::abs(ny)); pa = std::min(pa, config.y.a_max/std::abs(ny)); }
        calcProfile(L, pv, pa, xy_ta, xy_tc, xy_vm);
    }
    float t_total_xy = 2.0f * xy_ta + xy_tc;
    res.totalTime = t_total_z + t_total_xy;

    int steps = static_cast<int>(std::ceil(res.totalTime / dt)) + 1;
    for (int i = 0; i <= steps; ++i) {
        float t = std::min(static_cast<float>(i) * dt, res.totalTime);
        float t_z = std::min(t, t_total_z);
        MotionState sz = evalProfile(t_z, config.z.start, z_dist, z_ta, z_tc, z_vm, config.z.a_max);
        if (t > t_total_z) { sz.a = 0; sz.v = 0; }
        res.z.t.push_back(t); res.z.s.push_back(sz.s); res.z.v.push_back(sz.v); res.z.a.push_back(sz.a);

        float t_xy = t - t_total_z;
        if (t_xy < 0) {
            res.x.t.push_back(t); res.x.s.push_back(config.x.start); res.x.v.push_back(0); res.x.a.push_back(0);
            res.y.t.push_back(t); res.y.s.push_back(config.y.start); res.y.v.push_back(0); res.y.a.push_back(0);
        } else {
            float a_path = (xy_ta > 0) ? xy_vm / xy_ta : 0;
            MotionState sp = evalProfile(t_xy, 0, L, xy_ta, xy_tc, xy_vm, a_path);
            res.x.t.push_back(t); res.x.s.push_back(config.x.start + sp.s*nx); res.x.v.push_back(sp.v*nx); res.x.a.push_back(sp.a*nx);
            res.y.t.push_back(t); res.y.s.push_back(config.y.start + sp.s*ny); res.y.v.push_back(sp.v*ny); res.y.a.push_back(sp.a*ny);
        }
    }
    return res;
}

// --- Hilfsfunktionen fuer Pick & Place ---

float Kinematics::appendDwell(SimulationResult& res, float x, float y, float z,
                              float duration, float tOffset, float dt) {
    if (duration <= 0) return tOffset;
    int steps = static_cast<int>(std::ceil(duration / dt));
    for (int i = 1; i <= steps; ++i) {
        float t = tOffset + std::min(static_cast<float>(i) * dt, duration);
        res.x.t.push_back(t); res.x.s.push_back(x); res.x.v.push_back(0); res.x.a.push_back(0);
        res.y.t.push_back(t); res.y.s.push_back(y); res.y.v.push_back(0); res.y.a.push_back(0);
        res.z.t.push_back(t); res.z.s.push_back(z); res.z.v.push_back(0); res.z.a.push_back(0);
    }
    return tOffset + duration;
}

float Kinematics::appendZMove(SimulationResult& res, float startX, float startY,
                              float zFrom, float zTo, float vmax_z, float amax_z,
                              float softDist, float softAccel, float tOffset, float dt) {
    float totalDist = zTo - zFrom;
    if (std::abs(totalDist) < 1e-6f) return tOffset;

    float absDist = std::abs(totalDist);
    float actualSoft = std::min(softDist, absDist * 0.5f);

    // Schnelles Segment
    float fastDist = absDist - actualSoft;
    float fast_ta = 0, fast_tc = 0, fast_vm = 0;
    if (fastDist > 1e-6f) calcProfile(fastDist, vmax_z, amax_z, fast_ta, fast_tc, fast_vm);
    float fastTime = 2.0f * fast_ta + fast_tc;

    // Sanftes Segment  
    float soft_ta = 0, soft_tc = 0, soft_vm = 0;
    float softAmax = std::min(softAccel, amax_z);
    float softVmax = std::min(vmax_z, std::sqrt(2.0f * softAmax * std::max(actualSoft, 0.001f)));
    if (actualSoft > 1e-6f) calcProfile(actualSoft, softVmax, softAmax, soft_ta, soft_tc, soft_vm);
    float softTime = 2.0f * soft_ta + soft_tc;

    float moveTime = fastTime + softTime;
    float dir = totalDist >= 0 ? 1.0f : -1.0f;

    int steps = static_cast<int>(std::ceil(moveTime / dt));
    for (int i = 1; i <= steps; ++i) {
        float t_local = std::min(static_cast<float>(i) * dt, moveTime);
        float zPos, zVel, zAcc;

        if (t_local <= fastTime && fastDist > 1e-6f) {
            MotionState ms = evalProfile(t_local, 0, fastDist * dir, fast_ta, fast_tc, fast_vm, amax_z);
            zPos = zFrom + ms.s; zVel = ms.v; zAcc = ms.a;
        } else {
            float t_soft = t_local - fastTime;
            float softStart = zFrom + fastDist * dir;
            MotionState ms = evalProfile(t_soft, 0, actualSoft * dir, soft_ta, soft_tc, soft_vm, softAmax);
            zPos = softStart + ms.s; zVel = ms.v; zAcc = ms.a;
        }

        float t = tOffset + t_local;
        res.x.t.push_back(t); res.x.s.push_back(startX); res.x.v.push_back(0); res.x.a.push_back(0);
        res.y.t.push_back(t); res.y.s.push_back(startY); res.y.v.push_back(0); res.y.a.push_back(0);
        res.z.t.push_back(t); res.z.s.push_back(zPos); res.z.v.push_back(zVel); res.z.a.push_back(zAcc);
    }
    return tOffset + moveTime;
}

float Kinematics::appendXYMove(SimulationResult& res, float xFrom, float yFrom, float zConst,
                               float xTo, float yTo, float vmax_x, float vmax_y,
                               float amax_x, float amax_y, float tOffset, float dt) {
    float dx = xTo - xFrom, dy = yTo - yFrom;
    float L = std::sqrt(dx*dx + dy*dy);
    if (L < 1e-6f) return tOffset;
    float nx = dx/L, ny = dy/L;
    float pv = std::numeric_limits<float>::max(), pa = pv;
    if (std::abs(nx) > 1e-6f) { pv = std::min(pv, vmax_x/std::abs(nx)); pa = std::min(pa, amax_x/std::abs(nx)); }
    if (std::abs(ny) > 1e-6f) { pv = std::min(pv, vmax_y/std::abs(ny)); pa = std::min(pa, amax_y/std::abs(ny)); }
    float xy_ta=0, xy_tc=0, xy_vm=0;
    calcProfile(L, pv, pa, xy_ta, xy_tc, xy_vm);
    float moveTime = 2.0f * xy_ta + xy_tc;
    float a_path = (xy_ta > 0) ? xy_vm / xy_ta : 0;

    int steps = static_cast<int>(std::ceil(moveTime / dt));
    for (int i = 1; i <= steps; ++i) {
        float tl = std::min(static_cast<float>(i) * dt, moveTime);
        MotionState sp = evalProfile(tl, 0, L, xy_ta, xy_tc, xy_vm, a_path);
        float t = tOffset + tl;
        res.x.t.push_back(t); res.x.s.push_back(xFrom + sp.s*nx); res.x.v.push_back(sp.v*nx); res.x.a.push_back(sp.a*nx);
        res.y.t.push_back(t); res.y.s.push_back(yFrom + sp.s*ny); res.y.v.push_back(sp.v*ny); res.y.a.push_back(sp.a*ny);
        res.z.t.push_back(t); res.z.s.push_back(zConst); res.z.v.push_back(0); res.z.a.push_back(0);
    }
    return tOffset + moveTime;
}

// --- Pick & Place Hauptfunktion ---
SimulationResult Kinematics::calculatePickPlace(const PickPlaceConfig& cfg, float dt) {
    SimulationResult res;
    float t = 0.0f;

    // Startpunkt: Erste Pick-Position
    float curX = cfg.pickPositions[0].x;
    float curY = cfg.pickPositions[0].y;
    float curZ = cfg.pickPositions[0].z;

    // Initialer Punkt
    res.x.t.push_back(0); res.x.s.push_back(curX); res.x.v.push_back(0); res.x.a.push_back(0);
    res.y.t.push_back(0); res.y.s.push_back(curY); res.y.v.push_back(0); res.y.a.push_back(0);
    res.z.t.push_back(0); res.z.s.push_back(curZ); res.z.v.push_back(0); res.z.a.push_back(0);

    for (size_t p = 0; p < cfg.pickPositions.size(); ++p) {
        float pickX = cfg.pickPositions[p].x;
        float pickY = cfg.pickPositions[p].y;
        float pickZ = cfg.pickPositions[p].z;

        // 1. Z hoch auf sichere Hoehe (sanftes Losfahren)
        t = appendZMove(res, curX, curY, curZ, cfg.safeZ, cfg.vmax_z, cfg.amax_z,
                        cfg.softDistance, cfg.softAccel, t, dt);
        curZ = cfg.safeZ;

        // 2. XY zur Pick-Position (Z bleibt auf sicherer Hoehe)
        t = appendXYMove(res, curX, curY, curZ, pickX, pickY,
                         cfg.vmax_x, cfg.vmax_y, cfg.amax_x, cfg.amax_y, t, dt);
        curX = pickX; curY = pickY;

        // 3. Z runter zur Pick-Hoehe (sanftes Abbremsen)
        t = appendZMove(res, curX, curY, curZ, pickZ, cfg.vmax_z, cfg.amax_z,
                        cfg.softDistance, cfg.softAccel, t, dt);
        curZ = pickZ;

        // 4. Verweilen am Pick
        t = appendDwell(res, curX, curY, curZ, cfg.dwellTime, t, dt);

        // 5. Z hoch auf sichere Hoehe
        t = appendZMove(res, curX, curY, curZ, cfg.safeZ, cfg.vmax_z, cfg.amax_z,
                        cfg.softDistance, cfg.softAccel, t, dt);
        curZ = cfg.safeZ;

        // 6. XY zur Place-Position
        t = appendXYMove(res, curX, curY, curZ, cfg.placePosition.x, cfg.placePosition.y,
                         cfg.vmax_x, cfg.vmax_y, cfg.amax_x, cfg.amax_y, t, dt);
        curX = cfg.placePosition.x; curY = cfg.placePosition.y;

        // 7. Z runter zur Place-Hoehe
        t = appendZMove(res, curX, curY, curZ, cfg.placePosition.z, cfg.vmax_z, cfg.amax_z,
                        cfg.softDistance, cfg.softAccel, t, dt);
        curZ = cfg.placePosition.z;

        // 8. Verweilen am Place
        t = appendDwell(res, curX, curY, curZ, cfg.dwellTime, t, dt);

        // 9. Z hoch auf sichere Hoehe (fuer naechsten Pick)
        t = appendZMove(res, curX, curY, curZ, cfg.safeZ, cfg.vmax_z, cfg.amax_z,
                        cfg.softDistance, cfg.softAccel, t, dt);
        curZ = cfg.safeZ;
    }

    res.totalTime = t;
    return res;
}
