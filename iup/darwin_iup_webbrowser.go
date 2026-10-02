//go:build darwin && !ios && web && !gtk3 && !gtk4 && !qt && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/srcweb/iupcocoa_webbrowser.m"
*/
import "C"
