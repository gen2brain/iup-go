//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl && !fltk

package iup

/*
#include "external/src/unix/iupunix_singleinstance.c"
*/
import "C"
