//go:build (windows || (darwin && !ios)) && gtk4 && web

package iup

/*
#include "external/srcweb/iupgtk4_webhost.c"
*/
import "C"
