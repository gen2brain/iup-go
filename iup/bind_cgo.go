package iup

/*
#cgo CFLAGS: -Iexternal/include -Iexternal/src -Iexternal/srcgl -w

#cgo !windows,!darwin,!motif,gl CFLAGS: -Iexternal/src/gtk -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED
#cgo !windows,!darwin,!motif,!gl CFLAGS: -Iexternal/src/gtk -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_NULL
#cgo !windows,!darwin,motif CFLAGS: -Iexternal/src/mot

#cgo !windows,!darwin,!motif,!gtk2,gl,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0 gl
#cgo !windows,!darwin,!motif,gtk2,gl,!nopkgconfig pkg-config: gtk+-2.0 gdk-2.0 gl
#cgo !windows,!darwin,!motif,!gtk2,!gl,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0
#cgo !windows,!darwin,!motif,gtk2,!gl,!nopkgconfig pkg-config: gtk+-2.0 gdk-2.0

#cgo !windows,!darwin,!motif,gl LDFLAGS: -lX11
#cgo !windows,!darwin,!motif LDFLAGS: -lm

#cgo !windows,!darwin,!linux,motif,gl LDFLAGS: -lXm -lXmu -lXt -lXext -lX11 -lGL -lm
#cgo !windows,!darwin,linux,motif,gl LDFLAGS: -lXm -lXmu -lXt -lXpm -lXext -lX11 -lGL -lm
#cgo !windows,!darwin,!linux,motif,!gl LDFLAGS: -lXm -lXmu -lXt -lXext -lX11 -lm
#cgo !windows,!darwin,linux,motif,!gl LDFLAGS: -lXm -lXmu -lXt -lXpm -lXext -lX11 -lm

#cgo windows CFLAGS: -Iexternal/src/win -Iexternal/src/win/wdl -DUSE_NEW_DRAW
#cgo windows CFLAGS: -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -DCOBJMACROS -DNOTREEVIEW -DUNICODE -D_UNICODE

#cgo windows,gtk CFLAGS: -Iexternal/src/gtk -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_NULL
#cgo windows,gtk,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0
#cgo windows,gl LDFLAGS: -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32 -lopengl32
#cgo windows,!gl LDFLAGS: -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32

#cgo darwin,!gtk CFLAGS: -Iexternal/src/cocoa -x objective-c
#cgo darwin,!gtk LDFLAGS: -framework SystemConfiguration -framework QuartzCore -framework Cocoa -mmacosx-version-min=10.14

#cgo darwin,gtk CFLAGS: -Iexternal/src/gtk -x objective-c -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_NULL -DGTK_MAC
#cgo darwin,gtk,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0
#cgo darwin,gtk LDFLAGS: -framework SystemConfiguration -framework QuartzCore -mmacosx-version-min=10.14
*/
import "C"
