//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && motif

package iup

/*
#include "../external/src/mot/iupx11_info.c"
*/
import "C"