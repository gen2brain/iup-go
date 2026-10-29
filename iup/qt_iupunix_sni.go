//go:build (qt6 || qt5) && (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && !android

package iup

/*
#include "external/src/unix/iupunix_sni.c"
*/
import "C"
