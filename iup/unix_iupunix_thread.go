//go:build (darwin && !gtk && !gtk4 && !qt && !efl) || motif || ((efl || qt) && !windows)

package iup

/*
#include "external/src/unix/iupunix_thread.c"
*/
import "C"
