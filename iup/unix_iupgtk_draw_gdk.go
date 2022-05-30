//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && gtk2) || (gtk && gtk2)

package iup

/*
#include "../external/src/gtk/iupgtk_draw_gdk.c"
*/
import "C"
