//go:build darwin && !ios && !motif

package iup

/*
#include "external/src/cocoa/iupcocoa_keepawake.m"
*/
import "C"
