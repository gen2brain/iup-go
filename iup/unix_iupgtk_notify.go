//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt && !gtk4 && !efl

package iup

/*
#include "external/src/mot/iupunix_notify.c"
*/
import "C"
