//go:build ((!windows && !darwin && !motif && !qt6 && !qt5 && !qml && !gtk4 && !efl && !fltk && !gnustep && !haiku) || (gtk3 && (windows || darwin))) && !android && !js

package iup

/*
#include "external/src/gtk/iupgtk_thread.c"
*/
import "C"
