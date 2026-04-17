//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !efl && !fltk && !gnustep

package iup

/*
#include "external/src/unix/iupunix_portal.c"
*/
import "C"
