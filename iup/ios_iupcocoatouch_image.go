//go:build ios && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoatouch/iupcocoatouch_image.m"
*/
import "C"
