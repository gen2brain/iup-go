//go:build (!windows && !qt) || ((windows || darwin) && (gtk || gtk4)) || motif || winui || efl || fltk

package iup

/*
#include "external/src/iup_datepick.c"
*/
import "C"
