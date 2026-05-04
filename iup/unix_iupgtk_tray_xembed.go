//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl && !fltk && !gnustep && xembed && !android

package iup

/*
#include "external/src/gtk/iupgtk_tray_xembed.c"
*/
import "C"
