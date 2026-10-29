//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && web && !motif && !efl && !fltk && !qt6 && !qt5 && !qml && !gnustep && !android

package iup

/*
#include "external/srcweb/iupgtk_webbrowser.c"
*/
import "C"
