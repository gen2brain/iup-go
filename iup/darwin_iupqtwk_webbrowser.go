//go:build darwin && !ios && (qt || qml) && web && !webengine

package iup

/*
#include "external/srcweb/iupqtwk_webbrowser.m"
*/
import "C"
