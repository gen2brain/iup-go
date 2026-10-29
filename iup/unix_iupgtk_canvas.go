//go:build (((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt6 && !qt5 && !qml && !gtk4 && !efl && !fltk && !gnustep) || gtk3) && !android

package iup

/*
#include "external/src/gtk/iupgtk_canvas.c"
*/
import "C"
