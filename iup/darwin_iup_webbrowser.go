//go:build darwin && web && !gtk && !gtk4 && !qt && !motif

package iup

/*
#include "external/srcweb/iupcocoa_webbrowser.m"
*/
import "C"
