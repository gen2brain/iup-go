//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl) || gtk

package iup

/*
#include "external/src/gtk/iupgtk_messagedlg.c"
*/
import "C"
