//go:build gtk4 && !windows && !darwin

package iup

/*
#include "external/src/gtk4/iupgtk4_tray.c"
#include "external/src/mot/iupunix_sni.c"
*/
import "C"
