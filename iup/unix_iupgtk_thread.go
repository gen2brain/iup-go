//go:build (!windows && !darwin && !motif && !qt && !gtk4 && !efl && !fltk && !gnustep) || (gtk && (windows || darwin))

package iup

/*
#include "external/src/gtk/iupgtk_thread.c"
*/
import "C"
