//go:build ios && !gtk && !gtk4 && !qt && !motif && !efl && !fltk

package iup

/*
#include "external/src/cocoatouch/iupcocoatouch_fontdlg.m"
*/
import "C"
