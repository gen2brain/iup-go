# IUP for WebAssembly

This directory builds the IUP C driver to WebAssembly with Emscripten and runs IUP-Go apps in the browser.
Widgets render as real DOM elements, so an app looks and behaves like a normal web page.

Two independent build flows are supported:

* **Go flow (primary)** - a normal IUP-Go program compiled for `GOOS=js GOARCH=wasm`, served alongside the Emscripten-built IUP module.
* **C flow (optional)** - a `*.c`/`*.cpp` IUP app compiled together with IUP into a single module.

## Prerequisites

* [Emscripten](https://emscripten.org/) (emsdk) at `/opt/emsdk`, or `EMSDK` exported. Activate once: `cd /opt/emsdk && ./emsdk activate latest && . ./emsdk_env.sh`.
* Go 1.21+ (or TinyGo for a smaller binary, `-t`).
* For screenshots only: Node, `playwright-core` (global), and a system Chrome or Chromium (override with `IUP_BROWSER`).

## Go flow

Write the same `main()` as desktop. `iup.MainLoop()` returns when a callback returns `iup.CLOSE`, so code after it (including `defer iup.Close()`) runs at app close, exactly like desktop.

```go
// main.go
package main

import "github.com/gen2brain/iup-go/iup"

func main() {
    iup.Open()
    defer iup.Close()

    dlg := iup.Dialog(iup.Button("Hello, WebAssembly!"))
    dlg.SetAttribute("TITLE", "IUP-Go")
    iup.Show(dlg)

    iup.MainLoop()
}
```

Build and serve:

```sh
./build-wasm.sh ../../../examples/dialog_hello   # -> build/
cd serve && go run . -dir ../build -port 8000    # open http://localhost:8000/
```

The argument is an example directory, a `*.go` file, or a `*.c`/`*.cpp` app. Pass `-O` for a release build, `-T` for build tags (`gl`, `web`, `ctrl`, `plot`), and `-f` to force a rebuild after C changes.
To build and screenshot in headless Chrome:

```sh
./build-wasm.sh -f -s ../../../examples/sample
```

Run `./build-wasm.sh -h` for the full flag list.

## Serving

The app needs cross-origin isolation, so the server **must** send:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

`serve/main.go` sets these (and the `application/wasm` MIME). A plain static server without them will not start.

## C flow

A `*.c`/`*.cpp` app is built together with IUP and run directly:

```sh
./build-wasm.sh -s mycanvas.c
```

## Caveats

* Blocking, value-returning modals (`IupAlarm`, custom `IupPopup`) show but do not pause the page, since a browser cannot block synchronously. `IupFileDlg` opens only from a real user gesture.
* `IupGLCanvas` works over WebGL2 with `-T gl`. The standalone go-gl examples cannot target wasm; use `gl_web` instead.
