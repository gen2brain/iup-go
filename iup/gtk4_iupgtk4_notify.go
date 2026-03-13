//go:build gtk4 && !windows && !darwin

package iup

/*
#include "external/src/unix/iupunix_notify.c"
*/
import "C"
