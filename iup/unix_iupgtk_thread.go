//go:build ((!windows && !darwin && !motif && !qt && !gtk4 && !efl && !fltk && !gnustep && !haiku) || (gtk && (windows || darwin))) && !android

package iup

/*
#include "external/src/gtk/iupgtk_thread.c"
*/
import "C"
