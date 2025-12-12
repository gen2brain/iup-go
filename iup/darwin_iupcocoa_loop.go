//go:build darwin && !gtk && !qt && !motif

package iup

/*
#include "external/src/cocoa/iupcocoa_loop.m"
*/
import "C"