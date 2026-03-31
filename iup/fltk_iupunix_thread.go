//go:build fltk && !windows && !darwin

package iup

/*
#include "external/src/unix/iupunix_thread.c"
*/
import "C"
