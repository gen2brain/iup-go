//go:build darwin && !ios && !motif

package iup

/*
#include "external/src/cocoa/iupcocoa_singleinstance.m"
*/
import "C"
