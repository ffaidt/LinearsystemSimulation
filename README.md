# Anti-Gravity Kinematics

Ein C++ Softwareprojekt zur Kinematik-Simulation eines 3D-Linearachsensystems im Punkt-zu-Punkt-Betrieb (PTP).

## Funktionsumfang

Dieses Programm berechnet und visualisiert die Trajektorie einer 3-Achsen-Maschine (X, Y, Z) von einer Startposition zu einer Zielposition. Dabei kommen moderne Bahnplanungs-Algorithmen zum Einsatz:

- **Trapezförmige Geschwindigkeitsprofile:** Jede Achse verwendet Beschleunigungs-, Konstantfahrt- und Verzögerungsphasen.
- **Isolierter Z-Hub:** Aus Sicherheitsgründen (Kollisionsvermeidung) wird die Z-Achse streng getrennt verfahren, bevor die Bewegung in der X/Y-Ebene beginnt.
- **XY-Interpolation:** Die X- und Y-Achsen verfahren synchronisiert. Es wird eine lineare Pfadlänge berechnet und die Geschwindigkeits-/Beschleunigungs-Limits der Einzelachsen so skaliert, dass eine perfekte Gerade abgefahren wird.

## Benutzeroberfläche

Die GUI wurde komplett in **C++** mit [Dear ImGui](https://github.com/ocornut/imgui) und [ImPlot](https://github.com/epezent/implot) geschrieben und bietet:

- **Dynamische Diagramme:** Darstellung von Position ($s$), Geschwindigkeit ($v$) und Beschleunigung ($a$) über die Zeit.
- **Echtzeit-Animation:** Ein 2x2 Layout zur Visualisierung der Maschinenbewegung.
  - **Iso Ansicht:** Isometrische 3D-Projektion der Fahrkurve.
  - **X-Z, Y-Z, X-Y Ansichten:** Präzise 2D-Darstellungen aus jedem Blickwinkel.
- **Persistenz:** Konfigurierte Achsparameter ($v_{max}$, $a_{max}$) werden automatisch in einer `config.json` lokal gespeichert.

## Kompilieren & Ausführen

Das Projekt verwendet **CMake**. Alle Abhängigkeiten (GLFW, ImGui, ImPlot, nlohmann_json) werden automatisch beim Build-Prozess heruntergeladen. Es ist keine manuelle Installation von Bibliotheken notwendig!

### Voraussetzungen
- Ein C++20 fähiger Compiler (z. B. GCC/MinGW, MSVC, Clang)
- CMake

### Build-Schritte
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Anschließend kann die Datei `AntiGravityKinematics` im Build-Ordner ausgeführt werden.
