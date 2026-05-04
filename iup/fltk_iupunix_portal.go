//go:build fltk && !windows && !darwin && !android

package iup

/*
#include "external/src/unix/iupunix_portal.c"
*/
import "C"
