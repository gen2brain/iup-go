//go:build (!windows && !qt) || ((windows || darwin) && (gtk || gtk4)) || motif

package iup

/*
#include "external/src/iup_datepick.c"
*/
import "C"
