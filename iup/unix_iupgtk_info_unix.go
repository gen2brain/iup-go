//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl && !fltk && !gnustep

package iup

/*
#include "external/src/gtk/iupgtk_info.c"
#include "external/src/unix/iupunix_info.c"
*/
import "C"
