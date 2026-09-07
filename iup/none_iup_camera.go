//go:build media && !windows && !darwin && !linux && !freebsd && !dragonfly

package iup

/*
#include "external/srcmedia/iup_camera_none.c"
*/
import "C"
