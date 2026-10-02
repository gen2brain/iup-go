//go:build ((!windows && !darwin && !motif && !qt && !qml && !gtk4 && !efl && !fltk && !gnustep && !haiku) || (gtk3 && (windows || darwin))) && !android && !js

package iup

/*
#include "external/src/gtk/iupgtk_thread.c"
*/
import "C"
