## iup-go
[![Build Status](https://github.com/gen2brain/iup-go/actions/workflows/build.yml/badge.svg)](https://github.com/gen2brain/iup-go/actions)
[![Go Reference](https://pkg.go.dev/badge/github.com/gen2brain/iup-go.svg)](https://pkg.go.dev/github.com/gen2brain/iup-go/iup)

Go bindings for [IUP](https://www.tecgraf.puc-rio.br/iup/), a multi-platform toolkit for building graphical user interfaces.
The toolkit provides system native UI controls for Windows (Win32), macOS (Cocoa), and Linux (GTK and Qt).

IUP C source code is included and compiled together with bindings.
Note that the first build can take a few minutes.

### Requirements

Go 1.21 is the minimum required version.

#### Windows

On Windows, you need a C compiler, like [Mingw-w64](https://mingw-w64.org) or [TDM-GCC](http://tdm-gcc.tdragon.net/).
You can also build a binary in [MSYS2](https://msys2.github.io/) shell.

* To remove a console window, i.e., compile a GUI app with WinMain entry-point, build with `-ldflags "-H=windowsgui"`.
* You can add an icon resource to an .exe file with the [rsrc](https://github.com/akavel/rsrc) tool.
* Windows manifest is included in the build by default. See below how to disable the manifest if you want to include your own.

[<img src="examples/sample/sample_win32.png" width="700"/>](examples/sample/sample_win32.png)

#### macOS

On macOS, you need Command Line Tools for Xcode (if you have `brew`, you already have this).

* To create an `.app` bundle or `.dmg` image, check this gist https://gist.github.com/mholt/11008646c95d787c30806d3f24b2c844.
* You can build for Qt in macOS, with the `qt` build tag. Install deps with `brew install qt`.
* You can build for GTK in macOS, with the `gtk` build tag. Install deps with `brew install gtk+3`.

[<img src="examples/sample/sample_cocoa.png" width="700"/>](examples/sample/sample_cocoa.png)

#### Linux

On Linux, you need a C compiler and development packages for GTK or Qt.

##### GTK

###### GTK3 (default)

* Debian/Ubuntu: `apt-get install libgtk-3-dev`
* RedHat/Fedora: `dnf install gtk3-devel`

###### GTK4

* Debian/Ubuntu: `apt-get install libgtk-4-dev`
* RedHat/Fedora: `dnf install gtk4-devel`

Note that you can also build for GTK2.

You do not need to install `WebKitGTK` development packages for `WebBrowser` control; libraries are loaded at runtime.

[<img src="examples/sample/sample_gtk3.png" width="700"/>](examples/sample/sample_gtk3.png)

##### Qt

###### Qt6 (default)

* Debian/Ubuntu: `apt-get install qt6-base-dev`
* RedHat/Fedora: `dnf install qt6-qtbase-devel`

###### Qt5

* Debian/Ubuntu: `apt-get install qt5base-dev`
* RedHat/Fedora: `dnf install qt5-qtbase-devel`

For the `WebBrowser` control, install `qt6-webengine-dev` or `qt6-qtwebengine-devel`.

For the `GlCanvas` control, Qt6 needs private headers for Wayland; install `qt6-base-private-dev` or `qt6-qtbase-private-devel`.
You must manually export the path to private header files.
For example, check `pkg-config --modversion Qt6Core` and `pkg-config --cflags Qt6Core` and then based on that use:
```
CGO_CXXFLAGS="-I/usr/include/qt6/QtCore/6.9.3 -I/usr/include/qt6/QtGui/6.9.3" go build -tags qt,gl
```

[<img src="examples/sample/sample_qt6.png" width="700"/>](examples/sample/sample_qt6.png)

#### Other

The library should work on other Unix-like systems, FreeBSD, NetBSD, OpenBSD, DragonFly, Solaris, Illumos, and AIX.

You can also compile for a time-tested [Motif](https://en.wikipedia.org/wiki/Motif_(software)) library if GTK or Qt are not available.

* Debian/Ubuntu: `apt-get install libmotif-dev libxmu-dev libxpm-dev`
* RedHat/Fedora: `dnf install motif-devel libXpm-devel`

[<img src="examples/sample/sample_motif.png" width="700"/>](examples/sample/sample_motif.png)

### Build tags

* `ctl` - build with support for `Matrix` and `Cells` controls
* `gl` - build with support for `GLCanvas` control
* `web` - build with support for `WebBrowser` control
* `gtk` - use GTK in macOS or Windows
* `gtk4` - build for GTK4, default is GTK3
* `gtk2` - build for GTK2
* `qt` - build for the Qt framework
* `qt5` - build for Qt5 version, default is Qt6 (used with `qt`)
* `motif` - build for X11/Motif 2.x environment
* `xembed` - use XEmbed tray protocol instead of SNI
* `nomanifest` - do not include manifest in Windows build
* `nopkgconfig` - do not use pkg-config for compile and link flags

### Compiler flags

You can provide explicit compiler and linker flags instead of using the defaults provided by pkg-config.
For example, if dependencies are in a non-standard location:

```
CGO_CFLAGS="-I<include path> ..." CGO_LDFLAGS="-L<dir> -llib ..." go build -tags nopkconfig
```

You can also point `PKG_CONFIG_LIBDIR` to some local directory with custom modified `.pc` files.

### Documentation

[IUP](https://www.tecgraf.puc-rio.br/iup/) documentation is a must; every Go function in the doc reference there.
Also check [Go Reference](https://pkg.go.dev/github.com/gen2brain/iup-go/iup) and [Examples](https://github.com/gen2brain/iup-go/tree/main/examples).

### Thread-Safety

User interfaces (and OpenGL) are usually not thread-safe, and IUP is not either. Some platforms enforce running UI on the main thread.
Note that a goroutine can arbitrarily and randomly be scheduled or rescheduled on different running threads.

The secondary threads (goroutine) should not directly update the UI; instead, use `PostMessage`, which is expected to be thread-safe.
See [example](https://github.com/gen2brain/iup-go/tree/main/examples/postmessage/postmessage.go) that sends data to an element,
which will be received by a callback when the main loop regains control. You can also use the `IdleFunc` and `Timer`.

### Cross-compile (Linux)

To cross-compile for Windows, install [MinGW](https://www.mingw-w64.org/) toolchain.

```
$ CGO_ENABLED=1 CC=x86_64-w64-mingw32-gcc GOOS=windows GOARCH=amd64 go build -ldflags "-s -w"
$ file alarm.exe
alarm.exe: PE32+ executable (console) x86-64, for MS Windows

$ CGO_ENABLED=1 CC=i686-w64-mingw32-gcc GOOS=windows GOARCH=386 go build -ldflags "-s -w"
$ file alarm.exe
alarm.exe: PE32 executable (console) Intel 80386, for MS Windows
```

To cross-compile for macOS, install [OSXCross](https://github.com/tpoechtrager/osxcross) toolchain.
Ready-made SDK tarballs are available [here](https://github.com/joseluisq/macosx-sdks).

```
$ CGO_ENABLED=1 CC=x86_64-apple-darwin25-clang GOOS=darwin GOARCH=amd64 go build -ldflags "-s -w"
$ file alarm
alarm: Mach-O 64-bit x86_64 executable, flags:<NOUNDEFS|DYLDLINK|TWOLEVEL|WEAK_DEFINES|BINDS_TO_WEAK>

$ CGO_ENABLED=1 CC=aarch64-apple-darwin25-clang GOOS=darwin GOARCH=arm64 go build -ldflags "-s -w"
$ file alarm
alarm: Mach-O 64-bit arm64 executable, flags:<NOUNDEFS|DYLDLINK|TWOLEVEL|WEAK_DEFINES|BINDS_TO_WEAK|PIE>
```

### Screenshots

See more [screenshots](https://github.com/gen2brain/iup-go/tree/main/examples/sample/README.md).

### Credits

* [Tecgraf/PUC-Rio](https://www.tecgraf.puc-rio.br)
* [matwachich](https://github.com/matwachich/iup)

### License

iup-go is MIT licensed, same as IUP. View [LICENSE](https://github.com/gen2brain/iup-go/blob/main/LICENSE).
