#include "Gui.h"
#include <imgui.h>
#include <implot.h>
#include <cmath>

Gui::Gui() { m_config = ConfigManager::loadConfig("config.json"); }
Gui::~Gui() { ConfigManager::saveConfig(m_config, "config.json"); }

void Gui::renderConfigPanel() {
    ImGui::Begin("Einstellungen", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::BeginTabBar("##ModeTabs")) {
        // --- PTP Tab ---
        if (ImGui::BeginTabItem("PTP")) {
            m_mode = 0;
            auto drawAxis = [](const char* label, AxisConfig& ax) {
                ImGui::PushID(label);
                ImGui::Text("%s-Achse", label);
                ImGui::DragFloat("Start", &ax.start, 0.5f, 0.0f, 0.0f, "%.1f mm");
                ImGui::DragFloat("Ziel", &ax.target, 0.5f, 0.0f, 0.0f, "%.1f mm");
                ImGui::DragFloat("v_max", &ax.v_max, 0.5f, 0.1f, 1000.0f, "%.1f mm/s");
                ImGui::DragFloat("a_max", &ax.a_max, 0.5f, 0.1f, 1000.0f, "%.1f mm/s^2");
                ImGui::Separator();
                ImGui::PopID();
            };
            drawAxis("X", m_config.x);
            drawAxis("Y", m_config.y);
            drawAxis("Z", m_config.z);

            if (ImGui::Button("Simulation Starten##PTP", ImVec2(-1, 30))) {
                m_result = Kinematics::calculate(m_config);
                m_hasResult = true;
                m_animationTime = 0; m_isPlaying = false;
            }
            ImGui::EndTabItem();
        }

        // --- Pick & Place Tab ---
        if (ImGui::BeginTabItem("Pick & Place")) {
            m_mode = 1;
            auto& pp = m_config.pp;

            ImGui::Text("--- Achsparameter ---");
            ImGui::DragFloat("v_max X", &pp.vmax_x, 0.5f, 0.1f, 1000.0f, "%.1f mm/s");
            ImGui::DragFloat("v_max Y", &pp.vmax_y, 0.5f, 0.1f, 1000.0f, "%.1f mm/s");
            ImGui::DragFloat("v_max Z", &pp.vmax_z, 0.5f, 0.1f, 1000.0f, "%.1f mm/s");
            ImGui::DragFloat("a_max X", &pp.amax_x, 0.5f, 0.1f, 1000.0f, "%.1f mm/s^2");
            ImGui::DragFloat("a_max Y", &pp.amax_y, 0.5f, 0.1f, 1000.0f, "%.1f mm/s^2");
            ImGui::DragFloat("a_max Z", &pp.amax_z, 0.5f, 0.1f, 1000.0f, "%.1f mm/s^2");
            ImGui::Separator();

            ImGui::Text("--- Allgemein ---");
            ImGui::DragFloat("Sichere Z-Hoehe", &pp.safeZ, 0.5f, 0.0f, 0.0f, "%.1f mm");
            ImGui::DragFloat("Verweildauer", &pp.dwellTime, 0.01f, 0.0f, 10.0f, "%.2f s");
            ImGui::DragFloat("Soft-Strecke", &pp.softDistance, 0.5f, 0.0f, 500.0f, "%.1f mm");
            ImGui::DragFloat("Soft-Beschl.", &pp.softAccel, 0.1f, 0.1f, 100.0f, "%.1f mm/s^2");
            ImGui::Separator();

            ImGui::Text("--- Place-Position ---");
            ImGui::DragFloat("Place X", &pp.placePosition.x, 0.5f, 0.0f, 0.0f, "%.1f mm");
            ImGui::DragFloat("Place Y", &pp.placePosition.y, 0.5f, 0.0f, 0.0f, "%.1f mm");
            ImGui::DragFloat("Place Z", &pp.placePosition.z, 0.5f, 0.0f, 0.0f, "%.1f mm");
            ImGui::Separator();

            ImGui::Text("--- Pick-Positionen (%d) ---", (int)pp.pickPositions.size());
            for (int i = 0; i < (int)pp.pickPositions.size(); ++i) {
                ImGui::PushID(i);
                ImGui::Text("Pick %d:", i + 1);
                ImGui::DragFloat("X", &pp.pickPositions[i].x, 0.5f, 0.0f, 0.0f, "%.1f mm");
                ImGui::SameLine(); ImGui::DragFloat("Y", &pp.pickPositions[i].y, 0.5f, 0.0f, 0.0f, "%.1f mm");
                ImGui::SameLine(); ImGui::DragFloat("Z", &pp.pickPositions[i].z, 0.5f, 0.0f, 0.0f, "%.1f mm");
                ImGui::SameLine();
                if (pp.pickPositions.size() > 1 && ImGui::Button("X##del")) {
                    pp.pickPositions.erase(pp.pickPositions.begin() + i);
                    ImGui::PopID(); break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("+ Pick hinzufuegen", ImVec2(-1, 25))) {
                pp.pickPositions.push_back({0, 0, 0});
            }
            ImGui::Spacing();

            if (ImGui::Button("Simulation Starten##PP", ImVec2(-1, 30))) {
                m_result = Kinematics::calculatePickPlace(pp);
                m_hasResult = true;
                m_animationTime = 0; m_isPlaying = false;
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (m_hasResult) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Gesamtzeit: %.3f s", m_result.totalTime);
        ImGui::Spacing();
    }
    if (ImGui::Button("Beenden", ImVec2(-1, 30))) m_shouldClose = true;
    ImGui::End();
}

void Gui::renderPlots() {
    if (!m_hasResult) return;
    int count = static_cast<int>(m_result.x.t.size());
    if (count == 0) return;

    ImPlotSpec specX(ImPlotProp_LineColor, ImVec4(1.0f, 0.3f, 0.3f, 1.0f), ImPlotProp_LineWeight, 2.0f);
    ImPlotSpec specY(ImPlotProp_LineColor, ImVec4(0.3f, 1.0f, 0.3f, 1.0f), ImPlotProp_LineWeight, 2.0f);
    ImPlotSpec specZ(ImPlotProp_LineColor, ImVec4(0.3f, 0.5f, 1.0f, 1.0f), ImPlotProp_LineWeight, 2.0f);

    ImGui::Begin("Position");
    if (ImPlot::BeginPlot("##PositionPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Zeit t [s]", "Position [mm]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("X", m_result.x.t.data(), m_result.x.s.data(), count, specX);
        ImPlot::PlotLine("Y", m_result.y.t.data(), m_result.y.s.data(), count, specY);
        ImPlot::PlotLine("Z", m_result.z.t.data(), m_result.z.s.data(), count, specZ);
        ImPlot::EndPlot();
    }
    ImGui::End();

    ImGui::Begin("Geschwindigkeit");
    if (ImPlot::BeginPlot("##GeschwindigkeitPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Zeit t [s]", "Geschw. [mm/s]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("X", m_result.x.t.data(), m_result.x.v.data(), count, specX);
        ImPlot::PlotLine("Y", m_result.y.t.data(), m_result.y.v.data(), count, specY);
        ImPlot::PlotLine("Z", m_result.z.t.data(), m_result.z.v.data(), count, specZ);
        ImPlot::EndPlot();
    }
    ImGui::End();

    ImGui::Begin("Beschleunigung");
    if (ImPlot::BeginPlot("##BeschleunigungPlot", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Zeit t [s]", "Beschl. [mm/s^2]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("X", m_result.x.t.data(), m_result.x.a.data(), count, specX);
        ImPlot::PlotLine("Y", m_result.y.t.data(), m_result.y.a.data(), count, specY);
        ImPlot::PlotLine("Z", m_result.z.t.data(), m_result.z.a.data(), count, specZ);
        ImPlot::EndPlot();
    }
    ImGui::End();
}

void Gui::render3DView(int currentIndex, int count, ImVec2 canvasPos, ImVec2 canvasSize) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("##3DCanvas", canvasSize);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        m_rotZ += delta.x * 0.01f;
        m_rotX += delta.y * 0.01f;
        if (m_rotX < -1.5f) m_rotX = -1.5f;
        if (m_rotX > 1.5f) m_rotX = 1.5f;
    }
    if (ImGui::IsItemHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            m_zoom *= (1.0f + wheel * 0.1f);
            if (m_zoom < 0.1f) m_zoom = 0.1f;
            if (m_zoom > 10.0f) m_zoom = 10.0f;
        }
    }
    dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x+canvasSize.x, canvasPos.y+canvasSize.y), IM_COL32(25,25,35,255));
    dl->PushClipRect(canvasPos, ImVec2(canvasPos.x+canvasSize.x, canvasPos.y+canvasSize.y), true);
    float cz = cosf(m_rotZ), sz = sinf(m_rotZ), cx = cosf(m_rotX), sx = sinf(m_rotX);
    float mnX=m_result.x.s[0],mxX=mnX,mnY=m_result.y.s[0],mxY=mnY,mnZ=m_result.z.s[0],mxZ=mnZ;
    for (int i=1;i<count;++i) {
        float x=m_result.x.s[i],y=m_result.y.s[i],z=m_result.z.s[i];
        if(x<mnX)mnX=x; if(x>mxX)mxX=x;
        if(y<mnY)mnY=y; if(y>mxY)mxY=y;
        if(z<mnZ)mnZ=z; if(z>mxZ)mxZ=z;
    }
    float pad=5.0f; mnX-=pad;mxX+=pad;mnY-=pad;mxY+=pad;mnZ-=pad;mxZ+=pad;
    float midX=(mnX+mxX)*0.5f,midY=(mnY+mxY)*0.5f,midZ=(mnZ+mxZ)*0.5f;
    float span=fmaxf(fmaxf(mxX-mnX,mxY-mnY),fmaxf(mxZ-mnZ,1.0f));
    float scale=fminf(canvasSize.x,canvasSize.y)*0.35f/span*m_zoom;
    ImVec2 center(canvasPos.x+canvasSize.x*0.5f,canvasPos.y+canvasSize.y*0.5f);
    auto project=[&](float px,float py,float pz)->ImVec2{
        float dx=px-midX,dy=py-midY,dz=pz-midZ;
        float rx=cz*dx-sz*dy,ry=sz*dx+cz*dy;
        float fz=sx*ry+cx*dz;
        return ImVec2(center.x+rx*scale,center.y-fz*scale);
    };
    float bx[2]={mnX,mxX},by[2]={mnY,mxY},bz[2]={mnZ,mxZ};
    ImU32 gc=IM_COL32(80,80,100,120);
    for(int i=0;i<2;++i)for(int j=0;j<2;++j){
        dl->AddLine(project(bx[0],by[i],bz[j]),project(bx[1],by[i],bz[j]),gc);
        dl->AddLine(project(bx[i],by[0],bz[j]),project(bx[i],by[1],bz[j]),gc);
        dl->AddLine(project(bx[i],by[j],bz[0]),project(bx[i],by[j],bz[1]),gc);
    }
    float axL=span*0.5f+pad; ImVec2 o0=project(midX,midY,midZ);
    dl->AddLine(o0,project(midX+axL,midY,midZ),IM_COL32(255,80,80,200),2);
    dl->AddLine(o0,project(midX,midY+axL,midZ),IM_COL32(80,255,80,200),2);
    dl->AddLine(o0,project(midX,midY,midZ+axL),IM_COL32(80,80,255,200),2);
    dl->AddText(project(midX+axL,midY,midZ),IM_COL32(255,100,100,255),"X");
    dl->AddText(project(midX,midY+axL,midZ),IM_COL32(100,255,100,255),"Y");
    dl->AddText(project(midX,midY,midZ+axL),IM_COL32(100,100,255,255),"Z");
    for(int i=1;i<count;++i){
        dl->AddLine(project(m_result.x.s[i-1],m_result.y.s[i-1],m_result.z.s[i-1]),
                    project(m_result.x.s[i],m_result.y.s[i],m_result.z.s[i]),IM_COL32(200,200,200,140),2);
    }
    ImVec2 pos=project(m_result.x.s[currentIndex],m_result.y.s[currentIndex],m_result.z.s[currentIndex]);
    dl->AddCircleFilled(pos,7,IM_COL32(255,50,50,255));
    dl->AddCircle(pos,9,IM_COL32(255,255,255,200),0,2);
    dl->AddText(ImVec2(canvasPos.x+5,canvasPos.y+5),IM_COL32(180,180,180,200),"Maus ziehen = Drehen, Scrollrad = Zoom");
    dl->PopClipRect();
}

void Gui::renderAnimation() {
    if (!m_hasResult) return;
    int count = static_cast<int>(m_result.x.t.size());
    if (count == 0) return;
    ImGui::Begin("Animation");
    if (ImGui::Button(m_isPlaying?"Pause":"Play",ImVec2(80,0))) m_isPlaying=!m_isPlaying;
    ImGui::SameLine();
    if (m_isPlaying) {
        m_animationTime+=ImGui::GetIO().DeltaTime;
        if (m_animationTime>=m_result.totalTime){m_animationTime=m_result.totalTime;m_isPlaying=false;}
    }
    ImGui::SliderFloat("Zeit",&m_animationTime,0,m_result.totalTime,"%.3f s");
    int ci=0;
    for(int i=0;i<count;++i){if(m_result.x.t[i]>=m_animationTime){ci=i;break;}}
    ImVec2 avail=ImGui::GetContentRegionAvail();
    float halfW=avail.x*0.5f,topH=avail.y*0.5f;
    ImVec2 cPos=ImGui::GetCursorScreenPos();
    render3DView(ci,count,cPos,ImVec2(halfW-4,topH-4));

    ImPlotSpec specPath(ImPlotProp_LineColor,ImVec4(0.5f,0.5f,0.5f,0.5f),ImPlotProp_LineWeight,2.0f);
    ImPlotSpec specPt(ImPlotProp_Marker,ImPlotMarker_Circle,ImPlotProp_MarkerSize,8.0f,ImPlotProp_MarkerFillColor,ImVec4(1,0.2f,0.2f,1));

    ImGui::SetCursorScreenPos(ImVec2(cPos.x+halfW,cPos.y));
    ImGui::BeginChild("##XZ",ImVec2(halfW,topH-4));
    if(ImPlot::BeginPlot("X-Z Ansicht",ImVec2(-1,-1))){
        ImPlot::SetupAxes("X [mm]","Z [mm]",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Pfad",m_result.x.s.data(),m_result.z.s.data(),count,specPath);
        float px=m_result.x.s[ci],pz=m_result.z.s[ci];
        ImPlot::PlotScatter("Pos",&px,&pz,1,specPt);
        ImPlot::EndPlot();
    }
    ImGui::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(cPos.x,cPos.y+topH));
    ImGui::BeginChild("##YZ",ImVec2(halfW-4,avail.y-topH));
    if(ImPlot::BeginPlot("Y-Z Ansicht",ImVec2(-1,-1))){
        ImPlot::SetupAxes("Y [mm]","Z [mm]",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Pfad",m_result.y.s.data(),m_result.z.s.data(),count,specPath);
        float py=m_result.y.s[ci],pz2=m_result.z.s[ci];
        ImPlot::PlotScatter("Pos",&py,&pz2,1,specPt);
        ImPlot::EndPlot();
    }
    ImGui::EndChild();

    ImGui::SetCursorScreenPos(ImVec2(cPos.x+halfW,cPos.y+topH));
    ImGui::BeginChild("##XY",ImVec2(halfW,avail.y-topH));
    if(ImPlot::BeginPlot("X-Y Ansicht",ImVec2(-1,-1))){
        ImPlot::SetupAxes("X [mm]","Y [mm]",ImPlotAxisFlags_AutoFit,ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Pfad",m_result.x.s.data(),m_result.y.s.data(),count,specPath);
        float px2=m_result.x.s[ci],py2=m_result.y.s[ci];
        ImPlot::PlotScatter("Pos",&px2,&py2,1,specPt);
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
    ImGui::End();
}

void Gui::render() {
    renderConfigPanel();
    renderPlots();
    renderAnimation();
}
