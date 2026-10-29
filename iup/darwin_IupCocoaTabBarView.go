//go:build (darwin && !ios && !gtk3 && !gtk4 && !qt6 && !qt5 && !qml && !motif && !efl && !fltk) || gnustep

package iup

/*
#include "external/src/cocoa/IupCocoaTabBarView.m"
*/
import "C"
