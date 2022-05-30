//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && motif

package iup

/*
#include "../external/src/mot/iupmot_canvas.c"
*/
import "C"