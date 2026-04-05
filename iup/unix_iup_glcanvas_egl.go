//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && gl && !motif && !gtk2

package iup

/*
#include "external/srcgl/iup_glcanvas_egl.c"
*/
import "C"
