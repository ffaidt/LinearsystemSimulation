#include "Gui.h"
#include <imgui.h>
#include <implot.h>

Gui::Gui() {
    m_config = ConfigManager::loadConfig("config.json");
}

Gui::~Gui() {
    ConfigManager::saveConfig(m_config, "config.json");
}

void Gui::renderConfigPanel() {
    ImGui::Begin("Einstellungen", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    auto drawAxisConfig = [](const char* label, AxisConfig& ax) {
        ImGui::PushID(label);
        ImGui::Text("%s-Achse", label);
        ImGui::DragFloat("Start", &ax.start, 0.5f, 0.0f, 0.0f, "%.1f mm");
        ImGui::DragFloat("Ziel", &ax.target, 0.5f, 0.0f, 0.0f, "%.1f mm");
        ImGui::DragFloat("v_max", &ax.v_max, 0.5f, 0.1f, 1000.0f, "%.1f mm/s");
        ImGui::DragFloat("a_max", &ax.a_max, 0.5f, 0.1f, 1000.0f, "%.1f mm/s^2");
        ImGui::Separator();
        ImGui::PopID();
    };

    drawAxisConfig("X", m_config.x);
    drawAxisConfig("Y", m_config.y);
    drawAxisConfig("Z", m_config.z);

    ImGui::Spacing();
    
    if (ImGui::Button("Simulation Starten", ImVec2(-1, 30))) {
        m_result = Kinematics::calculate(m_config);
        m_hasResult = true;
        
        // Isometrische Projektion vorbereiten
        m_isoU.clear();
        m_isoV.clear();
        for (size_t i = 0; i < m_result.x.t.size(); ++i) {
            float x = m_result.x.s[i];
            float y = m_result.y.s[i];
            float z = m_result.z.s[i];
            float u = (x - y) * 0.866f;
            float v = z - (x + y) * 0.5f;
            m_isoU.push_back(u);
            m_isoV.push_back(v);
        }
        m_animationTime = 0.0f;
        m_isPlaying = false;
    }

    if (m_hasResult) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Gesamtzeit: %.3f s", m_result.totalTime);
        ImGui::Spacing();
    }

    if (ImGui::Button("Beenden", ImVec2(-1, 30))) {
        m_shouldClose = true;
    }

    ImGui::End();
}

void Gui::renderPlots() {
    if (!m_hasResult) return;

    int count = static_cast<int>(m_result.x.t.size());
    if (count == 0) return;

    // --- Positions-Fenster ---
    ImGui::Begin("Position");
    if (ImPlot::BeginPlot("##PositionPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Zeit t [s]", "Position [mm]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("X-Achse", m_result.x.t.data(), m_result.x.s.data(), count);
        ImPlot::PlotLine("Y-Achse", m_result.y.t.data(), m_result.y.s.data(), count);
        ImPlot::PlotLine("Z-Achse", m_result.z.t.data(), m_result.z.s.data(), count);
        ImPlot::EndPlot();
    }
    ImGui::End();

    // --- Geschwindigkeits-Fenster ---
    ImGui::Begin("Geschwindigkeit");
    if (ImPlot::BeginPlot("##GeschwindigkeitPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Zeit t [s]", "Geschw. [mm/s]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("X-Achse", m_result.x.t.data(), m_result.x.v.data(), count);
        ImPlot::PlotLine("Y-Achse", m_result.y.t.data(), m_result.y.v.data(), count);
        ImPlot::PlotLine("Z-Achse", m_result.z.t.data(), m_result.z.v.data(), count);
        ImPlot::EndPlot();
    }
    ImGui::End();

    // --- Beschleunigungs-Fenster ---
    ImGui::Begin("Beschleunigung");
    if (ImPlot::BeginPlot("##BeschleunigungPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Zeit t [s]", "Beschl. [mm/s^2]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("X-Achse", m_result.x.t.data(), m_result.x.a.data(), count);
        ImPlot::PlotLine("Y-Achse", m_result.y.t.data(), m_result.y.a.data(), count);
        ImPlot::PlotLine("Z-Achse", m_result.z.t.data(), m_result.z.a.data(), count);
        ImPlot::EndPlot();
    }
    ImGui::End();
}

void Gui::renderAnimation() {
    if (!m_hasResult || m_isoU.empty()) return;

    ImGui::Begin("Animation");

    if (ImGui::Button(m_isPlaying ? "Pause" : "Play", ImVec2(80, 0))) {
        m_isPlaying = !m_isPlaying;
    }
    ImGui::SameLine();
    
    if (m_isPlaying) {
        m_animationTime += ImGui::GetIO().DeltaTime;
        if (m_animationTime >= m_result.totalTime) {
            m_animationTime = m_result.totalTime;
            m_isPlaying = false;
        }
    }
    
    ImGui::SliderFloat("Zeit", &m_animationTime, 0.0f, m_result.totalTime, "%.3f s");

    int count = static_cast<int>(m_result.x.t.size());
    int currentIndex = 0;
    for (int i = 0; i < count; ++i) {
        if (m_result.x.t[i] >= m_animationTime) {
            currentIndex = i;
            break;
        }
    }

    if (ImPlot::BeginSubplots("##AnimationViews", 2, 2, ImVec2(-1, -1))) {
        
        // 1: Iso Ansicht
        if (ImPlot::BeginPlot("Iso Ansicht")) {
            ImPlot::SetupAxes("U", "V", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels);
            ImPlot::PlotLine("Pfad", m_isoU.data(), m_isoV.data(), count, ImPlotSpec(
                ImPlotProp_LineColor, ImVec4(0.5f, 0.5f, 0.5f, 0.5f), ImPlotProp_LineWeight, 2.0f
            ));
            float currU = m_isoU[currentIndex];
            float currV = m_isoV[currentIndex];
            ImPlot::PlotScatter("Aktuelle Position", &currU, &currV, 1, ImPlotSpec(
                ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 8.0f, ImPlotProp_MarkerFillColor, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
            ));
            ImPlot::EndPlot();
        }

        // 2: X-Z Ansicht
        if (ImPlot::BeginPlot("X-Z Ansicht")) {
            ImPlot::SetupAxes("X [mm]", "Z [mm]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::PlotLine("Pfad", m_result.x.s.data(), m_result.z.s.data(), count, ImPlotSpec(
                ImPlotProp_LineColor, ImVec4(0.5f, 0.5f, 0.5f, 0.5f), ImPlotProp_LineWeight, 2.0f
            ));
            float currX = m_result.x.s[currentIndex];
            float currZ = m_result.z.s[currentIndex];
            ImPlot::PlotScatter("Aktuelle Position", &currX, &currZ, 1, ImPlotSpec(
                ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 8.0f, ImPlotProp_MarkerFillColor, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
            ));
            ImPlot::EndPlot();
        }

        // 3: Y-Z Ansicht
        if (ImPlot::BeginPlot("Y-Z Ansicht")) {
            ImPlot::SetupAxes("Y [mm]", "Z [mm]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::PlotLine("Pfad", m_result.y.s.data(), m_result.z.s.data(), count, ImPlotSpec(
                ImPlotProp_LineColor, ImVec4(0.5f, 0.5f, 0.5f, 0.5f), ImPlotProp_LineWeight, 2.0f
            ));
            float currY = m_result.y.s[currentIndex];
            float currZ = m_result.z.s[currentIndex];
            ImPlot::PlotScatter("Aktuelle Position", &currY, &currZ, 1, ImPlotSpec(
                ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 8.0f, ImPlotProp_MarkerFillColor, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
            ));
            ImPlot::EndPlot();
        }

        // 4: X-Y Ansicht
        if (ImPlot::BeginPlot("X-Y Ansicht")) {
            ImPlot::SetupAxes("X [mm]", "Y [mm]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::PlotLine("Pfad", m_result.x.s.data(), m_result.y.s.data(), count, ImPlotSpec(
                ImPlotProp_LineColor, ImVec4(0.5f, 0.5f, 0.5f, 0.5f), ImPlotProp_LineWeight, 2.0f
            ));
            float currX = m_result.x.s[currentIndex];
            float currY = m_result.y.s[currentIndex];
            ImPlot::PlotScatter("Aktuelle Position", &currX, &currY, 1, ImPlotSpec(
                ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 8.0f, ImPlotProp_MarkerFillColor, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
            ));
            ImPlot::EndPlot();
        }

        ImPlot::EndSubplots();
    }
    ImGui::End();
}

void Gui::render() {
    renderConfigPanel();
    renderPlots();
    renderAnimation();
}
