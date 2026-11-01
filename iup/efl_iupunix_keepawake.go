//go:build efl && !windows && !darwin && !android

package iup

/*
#include "external/src/unix/iupunix_keepawake.c"
*/
import "C"
