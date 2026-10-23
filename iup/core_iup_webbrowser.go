//go:build web && !efl && !motif && !gnustep && (!fltk || windows || (darwin && !ios))

package iup

/*
#include "external/srcweb/iup_webbrowser.c"
*/
import "C"
