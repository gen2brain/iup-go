//go:build motif || (efl && !windows)

package iup

/*
#include "external/src/unix/iupunix_thread.c"
*/
import "C"
