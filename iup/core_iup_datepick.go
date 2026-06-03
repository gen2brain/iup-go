//go:build ((!windows && !qt && !android && !ios) || ((windows || darwin) && (gtk || gtk4)) || motif || winui || efl || fltk) && !js

package iup

/*
#include "external/src/iup_datepick.c"
*/
import "C"
