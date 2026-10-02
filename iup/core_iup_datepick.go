//go:build ((!windows && !qt && !android && !ios) || ((windows || darwin) && (gtk3 || gtk4)) || motif || winui || efl || fltk || qml) && !js

package iup

/*
#include "external/src/iup_datepick.c"
*/
import "C"
