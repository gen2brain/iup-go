//go:build darwin && !gtk && !gtk4 && !qt && !motif && !efl

package iup

/*
#include "external/src/cocoa/IupCocoaTabBarView.m"
*/
import "C"
