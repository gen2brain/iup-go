//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt) || gtk

package iup

/*
#include "external/src/gtk/iupgtk_canvas.c"
*/
import "C"
