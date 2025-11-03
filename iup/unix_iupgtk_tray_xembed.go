//go:build (((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt) || gtk) && xembed

package iup

/*
#include "external/src/gtk/iupgtk_tray_xembed.c"
*/
import "C"
