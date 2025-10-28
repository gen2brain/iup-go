//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && web && !motif

package iup

/*
#include "external/srcweb/iupgtk_webbrowser.c"
*/
import "C"
