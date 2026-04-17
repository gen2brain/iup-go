//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) || gtk) && web && !motif && !qt && !gnustep

package iup

/*
#include "external/srcweb/iupgtk_webbrowser.c"
*/
import "C"
