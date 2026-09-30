//go:build qml && !windows && !darwin && !android && !haiku

package iup

/*
#include "external/src/unix/iupunix_singleinstance.c"
*/
import "C"
