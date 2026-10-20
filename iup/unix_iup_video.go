//go:build media && !android && !darwin && !ios && !windows && !js

package iup

/*
#include "external/srcmedia/iupunix_video.c"
#include "external/srcmedia/iupvideo_vlc.c"
#include "external/srcmedia/iupvideo_mpv.c"
#include "external/srcmedia/iupvideo_gstreamer.c"
*/
import "C"
