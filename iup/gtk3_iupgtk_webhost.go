//go:build (windows || (darwin && !ios)) && gtk3 && web

package iup

/*
#include "external/srcweb/iupgtk_webhost.c"
*/
import "C"
