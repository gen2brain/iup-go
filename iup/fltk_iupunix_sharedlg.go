//go:build fltk && !windows && !darwin && !android

package iup

/*
#include "external/src/unix/iupunix_sharedlg.c"
*/
import "C"
