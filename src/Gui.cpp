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
            if (ImGui::Button("Pick-Positionen konfigurieren...", ImVec2(-1, 30))) {
                m_showPickPopup = true;
                m_previewReady = false;
                ImGui::OpenPopup("PickGenerator");
            }
            // Kompakte Anzeige der aktuellen Picks
            if (ImGui::BeginChild("##PickList", ImVec2(-1, 120), true)) {
                for (int i = 0; i < (int)pp.pickPositions.size(); ++i) {
                    ImGui::Text("%d: (%.1f, %.1f, %.1f)", i + 1,
                        pp.pickPositions[i].x, pp.pickPositions[i].y, pp.pickPositions[i].z);
                }
            }
            ImGui::EndChild();
            ImGui::Spacing();

            if (ImGui::Button("Simulation Starten##PP", ImVec2(-1, 30))) {
                m_result = Kinematics::calculatePickPlace(pp);
                m_hasResult = true;
                m_animationTime = 0; m_isPlaying = false;
            }

            renderPickPopup();

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

void Gui::renderPickPreview3D(ImVec2 canvasPos, ImVec2 canvasSize) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(canvasPos);
    ImGui::InvisibleButton("##PickPreview3D", canvasSize);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        m_prevRotZ += d.x * 0.01f;
        m_prevRotX += d.y * 0.01f;
        if (m_prevRotX < -1.5f) m_prevRotX = -1.5f;
        if (m_prevRotX > 1.5f) m_prevRotX = 1.5f;
    }
    if (ImGui::IsItemHovered()) {
        float w = ImGui::GetIO().MouseWheel;
        if (w != 0) { m_prevZoom *= (1+w*0.1f); if(m_prevZoom<0.1f)m_prevZoom=0.1f; if(m_prevZoom>10)m_prevZoom=10; }
    }
    dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x+canvasSize.x, canvasPos.y+canvasSize.y), IM_COL32(25,25,35,255));
    dl->PushClipRect(canvasPos, ImVec2(canvasPos.x+canvasSize.x, canvasPos.y+canvasSize.y), true);

    float cz=cosf(m_prevRotZ),sz=sinf(m_prevRotZ),cx=cosf(m_prevRotX),sx=sinf(m_prevRotX);
    // Bounds
    float mnX=1e9f,mxX=-1e9f,mnY=1e9f,mxY=-1e9f,mnZ=1e9f,mxZ=-1e9f;
    for (auto& p : m_previewPicks) {
        if(p.x<mnX)mnX=p.x; if(p.x>mxX)mxX=p.x;
        if(p.y<mnY)mnY=p.y; if(p.y>mxY)mxY=p.y;
        if(p.z<mnZ)mnZ=p.z; if(p.z>mxZ)mxZ=p.z;
    }
    auto& pl = m_config.pp.placePosition;
    if(pl.x<mnX)mnX=pl.x; if(pl.x>mxX)mxX=pl.x;
    if(pl.y<mnY)mnY=pl.y; if(pl.y>mxY)mxY=pl.y;
    if(pl.z<mnZ)mnZ=pl.z; if(pl.z>mxZ)mxZ=pl.z;
    float pad=10; mnX-=pad;mxX+=pad;mnY-=pad;mxY+=pad;mnZ-=pad;mxZ+=pad;
    float midX=(mnX+mxX)*0.5f,midY=(mnY+mxY)*0.5f,midZ=(mnZ+mxZ)*0.5f;
    float span=fmaxf(fmaxf(mxX-mnX,mxY-mnY),fmaxf(mxZ-mnZ,1.0f));
    float scale=fminf(canvasSize.x,canvasSize.y)*0.35f/span*m_prevZoom;
    ImVec2 center(canvasPos.x+canvasSize.x*0.5f,canvasPos.y+canvasSize.y*0.5f);
    auto project=[&](float px,float py,float pz)->ImVec2{
        float dx=px-midX,dy=py-midY,dz=pz-midZ;
        float rx=cz*dx-sz*dy,ry=sz*dx+cz*dy;
        float fz=sx*ry+cx*dz;
        return ImVec2(center.x+rx*scale,center.y-fz*scale);
    };
    // Grid
    float bx[2]={mnX,mxX},by[2]={mnY,mxY},bz[2]={mnZ,mxZ};
    ImU32 gc=IM_COL32(60,60,80,100);
    for(int i=0;i<2;++i)for(int j=0;j<2;++j){
        dl->AddLine(project(bx[0],by[i],bz[j]),project(bx[1],by[i],bz[j]),gc);
        dl->AddLine(project(bx[i],by[0],bz[j]),project(bx[i],by[1],bz[j]),gc);
        dl->AddLine(project(bx[i],by[j],bz[0]),project(bx[i],by[j],bz[1]),gc);
    }
    // Axes
    float axL=span*0.4f; ImVec2 o0=project(midX,midY,midZ);
    dl->AddLine(o0,project(midX+axL,midY,midZ),IM_COL32(255,80,80,180),1.5f);
    dl->AddLine(o0,project(midX,midY+axL,midZ),IM_COL32(80,255,80,180),1.5f);
    dl->AddLine(o0,project(midX,midY,midZ+axL),IM_COL32(80,80,255,180),1.5f);
    dl->AddText(project(midX+axL,midY,midZ),IM_COL32(255,100,100,255),"X");
    dl->AddText(project(midX,midY+axL,midZ),IM_COL32(100,255,100,255),"Y");
    dl->AddText(project(midX,midY,midZ+axL),IM_COL32(100,100,255,255),"Z");

    // Pick-Punkte (nummeriert)
    for (int i = 0; i < (int)m_previewPicks.size(); ++i) {
        ImVec2 p = project(m_previewPicks[i].x, m_previewPicks[i].y, m_previewPicks[i].z);
        dl->AddCircleFilled(p, 5, IM_COL32(50,200,255,255));
        char buf[16]; snprintf(buf, sizeof(buf), "%d", i+1);
        dl->AddText(ImVec2(p.x+6, p.y-6), IM_COL32(200,220,255,255), buf);
    }
    // Place-Punkt
    ImVec2 pp2 = project(pl.x, pl.y, pl.z);
    dl->AddCircleFilled(pp2, 7, IM_COL32(255,180,50,255));
    dl->AddText(ImVec2(pp2.x+8, pp2.y-6), IM_COL32(255,200,100,255), "Place");

    dl->AddText(ImVec2(canvasPos.x+5,canvasPos.y+5),IM_COL32(180,180,180,180),"Maus ziehen = Drehen");
    dl->PopClipRect();
}

void Gui::renderPickPopup() {
    ImVec2 popupSize(720, 580);
    ImGui::SetNextWindowSize(popupSize, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("PickGenerator", nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Pick-Positionen Grid-Generator");
        ImGui::Separator();

        ImGui::Text("First Pick (oberste Ebene, erste Position):");
        ImGui::DragFloat("First X", &m_firstPick.x, 0.5f, 0.0f, 0.0f, "%.1f mm");
        ImGui::SameLine(); ImGui::DragFloat("First Y", &m_firstPick.y, 0.5f, 0.0f, 0.0f, "%.1f mm");
        ImGui::SameLine(); ImGui::DragFloat("First Z", &m_firstPick.z, 0.5f, 0.0f, 0.0f, "%.1f mm");

        ImGui::Text("Last Pick (unterste Ebene, letzte Position):");
        ImGui::DragFloat("Last X", &m_lastPick.x, 0.5f, 0.0f, 0.0f, "%.1f mm");
        ImGui::SameLine(); ImGui::DragFloat("Last Y", &m_lastPick.y, 0.5f, 0.0f, 0.0f, "%.1f mm");
        ImGui::SameLine(); ImGui::DragFloat("Last Z", &m_lastPick.z, 0.5f, 0.0f, 0.0f, "%.1f mm");

        ImGui::Separator();

        // Ebene erkennen
        float dx = std::abs(m_lastPick.x - m_firstPick.x);
        float dy = std::abs(m_lastPick.y - m_firstPick.y);
        float dz = std::abs(m_lastPick.z - m_firstPick.z);
        bool xSame = dx < 0.1f;
        bool ySame = dy < 0.1f;

        if (xSame && ySame) {
            snprintf(m_detectedPlane, sizeof(m_detectedPlane), "Nur Z-Achse (Stapel). X=Y=const.");
        } else if (xSame) {
            snprintf(m_detectedPlane, sizeof(m_detectedPlane), "Y-Z Ebene erkannt (X = %.1f mm = const.)", m_firstPick.x);
        } else if (ySame) {
            snprintf(m_detectedPlane, sizeof(m_detectedPlane), "X-Z Ebene erkannt (Y = %.1f mm = const.)", m_firstPick.y);
        } else {
            snprintf(m_detectedPlane, sizeof(m_detectedPlane), "X-Y-Z Volumen (beide Achsen variieren)");
        }
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.6f, 1.0f), "%s", m_detectedPlane);

        ImGui::DragFloat("Abstand horizontal [mm]", &m_pitchHoriz, 0.5f, 1.0f, 1000.0f, "%.1f mm");
        ImGui::DragFloat("Abstand Z (Ebenen) [mm]", &m_pitchZ, 0.5f, 1.0f, 1000.0f, "%.1f mm");

        // Berechne Anzahl
        int nHoriz = 1, nHorizY = 1, nZ = 1;
        if (!xSame && m_pitchHoriz > 0.1f) nHoriz = std::max(1, (int)std::round(dx / m_pitchHoriz) + 1);
        if (!ySame && m_pitchHoriz > 0.1f) nHorizY = std::max(1, (int)std::round(dy / m_pitchHoriz) + 1);
        if (dz > 0.1f && m_pitchZ > 0.1f) nZ = std::max(1, (int)std::round(dz / m_pitchZ) + 1);

        int total = nHoriz * nHorizY * nZ;
        ImGui::Text("Ergibt: %d x %d x %d = %d Positionen", nHoriz, nHorizY, nZ, total);

        if (ImGui::Button("Vorschau generieren", ImVec2(-1, 28))) {
            m_previewPicks.clear();
            // Reihenfolge: Spalte fuer Spalte (horiz), jede Spalte von oben nach unten
            for (int ix = 0; ix < nHoriz; ++ix) {
                for (int iy = 0; iy < nHorizY; ++iy) {
                    for (int iz = 0; iz < nZ; ++iz) {
                        float fx = (nHoriz > 1) ? (float)ix / (nHoriz - 1) : 0.0f;
                        float fy = (nHorizY > 1) ? (float)iy / (nHorizY - 1) : 0.0f;
                        float fz = (nZ > 1) ? (float)iz / (nZ - 1) : 0.0f;
                        Vec3 p;
                        p.x = m_firstPick.x + fx * (m_lastPick.x - m_firstPick.x);
                        p.y = m_firstPick.y + fy * (m_lastPick.y - m_firstPick.y);
                        p.z = m_firstPick.z + fz * (m_lastPick.z - m_firstPick.z);
                        m_previewPicks.push_back(p);
                    }
                }
            }
            m_previewReady = true;
        }

        if (m_previewReady && !m_previewPicks.empty()) {
            ImGui::Text("Vorschau: %d Positionen (Blau=Pick, Orange=Place)", (int)m_previewPicks.size());
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float previewH = avail.y - 35;
            ImVec2 cPos = ImGui::GetCursorScreenPos();
            renderPickPreview3D(cPos, ImVec2(avail.x, previewH));
            ImGui::SetCursorScreenPos(ImVec2(cPos.x, cPos.y + previewH + 5));

            float btnW = (avail.x - 10) * 0.5f;
            if (ImGui::Button("Uebernehmen", ImVec2(btnW, 28))) {
                m_config.pp.pickPositions = m_previewPicks;
                m_previewReady = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Zurueck", ImVec2(btnW, 28))) {
                m_previewReady = false;
            }
        }
        ImGui::EndPopup();
    }
}

void Gui::render() {
    renderConfigPanel();
    renderPlots();
    renderAnimation();
}
