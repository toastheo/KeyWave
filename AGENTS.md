# Working on KeyWave

KeyWave is a C++20 desktop MIDI visualizer: MIDI timeline -> CPU scene/layout ->
backend-independent render commands -> OpenGL 3.3, with Dear ImGui controls and
optional FluidSynth audio. CMake defines the module boundaries. Follow the existing
design rather than introducing a second rendering, playback, or settings system.

## Scope and sources of truth

- Read the files relevant to the task and their tests; a full repository scan is
  unnecessary for a small change. Use `src/CMakeLists.txt` for the dependency graph,
  `.clang-format` / `.clang-tidy` for tooling rules, and
  `tests/architecture/` for enforced boundaries.
- Preserve unrelated working-tree changes. Keep refactoring and formatting scoped
  to the requested work. Do not edit vendored `external/` code, generated build
  files, or dependency pins as incidental cleanup.
- Read the relevant plan in `docs/superpowers/plans/` when implementing a planned
  feature or milestone. Implement the requested scope; a roadmap is not permission
  to implement every later milestone. Check the code before assuming a checkbox
  accurately describes the current implementation.
- For the ongoing video-export feature, defer README feature documentation until
  the feature is complete and being prepared for merge into `main`, unless the
  maintainer explicitly requests it. Keep implementation notes in the feature plan.
- Source identifiers, comments, diagnostics, UI text, and project documentation
  use English. Respond to the maintainer in the language of their request.

## Architecture and placement

| Location | Responsibility and boundary |
| --- | --- |
| `src/core/` | Shared value types such as `Color`, `Rect`, and `FramebufferSize`. |
| `src/midi/` | MIDI loading, timeline data, tempo/sustain events, and queries; third-party parsing stays behind `MidiFileLoader`. |
| `src/render/` | Render commands, scenes, views, and the abstract backend; no OpenGL or ImGui implementation. |
| `src/keyboard/`, `src/fallingnotes/` | CPU geometry/layout and conversion to render commands. Accept domain config/style types, not `AppSettings`. |
| `src/playback/` | Transport state, explicit elapsed-time updates, and playback rate. |
| `src/audio/` | `PianoSynth`, scheduling, `NullPianoSynth`, and the optional FluidSynth implementation. |
| `src/export/` | CPU export settings, immutable snapshots, indexed timing, and the encoder interface; no concrete GPU/process backend. |
| `src/export_ffmpeg/` | FFmpeg adapter and asynchronous frame streaming through `ChildProcess`. |
| `src/platform/` | Native windows, dialogs, and child processes; keep platform APIs here and behind platform guards. |
| `src/app/` | Settings, persistence, controllers, settings-to-domain adapters, and application wiring. `Application` wires the concrete backends. |
| `src/ui/` | ImGui controls and testable UI helpers. Return actions to the application instead of duplicating controller logic. |

- Keep pure layout/timing logic usable without a window, GPU, audio device, or
  encoder. Reuse `PianoRollSceneBuilder::build(timeline, time, config)` for scenes
  at explicit times. Keep `AppSettings` conversion in
  `src/app/VisualizationSettingsAdapters.*`.
- Keep FluidSynth out of MIDI, render, OpenGL, and UI modules, including their
  public dependencies. Inject `PianoSynth` and preserve the silent fallback.
- Route application diagnostics through `DiagnosticSink` and `reportInfo`,
  `reportWarning`, or `reportError`. Direct console output in `src/` belongs only
  in `src/diagnostics/Diagnostics.cpp`.
- Register new source files and tests in the appropriate `src/CMakeLists.txt` or
  `tests/CMakeLists.txt` target. Prefer target-scoped configuration and `PRIVATE`
  dependencies; expose a dependency publicly only when the public API needs it.

## C++ conventions

- Use C++20 without compiler extensions. Match adjacent code: PascalCase types and
  files, camelCase functions/variables, `m_` private data members, `#pragma once`
  headers, and anonymous namespaces for implementation/test helpers. Do not add a
  repository-wide namespace or `using namespace std` as a cleanup.
- `.clang-format` is authoritative: two spaces, no tabs, 100-column C++ lines,
  opening braces on their own line for functions/classes and on the same line for
  control flow. Use LF line endings as required by `.gitattributes`.
- Include the matching header first in a `.cpp`, then sorted `<...>` headers,
  then sorted project headers such as `"midi/MidiTypes.hpp"`. Include what each
  file uses directly, including standard headers; do not rely on transitive includes.
- Prefer small value/config structs with default member initializers. Use
  designated initializers for aggregates, scoped enums with suitable explicit
  underlying types, named `constexpr` constants, and `[[nodiscard]]` for query results.
  Use an options struct when several new arguments would be easily confused.
- Use `const` where semantically correct, not mechanically. Buffers may be mutated
  indirectly through C APIs or `rdbuf()`. Immutable snapshots may copy on an rvalue;
  do not promise `noexcept` moves that actually copy allocating members.
- Use references, `std::string_view`, and `std::span<const T>` for borrowed inputs
  where appropriate; use `std::vector` for owned variable-length data and
  `std::array` for fixed buffers. Keep raw pointers/arrays at APIs that require them,
  such as native C callbacks, and adapt at the boundary.
- Own resources with RAII and `std::unique_ptr` where suitable. Resource wrappers
  must not accidentally copy handles. Destructors and `noexcept` cleanup must not
  let failures escape; filesystem cleanup can use `std::error_code` overloads.
- Validate finite values, bounds, sizes, and conversions at boundaries. Preserve
  each module's policy: settings sanitization supplies safe defaults, while strict
  export validation rejects invalid requests. Follow existing optional/bool plus
  diagnostics or exception contracts rather than changing error conventions globally.
  Handle failures at UI/worker boundaries and do not let exceptions cross C callbacks.

## Behavior that must stay consistent

- Distinguish MIDI source time, playback time (with visual lead-in), and export
  output time. Preserve the controller's timeline offset and the playback rate.
  Use `ExportTiming` and integer frame/sample indices for export timestamps/counts;
  do not derive exports from live frame deltas or repeatedly add floating-point steps.
- Settings changes may need coordinated updates to `AppSettings`,
  `AppSettingsConstraints`, sanitizers, serializer, adapters, and controls. Preserve
  defaults, older saved files, and safe handling of malformed input.
- Keep MIDI tempo changes, overlapping notes, sustain ordering, pause/resume, and
  seek behavior intact. A loaded timeline is not necessarily nonempty or exportable.
- GL creation, rendering, readback, and destruction require the owning current
  context on its thread. Release GL resources before destroying the window/context;
  offscreen operations must restore live state. Preserve the top-left, tightly
  packed RGBA8 pixel-buffer contract and do not capture ImGui into export frames.
- Export uses a frozen snapshot, bounded frame buffering, and worker-side process
  I/O. Do not block the UI on pipe writes or hold a status mutex across slow I/O.
  Preserve cancellation, process reaping, bounded diagnostics, and job-owned temp
  cleanup. Publish only successfully finalized output; preserve existing files on
  failure and respect the requested overwrite policy.
- Launch FFmpeg through executable plus arguments, never shell command concatenation.
  Preserve Unicode paths and Windows no-console behavior. Audio-disabled builds
  must remain usable without FluidSynth. Do not add blocking/allocating work to the
  real-time audio callback.

## Build and verification

Run commands from the repository root. Reuse a compatible configured build tree;
do not mix compilers in one cache. For a new Ninja debug tree:

```sh
cmake -S . -B build/agent-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/agent-debug --parallel
ctest --test-dir build/agent-debug --output-on-failure
```

- CMake 3.24+ is required. `KEYWAVE_ENABLE_AUDIO` defaults to `ON` and needs
  FluidSynth development files. To check the fallback, configure a separate tree
  with `-DKEYWAVE_ENABLE_AUDIO=OFF`. Check both modes for changes to shared app,
  audio wiring, or CMake dependencies. CI covers Windows/MSYS2 UCRT64, Linux, and
  macOS; preserve platform guards and portable paths.
- On Windows, use the configured toolchain consistently and put its DLL directory
  on `PATH` when running test executables directly. If sandbox temp storage is not
  writable, point `TEMP`/`TMP` to a dedicated ignored workspace test directory;
  diagnose the environment rather than weakening filesystem tests.
- `KeyWaveMidiTests` is the main Catch2 suite. Mirror source directories in `tests/`,
  name files `*Tests.cpp`, put tests/helpers in anonymous namespaces, use descriptive
  `TEST_CASE` names/tags, `REQUIRE` for prerequisites, and `Catch::Approx` for floats.
  Test observable behavior with independent expectations and disposable fixtures.
  Use explicit paths for storage tests, not the maintainer's real settings/library.
- Add regression coverage for behavior fixes. Prefer CPU tests and injected fake
  backends for orchestration; use real backends for integration checks. Do not add
  tests that merely restate formatting or private implementation details.
- Run affected tests while iterating, then the relevant CTest suite before claiming
  a code change is complete. Changes to layering, diagnostics, or borrowed APIs must
  keep `tests/architecture/LayeringTests.cpp` and `SpanApiTests.cpp` passing. For
  documentation-only edits, validate the content/links without rebuilding the app.
- GPU tests are opt-in: `-DKEYWAVE_ENABLE_GPU_TESTS=ON`, then
  `ctest --test-dir build/agent-debug -L gpu --output-on-failure`. They need a real
  OpenGL 3.3 context; Linux CI uses Xvfb/software Mesa (see `.github/workflows/ci.yml`).
- Real FFmpeg tests are opt-in: `-DKEYWAVE_ENABLE_ENCODER_TESTS=ON`, then
  `ctest --test-dir build/agent-debug -L encoder --output-on-failure`. They require
  FFmpeg with `libx264` on `PATH` (or beside the executable on Windows). Enable both
  options for the complete GPU-to-MP4 test. State when environment-dependent tests
  could not be run; do not describe a reduced suite as full verification.

## Formatting and static analysis

- Format changed C++ files with `clang-format -i <files>` using the repository
  config. `scripts/format.ps1` and `scripts/format.sh` can format a whole directory;
  avoid unrelated bulk formatting.
- Use the actual build's `compile_commands.json` for Clang-Tidy. Start with changed
  `.cpp` files and representative consumers of changed headers; a full scan is
  appropriate when requested or for broad changes. For example:

```powershell
./scripts/run-clang-tidy.ps1 -BuildDir build/agent-debug -File src/app/VisualizerController.cpp -Output build/clang-tidy.txt
```

```sh
./scripts/run-clang-tidy.sh --build-dir build/agent-debug --file src/app/VisualizerController.cpp --output build/clang-tidy.txt
```

- Select tools via `CLANG_TIDY` / `CLANG_FORMAT` or the scripts' tool arguments.
  To include tests, pass `-SourceDirs @("src", "tests")` in PowerShell or
  `--source-dirs src,tests` in Bash. Inspect diagnostics and compiler errors, not
  just the wrapper's exit status. Fix analysis include/toolchain setup before
  treating incomplete parse results as valid findings.
- Do not disable checks globally to obtain a clean report. Verify possible
  false positives individually (SDK umbrella headers, Catch2 assertions, deliberate
  invalid-value tests). If a suppression is necessary, name the specific check,
  keep its scope narrow, and explain the invariant. Keep generated reports and
  scratch scripts under ignored build directories.
- Finish with `git diff --check` and a short report of changes, checks actually run,
  and any remaining failures, unrun checks, or justified lint exceptions. Do not
  infer success from earlier runs or hard-code test totals in repository guidance.
