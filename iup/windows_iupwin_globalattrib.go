//go:build windows && !gtk && !gtk4 && !qt && !winui && !efl

package iup

/*
#include "external/src/win/iupwin_globalattrib.c"
*/
import "C"