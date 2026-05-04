//go:build ((!windows && !darwin && !motif && !qt && !gtk4 && !efl && !fltk && !gnustep) || (gtk && (windows || darwin))) && !android

package iup

/*
#include "external/src/gtk/iupgtk_thread.c"
*/
import "C"
