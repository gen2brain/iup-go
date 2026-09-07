//go:build (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos) && media && !android

package iup

/*
#include "external/srcmedia/iupunix_camera.c"
*/
import "C"
