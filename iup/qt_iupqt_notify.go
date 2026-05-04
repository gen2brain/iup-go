//go:build qt && !windows && !darwin && !android

package iup

/*
#include "external/src/unix/iupunix_notify.c"
*/
import "C"
