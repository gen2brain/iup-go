//go:build gtk4 && !windows && !darwin && !android

package iup

/*
#include "external/src/unix/iupunix_location.c"
*/
import "C"
