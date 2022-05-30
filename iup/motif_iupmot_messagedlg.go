//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && motif

package iup

/*
#include "../external/src/mot/iupmot_messagedlg.c"
*/
import "C"