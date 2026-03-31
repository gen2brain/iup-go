//go:build fltk && !windows && !darwin

package iup

/*
#include "external/src/unix/iupunix_notify.c"
*/
import "C"
