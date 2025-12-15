//go:build (!windows && !qt) || ((windows || darwin) && gtk) || motif

package iup

/*
#include "external/src/iup_datepick.c"
*/
import "C"
