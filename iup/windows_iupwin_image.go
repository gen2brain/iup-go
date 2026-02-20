//go:build windows && !gtk && !gtk4 && !qt && !winui

package iup

/*
#include "external/src/win/iupwin_image.c"
*/
import "C"