//go:build (qt6 || qt5) && !windows && !darwin && !android && !haiku

package iup

/*
#include "external/src/unix/iupunix_sensor.c"
*/
import "C"
