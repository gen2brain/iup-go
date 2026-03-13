//go:build efl && !windows && !darwin

package iup

/*
#include "external/src/efl/iupefl_tray.c"
#include "external/src/unix/iupunix_sni.c"
*/
import "C"
