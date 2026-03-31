# IUP - Portable User Interface

IUP is a multi-platform toolkit for building graphical user interfaces.
It uses native interface elements for high performance and platform-consistent look and feel.

This is a fork maintained as part of [IUP-Go](https://github.com/gen2brain/iup-go) with additional backends (Cocoa, WinUI, GTK4, Qt, FLTK, EFL) and features.

API reference documentation is available in the [docs](https://github.com/gen2brain/iup-go/tree/main/docs) directory.
For the original IUP documentation, visit [IUP's website](https://www.tecgraf.puc-rio.br/iup).

## Building

Requires CMake 3.20 or later.

### Quick Start

```bash
# Default backend (GTK3 on Linux, Win32 on Windows, Cocoa on macOS)
cmake -B build
cmake --build build

# Shared library
cmake -B build -DBUILD_SHARED_LIBS=ON
cmake --build build

# Install
cmake --install build --prefix /usr/local
```

### Using Presets

```bash
cmake --preset gtk4
cmake --build build/gtk4
```

Available presets:

| Preset       | Backend                   | Notes                |
|--------------|---------------------------|----------------------|
| `default`    | Platform native           | GTK3 / Win32 / Cocoa |
| `gtk3`       | GTK3                      |                      |
| `gtk4`       | GTK4                      |                      |
| `gtk2`       | GTK2                      | Legacy               |
| `win32`      | Win32                     | Windows native       |
| `winui`      | WinUI / XAML Islands      | Requires Clang++     |
| `cocoa`      | Cocoa                     | macOS native         |
| `motif`      | Motif / X11               |                      |
| `qt6`        | Qt6                       |                      |
| `qt5`        | Qt5                       |                      |
| `fltk`       | FLTK                      |                      |
| `efl`        | EFL / Elementary          |                      |
| `gtk3-full`  | GTK3 + all optional libs  |                      |
| `gtk4-full`  | GTK4 + all optional libs  |                      |
| `win32-full` | Win32 + all optional libs |                      |
| `cocoa-full` | Cocoa + all optional libs |                      |
| `debug`      | Platform native, debug    |                      |

You can create a `CMakeUserPresets.json` file for local overrides (e.g., toolchain or compiler paths) without modifying the tracked `CMakePresets.json`.
IDEs automatically pick up both files.

### Options

| Option               | Default          | Description                                          |
|----------------------|------------------|------------------------------------------------------|
| `IUP_BACKEND`        | Platform default | GUI backend selection                                |
| `IUP_BUILD_GL`       | `OFF`            | Build `iupgl` (OpenGL canvas)                        |
| `IUP_BUILD_WEB`      | `OFF`            | Build `iupweb` (Web browser control)                 |
| `IUP_BUILD_CTRL`     | `OFF`            | Build `iupctrl` (Matrix, Cells, and Flat* controls)  |
| `IUP_BUILD_PLOT`     | `OFF`            | Build `iupplot` (Plot control)                       |
| `IUP_BUILD_EXAMPLES` | `OFF`            | Build example programs (C and C++)                   |
| `IUP_USE_XEMBED`     | `OFF`            | Use XEmbed tray protocol instead of SNI (GTK3/Motif) |
| `BUILD_SHARED_LIBS`  | `OFF`            | Build shared libraries instead of static             |

### Examples

```bash
# GTK4 with OpenGL support
cmake -B build -DIUP_BACKEND=gtk4 -DIUP_BUILD_GL=ON
cmake --build build

# Qt6 with OpenGL and WebEngine
cmake -B build -DIUP_BACKEND=qt6 -DIUP_BUILD_GL=ON -DIUP_BUILD_WEB=ON
cmake --build build

# FLTK
cmake -B build -DIUP_BACKEND=fltk
cmake --build build

# Everything as shared libraries
cmake -B build -DBUILD_SHARED_LIBS=ON -DIUP_BUILD_GL=ON -DIUP_BUILD_WEB=ON -DIUP_BUILD_CTRL=ON -DIUP_BUILD_PLOT=ON
cmake --build build
```

### Platform Dependencies

**Win32** (default on Windows):
No external dependencies.

**Cocoa** (default on macOS):
No external dependencies.

**GTK3** (default on Linux):
`gtk3-devel` or `libgtk-3-dev`

**GTK4**:
`gtk4-devel` or `libgtk-4-dev`

**Qt6**:
`qt6-base-dev` or `qt6-qtbase-devel`.
For GL: also `qt6-base-private-dev` or `qt6-qtbase-private-devel` (CMake finds private headers automatically).
For Web: also `qt6-webengine-dev` or `qt6-qtwebengine-devel`.

**Qt5**:
`qtbase5-dev` or `qt5-qtbase-devel`.
For Web: also `qtwebengine5-dev` or `qt5-qtwebengine-devel`.

**EFL**:
`efl-devel` or `libefl-dev`.

**FLTK**:
`fltk-devel` or `libfltk1.4-dev`. Requires FLTK 1.4.x. Works on Linux (X11/Wayland), Windows, and macOS.
FLTK does not provide pkg-config files, so `libfltk` and `libfltk_images` must be in the library search path.

**Motif**:
`motif-devel` or `libmotif-dev`. XFT and FreeType are auto-detected for font rendering.

**WinUI** (Windows only):
Requires Clang++ with C++20 and libc++ support. Use the `winui` preset or set `CMAKE_CXX_COMPILER=clang++`.
At runtime, `Microsoft.WindowsAppRuntime.Bootstrap.dll` and `resources.pri` must be next to the `.exe` file.
These files can be found in the `Microsoft.WindowsAppSDK` NuGet package.

**OpenGL** (`IUP_BUILD_GL`):
GTK3/GTK4/Qt/EFL/FLTK use EGL on Linux: `libegl-dev libgl-dev` or `libglvnd-devel`.
Motif/GTK2 use GLX: `libgl-dev` or `libglvnd-devel`.
Windows uses WGL, macOS uses OpenGL framework (no extra deps).

### Using IUP from CMake

After installing, use `find_package` in your project:

```cmake
find_package(IUP REQUIRED)
target_link_libraries(myapp PRIVATE IUP::iup)

# Optional libraries
target_link_libraries(myapp PRIVATE IUP::iupgl)
target_link_libraries(myapp PRIVATE IUP::iupweb)
target_link_libraries(myapp PRIVATE IUP::iupctrl)
target_link_libraries(myapp PRIVATE IUP::iupplot)
```

### Source Tarball

To create a source distribution tarball:

```bash
cmake -B build
cd build
cpack --config CPackSourceConfig.cmake
```

This produces `iup-<version>.tar.gz` and `iup-<version>.tar.xz` in the build directory.

## Libraries

| Library   | Description                       |
|-----------|-----------------------------------|
| `iup`     | Core library (always built)       |
| `iupgl`   | OpenGL canvas control             |
| `iupweb`  | Web browser control               |
| `iupplot` | Plot/charting control             |
| `iupctrl` | Matrix, Cells, and Flat* controls |
