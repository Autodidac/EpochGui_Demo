# EpochGui Demo

A cross-platform C++23 interactive catalog for [`Autodidac/EpochGui`](https://github.com/Autodidac/EpochGui).

The demo exercises selected subsystems exported by the core `epoch.gui` module, together with the optional rounded-rectangle and fallback-input modules. The controls are live: windows drag and resize, text fields accept input, menus react to right-click, panes dock and pop out, and the native host frame can be toggled while the custom EpochGui title bar stays visible.

## Selectable library examples

`EPOCHGUI_DEMO_COMPOSITION` selects the compiled graph and Libraries page:

| Value | Public libraries | Example |
| --- | --- | --- |
| `gui` (default) | Gui | Controls and progress geometry |
| `media` | Gui, Media | Admitted metadata, playhead and aspect-fit geometry |
| `audio` | Gui, Audio | Deterministic PCM mixing and measured samples |
| `platform` | Gui, Platform | Canonical native runtime policy |
| `full` | All four | Combined metadata, mixer and GUI presentation |

The step control advances half a second. Media does not decode video; Audio
mixes into memory without opening a sound device. Each graph has the compiled
`epoch_gui_demo_libraries_contract` CTest consumer.

`EPOCHGUI_DEMO_HOST=native` keeps the existing Win32, GLX or Cocoa host.
On Windows, `EPOCHGUI_DEMO_HOST=platform` additionally selects Platform ownership
of the window, lifecycle, event pump and application entrypoint. Demo owns WGL
and rendering on the Platform caller thread. This host keeps the native frame
and disables frame toggling. Linux/macOS Platform GL adapters remain pending.

Configure separate directories for each example:

```sh
cmake -S . -B build/gui -DEPOCHGUI_DEMO_COMPOSITION=gui
cmake -S . -B build/media -DEPOCHGUI_DEMO_COMPOSITION=media
cmake -S . -B build/audio -DEPOCHGUI_DEMO_COMPOSITION=audio
cmake -S . -B build/platform -DEPOCHGUI_DEMO_COMPOSITION=platform -DEPOCHGUI_DEMO_HOST=platform
cmake -S . -B build/full -DEPOCHGUI_DEMO_COMPOSITION=full -DEPOCHGUI_DEMO_HOST=platform
cmake --build build/full --config Release
ctest --test-dir build/full -C Release --output-on-failure
```

Choose a module-capable Visual Studio or Ninja generator. `EPOCHGUI_ROOT` and
the selected `EPOCH_PLATFORM_ROOT`, `EPOCH_AUDIO_ROOT`, `EPOCH_MEDIA_ROOT` accept
explicit local checkouts; sibling public repositories are defaults. No network
fetch or Engine fallback is used. Enable `BUILD_TESTING` for CPU contracts and
`EPOCHGUI_DEMO_LIBRARY_PAGE` to start on the composition page. Build success
alone does not establish interactive runtime acceptance.

## Live pages

### Overview

- `Vec2`, `Rect`, and `contains`
- Segmented controls and hit testing
- Selectable lists, hover, and selection
- Progress bars in four directions
- Loading-screen layout
- Draggable splitter with persistent capture until mouse release

### Windows

- Draggable and resizable `FloatingWindowState` windows
- Working close buttons and context-menu reopen actions
- Focus ordering for overlapping windows
- Rectangular and optional rounded rendering paths
- `DockLayoutState` with left, center, and right panes
- Pane popout and redock behavior
- `DockableWindowState` dock, float, and detach actions
- `PanelHostState` docked and floating modes
- Cursor-anchored `PopupState` context menus

### Text

- Editable single-line edit box
- Editable multiline text control
- Read-only text control used as static text
- Native text-event insertion
- Caret placement and movement
- Backspace, Delete, arrows, Home, End, Enter, and Tab behavior
- EpochGui selection, clipboard, scrolling, read-only, and byte-limit APIs

### Image

- Runtime loading of `assets/epochgui_demo.ppm` through `epoch.gui.image`
- Bounded P3 and P6 decoding into renderer-neutral RGBA8 pixels
- Image display inside an EpochGui `Rect`
- Aspect-fit and stretch layout from EpochGui
- OpenGL pixel batching kept in the demo layer

EpochGui owns the reusable decoder and raster layout without taking ownership of OpenGL presentation.

### Input

- Live pointer buttons, movement, and wheel state
- Keyboard and modifier state
- Right-click context requests
- `InputTracker`
- Custom-chrome layout and hit testing
- Caption, client, resize edge/corner, minimize, maximize, and close regions

## Custom title bar and native frame

The custom EpochGui title bar is always visible.

Native host decorations are enabled by default. The toolbar and right-click menu switch only the outer host presentation:

- **Native frame on:** system decorations plus the custom EpochGui application bar
- **Borderless mode:** custom EpochGui title bar with native move, resize, minimize, maximize, and close mapping

### Windows

The HWND is created once with `WS_OVERLAPPEDWINDOW` and keeps that style for its entire lifetime. Runtime toggling changes custom non-client handling rather than replacing the window style, so the HWND, HDC, pixel format, and OpenGL context remain stable.

When native mode is restored, the host resets the window to the current default Windows theme and restores DWM non-client rendering policy. It does not force the old Explorer/Windows 8 caption style.

Pointer, resize, keyboard, and wheel events queue a single render request rather than calling `SwapBuffers` synchronously from every event callback. This coalescing keeps splitter and floating-window drags responsive instead of flooding or recursively entering the renderer.

The executable includes a dedicated EpochGui Demo icon.

### Linux

The X11 host starts with ordinary window-manager decorations and uses `_MOTIF_WM_HINTS` for the live decoration toggle. It supplies an `_NET_WM_ICON`, forwards printable text input, drains pending X11 events, and renders once per event batch to avoid GLX lockups during rapid dragging.

## Optional EpochGui features

Both optional modules remain disabled by default in the standalone EpochGui library:

```text
EPOCHGUI_ENABLE_ROUNDED_RECT=OFF
EPOCHGUI_ENABLE_INPUT=OFF
```

This demo explicitly enables them:

```text
-DEPOCHGUI_ENABLE_ROUNDED_RECT=ON
-DEPOCHGUI_ENABLE_INPUT=ON
```

Applications that already have engine, SDL, GLFW, or native input should continue using their existing input path. `epoch.gui.input` is a portable fallback and adapter, not a mandatory replacement.

## EpochGui API coverage

The demo exercises public APIs for:

- Floating windows
- Splitters
- Progress bars
- Loading-screen layouts
- Selectable lists
- Segmented controls
- Popups
- Dock layouts
- Dockable windows
- Text controls
- Panel hosting
- Rounded-rectangle meshes
- Fallback input and custom window chrome

## Architecture

EpochGui owns reusable state, layout, geometry, hit testing, text editing, docking metadata, panel-host transitions, the embedded bitmap font, bounded PPM decoding, raster layout, optional rounded meshes, and optional normalized fallback input.

This repository owns:

- Win32/WGL, X11/GLX, and Cocoa event translation
- OpenGL 3.2 core presentation
- Demo-side text-event forwarding
- OpenGL image presentation using EpochGui raster data
- Native frame presentation and host commands
- Windows and Linux application icons
- Build scripts and release packaging

No EpochGui implementation is duplicated here.

The visible splitter originally used different geometry for drawing and hit testing. CMake now generates the renderer implementation from `src/complete_demo_renderer.cpp` with the demo-only splitter adapter injected immediately after the global module fragment. This preserves valid C++23 module ordering while making the interaction use the same rectangle that is rendered.

## EpochGui checkout

Keep the repositories beside one another:

```text
Projects/
|-- EpochGui/
`-- EpochGui_Demo/
```

The demo detects `../EpochGui` automatically. For another layout:

```text
-DEPOCHGUI_ROOT=/path/to/EpochGui
```

## Build

Requirements:

- CMake 3.28 or newer
- A C++23 compiler with module support
- Visual Studio 2022/2026 or Ninja
- Native OpenGL and window-system development packages

### Windows

```text
open_msvc.bat
build_msvc.bat Release
```

### Linux

```bash
chmod +x build_linux.sh
./build_linux.sh linux-gcc-release
```

or:

```bash
./build_linux.sh linux-clang-release
```

Clang requires a matching `clang-scan-deps` installation.

### macOS

```bash
cmake -S . -B build/macos -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DEPOCHGUI_ROOT=/path/to/EpochGui \
  -DBUILD_TESTING=ON
cmake --build build/macos
ctest --test-dir build/macos --output-on-failure
open build/macos/EpochGuiDemo.app
```

## Repository layout

```text
CMakeLists.txt                            Demo targets and generated adapter
CMakePresets.json                         Windows and Linux presets
assets/epochgui_demo.ppm                  Runtime-loaded image asset
modules/epoch.gui.demo.opengl.ixx         Renderer module interface and splitter state
src/complete_demo_renderer.cpp            Interactive EpochGui catalog source
src/platform/complete_windows_main.cpp    Win32/WGL host
src/platform/complete_linux_glx_main.cpp  X11/GLX host
src/platform/epochgui_demo.ico            Windows application icon
.github/workflows/windows-release.yml     Windows package and release creation
.github/workflows/linux-release.yml       Linux package attachment
```

The generated renderer source is written under the selected CMake build directory and is not committed.

## Releases

A release publishes matching platform archives and SHA-256 checksums.

### Windows x64

```text
EpochGui_Demo-v<version>-windows-x64.zip
```

Contents:

```text
EpochGuiDemo.exe
README.md
BUILD.txt
assets/epochgui_demo.ppm
```

### Linux x64

```text
EpochGui_Demo-v<version>-linux-x64.tar.gz
```

Contents:

```text
EpochGui_Demo-v<version>-linux-x64/
|-- EpochGuiDemo
|-- README.md
|-- BUILD.txt
`-- assets/epochgui_demo.ppm
```

The Windows workflow creates the release first. After it succeeds, the Linux workflow builds the same commit and attaches the Linux archive and checksum to that release.

For Media source distributions, `EPOCHGUI_DEMO_LIBRARY_PAGE=ON` opens the
Libraries page and `EPOCHGUI_DEMO_APP_NAME=EpochMediaDemo` names the shared host.
No renderer or window/event implementation is copied into Media. Standalone
Audio remains independent; the selected demonstration graph controls its dependencies.

Current source version is 0.6.0; historical release 0.5.3 remains unchanged.
Native runtime acceptance remains pending. Release workflows still
remain manually gated pending hosted checks and native acceptance.

## License

First-party source is MIT, including the owner-authorized migrated GUI code.
Third-party notices remain applicable. Historical releases retain their licenses.

Reviewed EpochGui source dependency: `f83b2f15385598b8efbb028a6f391ca637e07634` (0.90.0).
Platform: `c1b20ea9bf80fb213b5499a8857a9a0066db5ccd` (0.2.0); Audio: `89d151605830883e0d00723fa478b348e742e638` (0.1.0); Media: `4dbb222aab13624cf53d699453e00e4db26cb8d9` (0.1.0). These are exact source pins; Platform hosted Windows/Linux Debug/Release checks passed; Demo hosted checks and native acceptance remain pending, so this is not a release-ready graph.
