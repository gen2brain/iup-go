//go:build darwin && web && !gtk && !gtk4 && !qt && !motif && !efl && !fltk

package iup

/*
#include "external/srcweb/iupcocoa_webbrowser.m"
*/
import "C"
