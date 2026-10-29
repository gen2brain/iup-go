//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !motif && !qt6 && !qt5 && !qml && !efl && !fltk && !gnustep && !android

package iup

/*
#include "external/src/unix/iupunix_portal.c"
*/
import "C"
