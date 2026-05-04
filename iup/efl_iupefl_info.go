//go:build efl && !windows && !darwin && !android

package iup

/*
#include "external/src/efl/iupefl_info.c"
#include "external/src/unix/iupunix_info.c"
*/
import "C"
