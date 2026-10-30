//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !android && web && (fltk || motif || efl || gnustep)

package iup

/*
#include "external/srcweb/iupgtkhost_webbrowser.c"
*/
import "C"
