//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif

package iup

/*
#include "external/src/gtk/iupgtk_info.c"
*/
import "C"
