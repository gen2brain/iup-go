//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4) || gtk

package iup

/*
#include "external/src/gtk/iupgtk_val.c"
*/
import "C"
