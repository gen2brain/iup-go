//go:build darwin && !ios && web && (((qt6 || qt5 || qml) && !webengine) || gtk3 || gtk4 || fltk)

package iup

/*
#include "external/srcweb/iupwkhost_webbrowser.m"
*/
import "C"
