//go:build darwin && !gtk && !gtk4 && !qt && !motif

package iup

/*
#include "external/src/cocoa/iupcocoa_nibs.m"
*/
import "C"
