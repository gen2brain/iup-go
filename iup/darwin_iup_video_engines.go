//go:build darwin && !ios && media

package iup

/*
#include "external/srcmedia/iupvideo_mpv.c"
#include "external/srcmedia/iupvideo_vlc.c"
#include "external/srcmedia/iupvideo_gstreamer.c"
*/
import "C"
