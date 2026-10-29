//go:build ios && web && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/srcweb/iupcocoatouch_webbrowser.m"
*/
import "C"
