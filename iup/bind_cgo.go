package iup

/*
#cgo CFLAGS: -Iexternal/include -Iexternal/src
#cgo ctl CFLAGS: -Iexternal/srcctl
#cgo gl CFLAGS: -Iexternal/srcgl
#cgo web CFLAGS: -Iexternal/srcweb

#cgo CXXFLAGS: -Iexternal/include -Iexternal/src
#cgo CXXFLAGS: -std=c++17
#cgo web CXXFLAGS: -Iexternal/srcweb

#cgo linux LDFLAGS: -ldl
#cgo !windows,!darwin LDFLAGS: -lm

#cgo !windows,!darwin,!qt,!efl CFLAGS: -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DIUPDBUS_USE_DLOPEN
#cgo gtk,gtk2,gtk4 CFLAGS: -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED

#cgo !windows,!darwin,!motif,!qt,!gtk2,!gtk4,!efl CFLAGS: -Iexternal/src/gtk -DIUP_USE_GTK3
#cgo !windows,!darwin,!motif,!qt,!gtk2,gtk4,!efl CFLAGS: -Iexternal/src/gtk4 -DIUP_USE_GTK4
#cgo !windows,!darwin,!motif,!qt,gtk2,!gtk4,!efl CFLAGS: -Iexternal/src/gtk -DIUP_USE_GTK2

#cgo qt CFLAGS: -Iexternal/src/qt -DIUP_USE_QT
#cgo qt CXXFLAGS: -Iexternal/src/qt -DIUP_USE_QT
#cgo qt,!windows,!darwin CFLAGS: -DIUPDBUS_USE_DLOPEN

#cgo !windows,!darwin,!motif,!qt,!gtk2,!gtk4,!efl,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0 gdk-wayland-3.0 gdk-x11-3.0
#cgo !windows,!darwin,!motif,!qt,!gtk2,gtk4,!efl,!nopkgconfig pkg-config: gtk4 gtk4-wayland gtk4-x11
#cgo !windows,!darwin,!motif,!qt,gtk2,!gtk4,!efl,!nopkgconfig pkg-config: gtk+-2.0 gdk-2.0 x11
#cgo !windows,!darwin,!motif,!qt,!efl,web CFLAGS: -DIUPWEB_USE_DLOPEN

#cgo !windows,!darwin,!motif,!gtk2,!efl,gl,!nopkgconfig pkg-config: wayland-egl egl gl
#cgo !windows,!darwin,!motif,!qt,gtk2,!efl,gl,!nopkgconfig pkg-config: gl

#cgo qt,!qt5,!nopkgconfig pkg-config: Qt6Core Qt6Gui Qt6Widgets
#cgo qt,qt5,!nopkgconfig pkg-config: Qt5Core Qt5Gui Qt5Widgets
#cgo qt,!qt5,web,!nopkgconfig pkg-config: Qt6WebEngineCore Qt6WebEngineWidgets
#cgo qt,qt5,web,!nopkgconfig pkg-config: Qt5WebEngineCore  Qt5WebEngineWidgets

#cgo motif LDFLAGS: -lXm -lXmu -lXt -lXext -lX11
#cgo linux,motif LDFLAGS: -lXpm
#cgo darwin,motif LDFLAGS: -liconv
#cgo motif,gl LDFLAGS: -lGL
#cgo motif,xft CFLAGS: -DIUP_USE_XFT
#cgo motif,xft,!nopkgconfig pkg-config: xft freetype2
#cgo motif CFLAGS: -Iexternal/src/mot -DIUP_USE_ICONV

#cgo windows CFLAGS: -Iexternal/src/win -Iexternal/src/win/wdl
#cgo windows,!gtk,!gtk4,!qt,!winui CFLAGS: -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -DCOBJMACROS -DNOTREEVIEW -DUNICODE -D_UNICODE
#cgo windows LDFLAGS: -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32
#cgo windows,gl LDFLAGS: -lopengl32

#cgo windows,gtk CFLAGS: -Iexternal/src/gtk -DIUP_USE_GTK3
#cgo windows,gtk,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0

#cgo windows,gtk4 CFLAGS: -Iexternal/src/gtk4 -DIUP_USE_GTK4
#cgo windows,gtk4,!nopkgconfig pkg-config: gtk4

#cgo winui CFLAGS: -Iexternal/src/winui -DIUP_USE_WINUI -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DNTDDI_VERSION=0x0A000000
#cgo winui CXXFLAGS: -Iexternal/src/winui -DIUP_USE_WINUI -D_WIN32_WINNT=0x0A00 -DNTDDI_VERSION=0x0A000000 -DUNICODE -D_UNICODE -std=c++20 -stdlib=libc++
#cgo winui LDFLAGS: -lwindowsapp -lruntimeobject -ld2d1 -ld3d11 -ldxgi -ldwrite -lole32 -loleaut32 -luuid -luser32 -lshell32 -l:libc++.a -l:libc++abi.a

#cgo darwin CFLAGS: -Iexternal/src/cocoa -x objective-c
#cgo darwin LDFLAGS: -framework SystemConfiguration -framework QuartzCore -framework AppKit -framework UserNotifications
#cgo darwin,gl LDFLAGS: -framework OpenGL
#cgo darwin,!gtk,!gtk4,!qt,web LDFLAGS: -framework WebKit

#cgo darwin,gtk CFLAGS: -Iexternal/src/gtk -DIUP_USE_GTK3 -x objective-c
#cgo darwin,gtk,!nopkgconfig pkg-config: gtk+-3.0 gdk-3.0
#cgo darwin,gtk,web CFLAGS: -DIUPWEB_USE_DLOPEN

#cgo darwin,gtk4 CFLAGS: -Iexternal/src/gtk4 -DIUP_USE_GTK4 -x objective-c
#cgo darwin,gtk4,!nopkgconfig pkg-config: gtk4
#cgo darwin,gtk4,web CFLAGS: -DIUPWEB_USE_DLOPEN

#cgo efl CFLAGS: -Iexternal/src/efl -DIUP_USE_EFL -DEFL_BETA_API_SUPPORT=1 -DEFL_EO_API_SUPPORT=1 -DHAVE_ECORE_X -DHAVE_ECORE_WL2
#cgo efl,!nopkgconfig pkg-config: elementary ecore-x ecore-wl2 dbus-1
#cgo efl,gl,!nopkgconfig pkg-config: wayland-egl egl gl
*/
import "C"
