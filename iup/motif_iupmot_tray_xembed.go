//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && motif) && xembed

package iup

/*
#include "external/src/mot/iupmot_tray_xembed.c"
*/
import "C"