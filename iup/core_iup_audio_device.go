//go:build media && !haiku && !android && !js

package iup

/*
#include "external/srcmedia/iup_audio_device.c"
*/
import "C"
