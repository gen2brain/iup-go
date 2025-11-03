//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt)

package iup

/*
#include "external/src/gtk/iupgtk_help.c"
*/
import "C"
