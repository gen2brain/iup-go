//go:build ((aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) || (darwin && gtk3)) && web && !motif && !qt && !qml && !gnustep && !android

package iup

/*
#include "external/srcweb/iupgtk_webbrowser.c"
*/
import "C"
