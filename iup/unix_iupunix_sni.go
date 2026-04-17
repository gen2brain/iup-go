//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl && !fltk && !gnustep && !xembed

package iup

/*
#include "external/src/unix/iupunix_sni.c"
*/
import "C"
