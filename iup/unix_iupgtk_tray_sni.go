//go:build (((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4) || gtk) && !xembed

package iup

/*
#include "external/src/gtk/iupgtk_tray_sni.c"
*/
import "C"
