//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt) || gtk

package iup

/*
#include "external/src/gtk/iupgtk_tabs.c"
*/
import "C"
