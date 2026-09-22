// Package iup is a Go binding for IUP, a multi-platform toolkit for building graphical user
// interfaces with native controls.
//
// The package bundles a fork of the IUP C library with its own drivers and additions. The API
// reference is in the repository [docs], and each function links to its page. Runnable programs
// are in [examples].
//
// # Drivers
//
// The default driver is Win32 on Windows, GTK3 on Linux and BSD, and Cocoa on macOS. Build tags
// select the others: winui, gtk4, gtk2, gtk (GTK3 on Windows or macOS), qt, qt with qt5, motif,
// fltk, efl and gnustep. Haiku (GOOS=haiku), iOS (GOOS=ios), WebAssembly (GOOS=js) and Android
// have their own drivers. The tags gl, web, ctrl, plot and media add the OpenGL canvas, the web
// browser, the extra controls, the plot and the media elements.
//
// With CGO_ENABLED=0 the package loads a prebuilt IUP shared library through purego, so no C
// compiler is needed.
//
// # Elements, attributes and callbacks
//
// Every dialog and control is an [Ihandle]. Controls are never placed at fixed coordinates, the
// layout is computed from containers such as [Vbox], [Hbox] and [Fill].
//
// Attributes are read and written as strings with [Ihandle.SetAttribute] and [Ihandle.GetAttribute].
// Names are upper case, values such as YES and NO are case-insensitive, and an unset attribute can
// be inherited from the parent container.
//
// [Ihandle.SetCallback] registers a function for an event. A callback returns [DEFAULT], [CLOSE],
// [IGNORE] or [CONTINUE].
//
// # Program structure
//
// Call [Open] first, [MainLoop] to run the event loop and [Close] at the end. Calling [EntryPoint]
// from init lets the same main run on Android, iOS and WebAssembly.
//
// IUP is not thread-safe. Goroutines must not call IUP directly, use [PostMessage], an idle
// callback or a [Timer] to run code on the UI thread.
//
// [docs]: https://github.com/gen2brain/iup-go/tree/main/docs
// [examples]: https://github.com/gen2brain/iup-go/tree/main/examples
package iup

//go:generate go run ./internal/genversion
