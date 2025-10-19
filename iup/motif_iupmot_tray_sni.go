//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && motif) && !xembed

package iup

/*
#include "external/src/mot/iupmot_tray_sni.c"
*/
import "C"