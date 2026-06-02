# KeyWave

KeyWave is a small work-in-progress desktop MIDI visualizer. It loads a MIDI file,
turns note events into a piano-roll scene, and renders falling notes over a
keyboard with OpenGL.

## Core Features

- Load a midi file from the command line.
- Render a falling-notes piano-roll view.
- Playback controls.
- Ajust visualization.

## Requirements

- Git
- CMake 3.24 or newer
- A C++20 compiler
- OpenGL 3.3 support

On Linux, install the development packages needed. The CI build
uses:

```sh
apt-get install -y libgl1-mesa-dev libwayland-dev libxkbcommon-dev xorg-dev
```

## Build

```sh
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The app executable is named `KeyWave`.

## Run

Start with an empty window:

```sh
./build/KeyWave
```

Load a MIDI file at startup:

```sh
./build/KeyWave path/to/song.mid
```

Or windows:

```powershell
.\build\KeyWave.exe C:\Music\song.mid
```

If the MIDI path is missing or invalid, KeyWave logs a warning and opens an
empty window.

## Controls

| Input | Action |
| --- | --- |
| Space | Play or pause |
| R | Restart |
| S | Stop |
| Left / Right | Seek backward or forward |
| Up / Down | Increase or decrease playback speed |
| Escape | Show or hide the visualization settings panel |

## Tests

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

The main test executable is `KeyWaveMidiTests`. Tests are discovered by Catch2.

## Dependencies

Vendored in `external/`:

- GLAD
- Dear ImGui
- craigsapp/midifile
- nlohmann/json

Fetched by CMake:

- GLFW 3.4
- Catch2 v3.11.0 for tests

## License

KeyWave is licensed under the GNU General Public License v3.0. See
[LICENSE](LICENSE).
