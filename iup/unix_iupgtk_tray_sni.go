//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !xembed

package iup

/*
#include "external/src/gtk/iupgtk_tray_sni.c"
#include "external/src/mot/iupunix_sni.c"
*/
import "C"
