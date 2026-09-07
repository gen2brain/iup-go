//go:build (linux || freebsd || dragonfly) && media && !android

package iup

/*
#include "external/srcmedia/iupunix_camera.c"
*/
import "C"
