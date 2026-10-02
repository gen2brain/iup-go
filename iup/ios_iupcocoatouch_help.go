//go:build ios && !gtk3 && !gtk4 && !qt && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoatouch/iupcocoatouch_help.m"
*/
import "C"
