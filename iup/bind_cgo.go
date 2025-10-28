package iup

/*
#cgo CFLAGS: -Iexternal/include -Iexternal/src -w
#cgo CXXFLAGS: -Iexternal/include -Iexternal/src -w

#cgo gl CFLAGS: -Iexternal/srcgl
#cgo web CFLAGS: -Iexternal/srcweb
#cgo web CXXFLAGS: -Iexternal/srcweb

#cgo !windows,!darwin,!motif,!gtk2 CFLAGS: -Iexternal/src/gtk -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED
#cgo !windows,!darwin,!motif,gtk2 CFLAGS: -Iexternal/src/gtk
#cgo !windows,!darwin,motif CFLAGS: -Iexternal/src/mot -DIUPDBUS_USE_DLOPEN
#cgo !windows,!darwin,!motif,web CFLAGS: -DIUPWEB_USE_DLOPEN
#cgo !windows,!darwin LDFLAGS: -lm

#cgo !windows,!darwin,!motif,!gtk2,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0 gdk-wayland-3.0 gdk-x11-3.0 x11
#cgo !windows,!darwin,!motif,!gtk2,gl,!nopkgconfig pkg-config: wayland-egl egl
#cgo !windows,!darwin,!motif,gtk2,!nopkgconfig pkg-config: gtk+-2.0 gdk-2.0 x11
#cgo !windows,!darwin,!motif,gtk2,gl,!nopkgconfig pkg-config: gl

#cgo !windows,!darwin,motif LDFLAGS: -lXm -lXmu -lXt -lXext -lX11
#cgo !windows,!darwin,linux,motif LDFLAGS: -lXpm
#cgo !windows,!darwin,motif,gl LDFLAGS: -lGL

#cgo windows CFLAGS: -Iexternal/src/win -Iexternal/src/win/wdl -DUSE_NEW_DRAW
#cgo windows CFLAGS: -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -DCOBJMACROS -DNOTREEVIEW -DUNICODE -D_UNICODE

#cgo windows,gtk CFLAGS: -Iexternal/src/gtk -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED
#cgo windows,gtk,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0
#cgo windows LDFLAGS: -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32
#cgo windows,gl LDFLAGS: -lopengl32

#cgo darwin,!gtk CFLAGS: -Iexternal/src/cocoa -x objective-c
#cgo darwin,!gtk LDFLAGS: -framework SystemConfiguration -framework QuartzCore -framework Cocoa
#cgo darwin,!gtk,gl LDFLAGS: -framework OpenGL
#cgo darwin,!gtk,web LDFLAGS: -framework WebKit

#cgo darwin,gtk CFLAGS: -Iexternal/src/gtk -x objective-c -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGTK_MAC
#cgo darwin,gtk LDFLAGS: -framework SystemConfiguration -framework QuartzCore
#cgo darwin,gtk,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0 gdk-quartz-3.0
#cgo darwin,gtk,gl LDFLAGS: -framework OpenGL
#cgo darwin,gtk,web CFLAGS: -DIUPWEB_USE_DLOPEN
*/
import "C"
