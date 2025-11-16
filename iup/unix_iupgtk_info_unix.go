//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4

package iup

/*
#include "external/src/gtk/iupgtk_info.c"
#include "external/src/mot/iupunix_info.c"
*/
import "C"
