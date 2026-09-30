//go:build ios && !gtk && !gtk4 && !qt && !qml && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoatouch/iupcocoatouch_filedlg.m"
*/
import "C"
