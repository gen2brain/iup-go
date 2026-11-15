//go:build (darwin && !ios && gl) || (gnustep && gl)

package iup

/*
#include "external/srcgl/iup_glcanvas_cocoa.m"
*/
import "C"
