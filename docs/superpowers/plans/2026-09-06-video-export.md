# Video-Export: Umsetzungsplan für KeyWave

> Für die spätere Umsetzung: Meilensteine einzeln mit `superpowers:executing-plans` bearbeiten und nach jedem Meilenstein die Abnahmekriterien prüfen. Dieser Plan ist ein Architekturvorschlag auf Meilensteinebene; er autorisiert noch keine Implementierung.

**Ziel:** Ganze Lieder oder ausgewählte Ausschnitte als Video exportieren, mit 30/60 FPS, Qualitätsstufen, Auflösung, Seitenverhältnis und optionalem Ton.

**Architektur:** Szenen zu festen Zeitpunkten erzeugen, in ein eigenes OpenGL-Renderziel zeichnen und die Pixel an einen Video-Encoder übergeben. Audio mit einer separaten FluidSynth-Instanz offline erzeugen. Ein Export-Job koordiniert beide Pfade anhand derselben Zeitdefinition.

**Technik:** Bestehendes C++20, CMake, OpenGL 3.3, Dear ImGui, FluidSynth und Catch2; vorgeschlagene neue Laufzeitabhängigkeit: FFmpeg mit H.264- und AAC-Unterstützung.

**Stand:** 06.09.2026. Grundlage sind die Anforderungen aus dem Gespräch und die unten untersuchten Codepfade. Produktentscheidungen sind als Vorschlag beschrieben, nicht als bereits abgestimmt.

## 1. Befund im bestehenden Projekt

- `src/fallingnotes/PianoRollSceneBuilder.cpp`: `build()` akzeptiert bereits einen expliziten Zeitpunkt und liefert eine `RenderScene`. Der Export braucht daher keinen zweiten Piano-Roll-Renderer.
- `src/app/VisualizerController.cpp`: bindet die Szenenerzeugung an den Live-Transport. Der Offset `firstNoteStartSeconds - lookAheadSeconds` sorgt für den visuellen Vorlauf und muss beim Export wiederverwendet werden.
- `src/app/Application.cpp`: steuert Echtzeit-Update, OpenGL, ImGui und Fenstertausch in einer Schleife. Export-Frames dürfen nicht von deren gemessenen Zeitabständen abhängen.
- `src/render_opengl/OpenGLRendererBackend.cpp`: zeichnet mit konfigurierbarer Framebuffer-Größe, bietet aber noch keine eigene Offscreen-Zielverwaltung oder Pixel-Auslese.
- `src/render/RendererView.cpp`: bildet Weltkoordinaten auf die Ausgabe ab. Ein anderes Seitenverhältnis allein garantiert noch keine ansprechende Hochkant-Komposition.
- `src/audio/FluidSynthPianoSynth.cpp`: erstellt aktuell Synth und Audio-Gerät gemeinsam. Der Export benötigt eine Instanz ohne Audio-Gerät.
- `src/audio/TimelineAudioScheduler.cpp`: enthält Noten-/Pedal-Reihenfolge und Seek-Verhalten. Sein Echtzeit-Update liefert jedoch noch keinen samplebasierten Offline-Audiostrom; erneutes Anspielen gehaltener Noten rekonstruiert auch nicht den bereits entstandenen Hall.
- `src/audio/PianoSynth.hpp`: hat keinen allgemeinen Verfügbarkeitsstatus. Ob Offline-Audio funktioniert, sollte als eigene Export-Fähigkeit ermittelt werden.
- `src/app/AppSettings*`, `src/ui/TransportSeek*` und `src/platform/MidiFileDialog.cpp`: liefern Muster für persistierte Einstellungen, Zeitauswahl und native Dateidialoge.
- `tests/` und `tests/CMakeLists.txt`: Catch2-Tests und Architekturprüfungen sind vorhanden. Neue Logiktests hier integrieren; echte GPU-/Encoder-Tests separat kennzeichnen.

## 2. Empfohlener Ansatz und Alternativen

**Empfehlung: Offline-Rendering plus externer FFmpeg-Prozess.** Die App erzeugt jedes Bild gezielt. Die Renderdauer darf kürzer oder länger als die Lieddauer sein. FFmpeg übernimmt die Kodierung. Die Prozessgrenze hält Codec-Abhängigkeiten aus dem Rendering-Kern heraus.

**Alternative: FFmpeg-Bibliotheken direkt einbinden.** Bietet engere Kontrolle über Datenströme, erhöht aber Integrations-, Ressourcenverwaltungs- und Paketierungsaufwand. Erst sinnvoll, wenn die Prozesslösung konkrete Grenzen erreicht.

**Alternative: Live-Fensteraufnahme.** Als Weg zum geforderten Feature ungeeignet: an Laufzeit und Fensterauflösung gebunden, mit möglichem Bildverlust und UI-Einblendungen.

FFmpeg unterstützt Rohvideo-Eingaben sowie Codec-Parameter; die tatsächlichen Encoder hängen vom verwendeten Build ab. Grundlage: [FFmpeg-Dokumentation](https://ffmpeg.org/ffmpeg.html) und [Codec-Dokumentation](https://www.ffmpeg.org/ffmpeg-codecs.html). FluidSynth bietet die direkte Ausgabe von Audiosamples ohne Audio-Treiber: [Audio Rendering](https://www.fluidsynth.org/api/group__audio__rendering.html).

## 3. Vorgeschlagenes Verhalten der ersten vollständigen Version

| Einstellung | Verhalten |
|---|---|
| Format | MP4 mit H.264; mit Ton zusätzlich AAC, sonst keine Audiospur |
| Framerate | 30 oder 60; Standard 60 |
| Qualität | Diskreter Regler: Medium, High, Best; Standard High |
| Auflösung | 720p, 1080p, 4K; Standard 1080p; tatsächliche Pixelmaße immer anzeigen |
| Seitenverhältnis | 16:9, 9:16, 1:1; Standard 16:9 |
| Ton | Checkbox; standardmäßig aktiv, wenn Offline-Synth und SoundFont verfügbar; sonst deaktiviert mit verständlicher Begründung |
| Zeitbereich | „Ganzes Lied“ oder „Ausschnitt“; Doppelslider für Start/Ende und präzise Zeitfelder |
| Gestaltung | Beim Start eingefrorene aktuelle Visualisierungseinstellungen; Vorschau entspricht der Export-Komposition |
| Wiedergabetempo | Aktuellen Geschwindigkeitsfaktor beim Start übernehmen und im Dialog anzeigen |
| Während Export | Live-Wiedergabe pausieren; Fortschritt und Abbrechen bleiben bedienbar; ein Export gleichzeitig |

Auflösungspresets werden explizit zugeordnet, damit „1080p hochkant“ eindeutig ist:

| Preset | 16:9 | 9:16 | 1:1 |
|---|---|---|---|
| 720p | 1280 × 720 | 720 × 1280 | 720 × 720 |
| 1080p | 1920 × 1080 | 1080 × 1920 | 1080 × 1080 |
| 4K | 3840 × 2160 | 2160 × 3840 | 2160 × 2160 |

Quadratische Formate erhalten im Dialog zusätzlich ihre konkrete Größe; „4K“ bezeichnet dort die Preset-Familie. Zunächst keine frei eingebbaren Abmessungen.

Qualität steuert zunächst nur die Videokompression. Als zu überprüfende Startwerte für libx264: Medium CRF 23, High CRF 18, Best CRF 15, jeweils Encoder-Preset `medium`. „Best“ bedeutet nicht verlustfrei. An feinen Tastenkanten, Konturen und schnellen Notenbewegungen prüfen und bei Bedarf anpassen. Keine zusätzlichen Auflösungsänderungen durch den Qualitätsregler.

### Zeitvertrag

- Der Bereich bezieht sich auf dieselbe Playback-Zeit wie die Transportanzeige, einschließlich des bestehenden visuellen Vorlaufs.
- Sei `a` der Bereichsstart, `b` das Ende und `r > 0` der eingefrorene Geschwindigkeitsfaktor. Ohne Nachlauf gilt `D = (b-a)/r`.
- Frame `i` erhält den Quellzeitpunkt `offset + a + r*i/fps`; Zeitwerte aus ganzzahligen Indizes berechnen, nicht wiederholt Gleitkomma-Schritte addieren.
- `N = ceil(D*fps)` Frames; letzter Frame beginnt vor dem Bereichsende. Angezeigte tatsächliche Ausgabedauer: `N/fps`. Die Rundungsdifferenz bleibt kleiner als ein Frame.
- Für Audio dieselbe Abbildung mit Sample-Indizes verwenden; vorgeschlagen sind 48 kHz Stereo. Die Ausgabesamplezahl an die quantisierte Videodauer angleichen.
- Vollständiges Lied: vorgeschlagen zwei Sekunden expliziter Ausklang in Ausgabezeit, auch ohne Ton, damit die Laufzeit nicht vom Ton-Schalter abhängt. Am musikalischen Ende Pedal lösen und Stimmen natürlich ausklingen lassen; Schlussbild halten. Begrenzten Nachlauf sichtbar in der Dauer ausweisen.
- Ausschnitt: kein automatischer Nachlauf; Ende am gewählten Schnitt, ggf. mit weniger als einem Frame Rundung. Längere natürliche Ausklänge und ein frei einstellbarer Nachlauf sind spätere Erweiterungen.
- Bereits vor dem Ausschnitt klingendes Audio durch Offline-Vorlauf herstellen: vom Playback-Anfang bis `a` synthetisieren und die Samples verwerfen. Das ist langsamer als Seek, bildet aber Hüllkurven, Sustain und Effekte nachvollziehbar ab.

## 4. Meilensteine

### M1 – Export-Modell und gemeinsame Zeitbasis

**Ergebnis:** Ein validierter Exportauftrag, dessen Zeiten und Framezahl ohne GPU oder Encoder prüfbar sind.

**Dateien:** Neu `src/export/VideoExportSettings.{hpp,cpp}`, `src/export/ExportTiming.{hpp,cpp}`, `src/export/ExportSnapshot.hpp`; Anpassung `src/app/VisualizerController.{hpp,cpp}`, `src/playback/PlaybackTransport.{hpp,cpp}`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`; neu `tests/export/ExportTimingTests.cpp` und `VideoExportSettingsTests.cpp`.

- [ ] Typen für FPS, Qualität, Auflösung, Seitenverhältnis, Ton und Zeitbereich definieren; Audio- und Videoeinstellungen von Fenstereinstellungen trennen.
- [ ] Unveränderlichen Snapshot aus Timeline, Szenenkonfiguration, Hintergrund, Offset und Geschwindigkeitsfaktor bereitstellen. Kein Zugriff auf veränderlichen Live-Transport während des Exports.
- [ ] Gemeinsame Timing-Berechnung einschließlich Frame-Rundung, Vorlauf und Ausklang implementieren; dafür den Geschwindigkeitsfaktor lesbar machen.
- [ ] Ungültige bzw. nicht endliche Werte, leere Timeline, `start >= end`, Bereiche außerhalb des Liedes und Zählerüberläufe ablehnen.

**Abnahme:** Zehn Sekunden bei normalem Tempo ergeben ohne Nachlauf exakt 300/600 Frames. Tests decken halbes/doppeltes Tempo, Tempoereignisse der MIDI-Datei, Vorlauf, nicht framegenaue Enden und Grenzwerte ab. Bestehende Playback-Tests bleiben grün.

**Abhängigkeit:** Keine. Aufwand: klein bis mittel.

### M2 – Einzelbilder unabhängig vom Fenster rendern

**Ergebnis:** Eine Szene kann in einer gewählten Zielgröße als Pixelpuffer erzeugt werden.

**Dateien:** Neu `src/render_opengl/OffscreenRenderTarget.{hpp,cpp}`; Anpassung `src/render_opengl/OpenGLRendererBackend.{hpp,cpp}`; neu `tests/render/OffscreenRenderTargetIntegrationTests.cpp` als separater GPU-Test.

- [ ] Eigenes Framebuffer-Objekt mit Farbtextur und klarer Ressourcenlebensdauer anlegen; GPU-Größenlimits und Vollständigkeit prüfen.
- [ ] Vorhandene `RenderScene` in dieses Ziel zeichnen, ohne ImGui oder Fensteraufnahme.
- [ ] RGBA-Pixel auslesen; Zeilenrichtung, Stride und Pack-Alignment definieren.
- [ ] Framebuffer-Bindung, Viewport und Renderer-Größe nach dem Export-Schritt wiederherstellen. OpenGL-Aufrufe zunächst auf dem bestehenden Kontext-Thread ausführen.

**Abnahme:** Ein asymmetrisches Testbild kommt in korrekter Orientierung und Farbe an; 1080p-Export funktioniert bei kleinerem Fenster; anschließende Live-Ansicht ist korrekt. 4K entweder erfolgreich oder mit konkretem GPU-Fehler abgelehnt.

**Abhängigkeit:** M1. Aufwand: mittel.

### M3 – Erster vollständiger Videoexport ohne Ton

**Ergebnis:** Ein ganzes Lied wird mit festem Profil 1080p, 16:9, 30 FPS als abspielbares MP4 erzeugt. Das ist der erste technische MVP.

**Dateien:** Neu `src/export/VideoEncoder.hpp`, `src/export_ffmpeg/FfmpegVideoEncoder.{hpp,cpp}`, `src/platform/ChildProcess.{hpp,cpp}`, `src/app/VideoExportController.{hpp,cpp}`; minimale Anbindung in `src/app/Application.cpp`; neu `tests/export/VideoExportControllerTests.cpp`, `tests/export/FfmpegVideoEncoderIntegrationTests.cpp`; CMake-Anbindung.

- [ ] Encoder-Schnittstelle mit Start, Frame-Übergabe, Abschluss und Abbruch definieren; Fake-Encoder für Orchestrierungstests bereitstellen.
- [ ] FFmpeg explizit finden und benötigten Encoder prüfen. Unter Windows ohne sichtbares Konsolenfenster starten; Argumente ohne Shell-Kommandoverkettung übergeben, UTF-8-/Unicode-Pfade korrekt behandeln.
- [ ] Frames in Reihenfolge als Rohdaten streamen; keine Sammlung aller Bilder im RAM oder als Bildsequenz auf der Platte.
- [ ] Begrenzte Queue zwischen GPU-Erzeugung und Pipe-Writer einsetzen. Bei voller Queue den nächsten Frame später erzeugen; UI nicht durch blockierende Pipe-Schreiboperationen anhalten. stderr parallel leeren und Fehler begrenzt sammeln.
- [ ] Zunächst minimalen Exportknopf mit Status und Abbruch anbinden. Temporäre MP4 im Zielverzeichnis erzeugen; erst nach erfolgreichem Prozessabschluss als fertiges Ergebnis übernehmen.

**Abnahme:** Testlied hat exakt erwartete Bildzahl, FPS und Dauer sowie keine Audiospur. Encoder-Absturz, fehlender Encoder und ein blockierter Verbraucher führen zu kontrolliertem Fehler bzw. Abbruch. Der Speicherbedarf wächst nicht mit der Liedlänge.

**Abhängigkeit:** M1, M2. Aufwand: groß; intern Encoder-Adapter und App-Anbindung als getrennte Änderungen bearbeiten.

### M4 – Framerate, Qualität, Auflösung und Hochkant

**Ergebnis:** Alle visuellen Exportoptionen funktionieren und besitzen eine zutreffende Vorschau.

**Dateien:** Neu `src/export/ExportComposition.{hpp,cpp}`, `tests/export/ExportCompositionTests.cpp`; Anpassung Export-Settings, Encoder, `src/app/VisualizationSettingsAdapters.{hpp,cpp}`; bei Bedarf gezielte Erweiterung `src/fallingnotes/PianoRollSceneBuilder.{hpp,cpp}`.

- [ ] 30/60 FPS und die neun Auflösungs-/Formatkombinationen aus der Tabelle aktivieren.
- [ ] Qualität auf geprüfte Encoder-Einstellungen abbilden; keine Bindung an Live-FPS-Limit oder VSync.
- [ ] Hochkant als eigene Komposition berechnen: gewählten Tastenumfang erhalten, Tastatur unten ausrichten, Tastenproportionen erhalten und den Notenbereich an die verfügbare Höhe anpassen. Keine automatische Beschneidung der ausgewählten Tasten.
- [ ] Vorschau und Export mit derselben Kompositionsfunktion erzeugen. Pixelbasierte Konturen/Radien auf eine definierte Referenzgröße beziehen, damit sie bei 4K nicht relativ verschwinden.
- [ ] RGB-zu-YUV-Konvertierung und Farbbereich explizit festlegen und anhand bekannter Farben prüfen.

**Abnahme:** 9:16 ist ein natives Hochkantbild mit vollständigem gewähltem Tastenumfang; Vorschau und Export stimmen überein. Automatisierte Parameterprüfung aller 54 Kombinationen aus FPS, Qualität, Auflösung und Format; reale Kurzexports mindestens für 720p/30, 1080p/60 hochkant und 4K/60.

**Abhängigkeit:** M3. Aufwand: mittel bis groß, insbesondere Bildaufteilung.

### M5 – Ganzes Lied oder Ausschnitt auswählen

**Ergebnis:** Ein eigener Exportdialog steuert den vollständigen stummen Export einschließlich Bereichsauswahl.

**Dateien:** Neu `src/ui/VideoExportDialog.{hpp,cpp}`, `src/ui/ExportRangeControls.{hpp,cpp}`, `src/platform/VideoSaveDialog.{hpp,cpp}`, `tests/ui/ExportRangeControlsTests.cpp`; Anpassung `src/app/Application.{hpp,cpp}` und Export-Controller.

- [ ] Auswahl „Ganzes Lied / Ausschnitt“ und Doppelslider für Start/Ende mit Zeitfeldern ergänzen; transportnahe Formatierung wiederverwenden.
- [ ] „Aktuelle Position als Start/Ende“ und Zurücksetzen auf das ganze Lied anbieten.
- [ ] Vorschau beim Scrubbing aus dem Export-Snapshot erzeugen, ohne hörbare Live-Seeks.
- [ ] Zielpfad, tatsächliche Pixelgröße, Tempo, Ausgabe-Dauer und Bildzahl anzeigen; ungültige Kombinationen vor Start erklären.
- [ ] Neue Datei-Auswahl setzt den Bereich passend zurück. Zuletzt gewählte Formatoptionen dürfen erhalten bleiben.

**Abnahme:** Ein Bereich mitten in gehaltenen Noten beginnt mit dem richtigen visuellen Zustand. Start/Ende lassen sich nicht vertauschen, Tastaturbedienung ist möglich, und „Ganzes Lied“ umfasst unabhängig von der aktuellen Playhead-Position das ganze Lied.

**Abhängigkeit:** M4. Aufwand: mittel.

### M6 – Offline-Audio und synchrones MP4

**Ergebnis:** Vollständige Lieder und Ausschnitte können mit dem verfügbaren Piano-Sound exportiert werden.

**Dateien:** Neu `src/audio/OfflinePianoRenderer.{hpp,cpp}`, `src/audio/TimelineAudioEvents.{hpp,cpp}`; Anpassung `src/audio/TimelineAudioScheduler.{hpp,cpp}`, Export-Controller und FFmpeg-Adapter; neu `tests/audio/OfflinePianoRendererTests.cpp` und `tests/export/AudioVideoSyncIntegrationTests.cpp`.

- [ ] Noten-/Pedal-Ereignisaufbereitung aus dem Live-Scheduler gemeinsam nutzbar machen; dessen Reihenfolge und bisheriges Verhalten erhalten.
- [ ] Separate FluidSynth-Instanz ohne Audio-Gerät erzeugen. Gleichen SoundFont und explizite Synth-Einstellungen verwenden; Verfügbarkeit eigenständig prüfen.
- [ ] Audio samplebasiert erzeugen und Renderblöcke an Ereignisgrenzen teilen. Ereignisse nicht nur einmal je Videoframe auslösen. FluidSynth-interne Blockgranularität berücksichtigen und die tatsächlich erreichbare Timing-Toleranz messen.
- [ ] Bei Ausschnitten den Synth vom Anfang vorlaufen lassen und die Samples vor dem Schnitt verwerfen; Vorbereitungsphase im Fortschritt anzeigen.
- [ ] Für die erste Version PCM-Audio in eine temporäre WAV-Datei streamen, anschließend mit dem Video in MP4 zusammenführen. Audio nur einmal nach AAC kodieren; Videospur beim Zusammenführen kopieren. WAV-Größenlimit vorab prüfen und zu große Aufträge klar ablehnen oder RF64 implementieren.
- [ ] Sampleanzahl, Nachlauf und AAC-Padding auf die gemeinsame Ausgabetimeline abstimmen. Fehlender SoundFont deaktiviert Ton; ein Fehler nach einem ausdrücklich gestarteten Tonexport darf nicht stillschweigend ein stummes Erfolgsergebnis liefern.

**Abnahme:** Test-MIDI mit klaren Anschlägen am Anfang, in der Mitte und am Ende zeigt keine wachsende A/V-Abweichung; dekodierten Output inklusive AAC-Verzögerung messen, Ziel maximal ein Videoframe. Ein Ausschnitt mit Sustain und Hall entspricht bei festgelegten Synth-Einstellungen dem entsprechenden PCM-Bereich des vollständigen Renders. Build mit `KEYWAVE_ENABLE_AUDIO=OFF` exportiert weiterhin stumm.

**Abhängigkeit:** M5. Aufwand: groß; Ereignismodell, Offline-PCM und Zusammenführen als drei separat prüfbare Änderungen.

### M7 – Verlässliche Bedienung und Fehlerbehandlung

**Ergebnis:** Das komplette Feature ist im Alltag bedienbar und nach Fehlern erneut nutzbar.

**Dateien:** Anpassung Export-Controller, Exportdialog, `src/app/AppSettings.{hpp,cpp}`, `src/app/AppSettingsSerializer.{hpp,cpp}`, `src/app/Application.cpp`; entsprechende Controller- und Serialisierungstests erweitern.

- [ ] Zustände „Vorbereiten“, „Audio erzeugen“, „Video rendern“, „Abschließen“, „Fertig“, „Abgebrochen“, „Fehlgeschlagen“ explizit führen; Fortschritt pro Phase und vorsichtige Restzeitschätzung anzeigen.
- [ ] Abbruch in jeder Phase implementieren, auch bei voller Queue/blockierter Pipe. Prozesse beenden, Worker zusammenführen und ausschließlich jobeigene temporäre Dateien entfernen.
- [ ] Live-Transport bei Start pausieren, seine Position erhalten und nach Abschluss pausiert wieder freigeben. Änderungen/Liedwechsel während des Jobs sperren oder über den unveränderlichen Snapshot sicher entkoppeln.
- [ ] Vorhandene Zieldateien erst nach ausdrücklicher Überschreibauswahl ersetzen; alte Datei bei Fehler erhalten. App-Schließen während Export kontrolliert behandeln.
- [ ] Speicherplatzfehler, ungültige Pfade, fehlende Schreibrechte und verschwundenen SoundFont nachvollziehbar anzeigen; erneuten Export nach Fehler erlauben.
- [ ] FPS, Qualität, Auflösung, Seitenverhältnis und Tonpräferenz rückwärtskompatibel speichern. Zeitbereich nicht global auf ein anderes Lied übertragen.

**Abnahme:** Abbruch während jeder Phase, App-Schließen, Encoder-Absturz und voller Datenträger sind geprüft; kein verwaister Prozess, keine beschädigte alte Zieldatei. Dialog bleibt bedienbar, wiederholte Exporte funktionieren.

**Abhängigkeit:** M6. Aufwand: mittel. Die minimalen Schutzmaßnahmen aus M3 gelten bereits vorher.

### M8 – Performance, Distribution und Release-Abnahme

**Ergebnis:** Eine auslieferbare Version mit dokumentierten Voraussetzungen und überprüftem Ressourcenverhalten.

**Dateien:** `CMakeLists.txt`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`, `README.md`; neue Export-Integrationstests und plattformspezifische Paketierung gemäß vorhandenem Build-Prozess.

- [ ] Lange Lieder und 4K/60 mit repräsentativen dichten MIDI-Dateien messen: Frame-Erzeugung, GPU-Auslese, Encoder, Spitzenspeicher und temporären Speicher getrennt betrachten.
- [ ] Queue begrenzt halten: ein RGBA-Frame mit 3840 × 2160 benötigt rund 31,6 MiB. Nur bei gemessenem Engpass PBO-Auslese oder weitere Optimierungen ergänzen; keinen eigenen GL-Worker-Kontext ohne Bedarf einführen.
- [ ] FFmpeg-Verteilung konkretisieren: Entwicklungsbetrieb über konfigurierten Pfad/PATH, veröffentlichte Pakete mit festgelegtem unterstütztem Build und reproduzierbarer Erkennung. Benötigte Codec-Fähigkeiten vor Export prüfen; Versions- und Lizenzhinweise dokumentieren.
- [ ] Windows und Linux sowie Audio-ON/OFF prüfen; GPU-Integrationstests in geeigneter Umgebung betreiben. Keine plattformübergreifend bitidentischen GPU-Bilder als Testbedingung voraussetzen.
- [ ] Bedienung, temporären Speicherbedarf, erwartete Renderdauer und unterstützte Formate dokumentieren.

**Abnahme:** Export-Matrix bestanden: Ganzes Lied/Ausschnitt, 30/60 FPS, alle Qualitäts- und Formatpresets, mit/ohne Ton. Fertige MP4s vollständig dekodierbar; Ressourcenverbrauch bleibt begrenzt. Lange A/V-Synchronität und Abbruch bei hoher Last geprüft.

**Abhängigkeit:** M7. Aufwand: mittel bis groß, abhängig von Messergebnissen und Paketierung.

## 5. Reihenfolge und bewusste Grenzen

**M1 → M2 → M3 → M4 → M5 → M6 → M7 → M8**

- Nach **M3**: technischer MVP – komplettes stummes Video mit festem Profil.
- Nach **M5**: nutzbarer stummer Export mit allen visuellen Optionen und Ausschnitten.
- Nach **M6**: sämtliche angefragten Funktionen vorhanden.
- Nach **M8**: Release-Kandidat mit Fehler-, Ressourcen- und Distributionsprüfung.

Jeden Meilenstein als eigene Änderung bzw. PR abschließen; M3 und M6 jeweils in die genannten Teiländerungen zerlegen. Vor Umsetzung eines Meilensteins dessen konkrete Schnittstellen und Tests anhand des dann aktuellen Codes ausarbeiten. Dieser Plan enthält absichtlich noch keinen vorweggenommenen Implementierungscode für alle acht Meilensteine.

Vorerst außerhalb des Umfangs: Hardware-Encoding, Render-Warteschlange, Hintergrundwiedergabe parallel zum Export, mehrere Exportformate, transparente Videos, externe Audio-Dateien, neue Instrumentenunterstützung, frei wählbare Codecs und beliebige Pixelmaße. Keines davon ist Voraussetzung für die angefragten Funktionen.

Die größten Unsicherheiten sind die Hochkant-Komposition, Audiozustand an Schnittgrenzen und zuverlässige Prozess-/GPU-Koordination. Deshalb zuerst den stummen End-to-End-Pfad beweisen und Audio danach gezielt ergänzen.
