# EpochGui Showcase

An interactive C++23 example of [EpochGui](https://github.com/Autodidac/EpochGui), with optional Platform, Audio and Media examples.

## Try the controls

The main **Widgets** workspace keeps the examples together in one scrollable view. Click a heading to fold a section, or use the links on the left to jump to it.

- **Controls:** drag a slider, change a selection, toggle tips and resize a split pane. The progress indicators use the slider value.
- **Text:** edit a project name and a multiline note. Long text wraps and scrolls inside the field. Drag to select text, use Shift + arrows, and try Undo and Redo. The read-only example allows selection and copying.
- **Images:** load the included PPM image and compare aspect fit with stretch.
- **Input:** inspect pointer buttons, modifiers and window-chrome regions.

**Windows / Docks** contains draggable, resizable windows and working close, reopen, dock, float and detach controls. Right-click opens the relevant menu. The menu and window contents stay inside their bounds.

**Libraries** shows the libraries compiled into this build. Its step button advances the Media playhead and generates Audio PCM when those libraries are included. Media shows metadata and layout; it does not decode video. Audio mixes samples in memory; it does not open a sound device.

On Windows, Ctrl + A / C / X / V / Z / Y works with the system clipboard in both native hosts. Copying must succeed before a cut removes text. The edit buttons and text context menu use the same path. Other hosts retain the demo's local copy/paste buffer.

Ctrl + Tab and Ctrl + Shift + Tab change workspace. Tab moves focus between text fields; inside multiline notes it inserts a tab. The application uses the public EpochGui text controller, wrapping, selection, hit testing and scrollbar geometry.

## Run a download

Extract the complete Windows ZIP before starting `EpochGuiDemo.exe`. Keep its `assets` folder beside the executable. The Microsoft Visual C++2022 x64 runtime is required.

The source and runtime archives have separate SHA-256 files. Rebuilding and native acceptance apply to the exact new archive; earlier releases' screenshots or process exits are not proof of a new build.

## Build from source

Use Visual Studio2022 with Desktop development with C++ and a recent CMake. C++23 modules require a Visual Studio or Ninja generator. Full composition uses the current Platform library and CMake4.4 or newer.

A complete project bundle includes the libraries. For a plain Git clone, explicitly download the pinned public sources first:

```sh
python tools/prepare_dependencies.py --composition full
```

This command keeps existing folders and stops if their identity or source differs. It never replaces a library silently. `--check` verifies without downloading. CMake itself does not fetch dependencies.

To build just the GUI example on Windows:

```powershell
.\build_msvc.ps1 -Configuration Release -Composition gui -HostKind native
```

To build the combined example using the Platform-owned Win32 host:

```powershell
.\build_msvc.ps1 -Configuration Release -Composition full -HostKind platform
```

The default build uses two compiler jobs. Set `-Jobs` to change it.

The same settings can be passed directly to CMake:

```sh
cmake -S . -B build/full -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON -DEPOCHGUI_DEMO_COMPOSITION=full -DEPOCHGUI_DEMO_HOST=platform
cmake --build build/full --config Release --parallel 2
ctest --test-dir build/full -C Release --output-on-failure
```

| Composition | Libraries used |
| --- | --- |
| `gui` (default) | Gui |
| `media` | Gui + Media |
| `audio` | Gui + Audio |
| `platform` | Gui + Platform |
| `full` | Gui + Platform + Audio + Media |

`EPOCHGUI_DEMO_HOST=native` uses the existing Win32, GLX or Cocoa host. `platform` uses Platform's window, lifecycle, events and application entrypoint on Windows. Linux/macOS Platform-owned GL hosts remain pending; their native demo hosts are separate.

CMake checks bundled `dependencies` first, then sibling repositories. Explicit `EPOCHGUI_ROOT`, `EPOCH_PLATFORM_ROOT`, `EPOCH_AUDIO_ROOT` and `EPOCH_MEDIA_ROOT` override discovery. The exact public revisions are listed in `dependencies.json`.

## What the tests prove

CPU tests cover the selected library models, wrapped text and caret hit testing, bounded scrolling, Windows clipboard conversions, and DPI sizing. The showcase integration test drives the real renderer/controller through a recording OpenGL transport: it checks long pastes, transformed pointer hits, clip bounds, workspace keys and failed-cut preservation without opening a window.

These tests do not replace running the extracted package. Native checks must cover Windows input, clipping, normal and compressed sizes, DPI scaling, menus, docking and shutdown before a new release is accepted.

`python .github/audit_source.py` writes a fresh source inventory to `build/audits/source-audit.json`. A changed checkout is marked as changed; its old base commit is not presented as the current source revision. This report checks source files and license notices, not build or runtime behavior.

## Ownership and license

EpochGui owns reusable widgets, text, layout and geometry. The demo owns its presentation, example state and OpenGL drawing. Platform can own the native host; Audio and Media remain separate optional libraries.

The demo is MIT. Each included library keeps its own license. See the source folders and notices in the complete project bundle.
