//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl)

package iup

/*
#include "external/src/unix/iupunix_help.c"
*/
import "C"
