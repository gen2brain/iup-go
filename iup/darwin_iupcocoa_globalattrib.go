//go:build darwin && !gtk && !gtk4 && !qt && !motif

package iup

/*
#include "external/src/cocoa/iupcocoa_globalattrib.m"
*/
import "C"
