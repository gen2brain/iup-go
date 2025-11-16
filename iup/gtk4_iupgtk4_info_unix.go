//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && gtk4

package iup

/*
#include "external/src/gtk4/iupgtk4_info.c"
#include "external/src/mot/iupunix_info.c"
*/
import "C"
