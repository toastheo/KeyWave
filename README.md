# KeyWave

KeyWave is a small work-in-progress desktop MIDI visualizer. It loads a MIDI file,
turns note events into a piano-roll scene, and renders falling notes over a
keyboard with OpenGL.

<img width="1918" height="1008" alt="image" src="https://github.com/user-attachments/assets/a008dafa-709d-4c6a-a35d-aff2c075a11c" />

## Core Features

- Load a midi file from the command line.
- Render a falling-notes piano-roll view.
- Playback controls.
- Adjust visualization.

## Requirements

- Git
- CMake 3.24 or newer
- A C++20 compiler
- OpenGL 3.3 support

On Linux, install the development packages needed. The CI build
uses:

```sh
apt-get install -y libgl1-mesa-dev libwayland-dev libxkbcommon-dev libgtk-3-dev xorg-dev
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

## Controls

| Input | Action |
| --- | --- |
| Space | Play or pause |
| R | Restart |
| S | Stop |
| Left / Right | Seek backward or forward |
| Up / Down | Increase or decrease playback BPM |
| Escape | Show or hide the visualization settings panel |

## Tests

```sh
ctest --test-dir build --build-config Release --output-on-failure
```

The main test executable is `KeyWaveMidiTests`. Tests are discovered by Catch2.

## Dependencies

Vendored in `external/`:

- [GLAD 2.0.8](https://github.com/dav1dde/glad)
- [Dear ImGui v1.92.8](https://github.com/ocornut/imgui)
- [craigsapp/midifile](https://github.com/craigsapp/midifile)
- [nlohmann/json 3.12.0](https://github.com/nlohmann/json)

Fetched by CMake:

- [GLFW 3.4](https://github.com/glfw/glfw)
- [nativefiledialog-extended v1.3.0](https://github.com/btzy/nativefiledialog-extended/)
- [Catch2 v3.15.0](https://github.com/catchorg/Catch2) for tests

## License

KeyWave is licensed under the GNU General Public License v3.0. See
[LICENSE](LICENSE).
