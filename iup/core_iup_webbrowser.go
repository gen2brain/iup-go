//go:build web && !(motif && darwin) && !(efl && (windows || darwin))

package iup

/*
#include "external/srcweb/iup_webbrowser.c"
*/
import "C"
