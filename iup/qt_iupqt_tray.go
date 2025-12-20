//go:build qt && (aix || dragonfly || freebsd || linux || netbsd || openbsd || solaris || illumos)

package iup

/*
#include "external/src/mot/iupunix_sni.c"
*/
import "C"
