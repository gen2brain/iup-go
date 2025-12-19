//go:build windows && !gtk && !gtk4 && !qt

package iup

/*
// WDL (Windows Drawing Library) - Direct2D/DirectWrite backend for Windows
#include "external/src/win/wdl/backend-d2d.c"
#include "external/src/win/wdl/backend-dwrite.c"
#include "external/src/win/wdl/backend-gdix.c"
#include "external/src/win/wdl/backend-wic.c"
#include "external/src/win/wdl/bitblt.c"
#include "external/src/win/wdl/brush.c"
#include "external/src/win/wdl/cachedimage.c"
#include "external/src/win/wdl/canvas.c"
#include "external/src/win/wdl/draw.c"
#include "external/src/win/wdl/fill.c"
#include "external/src/win/wdl/font.c"
#include "external/src/win/wdl/image.c"
#include "external/src/win/wdl/init.c"
#include "external/src/win/wdl/memstream.c"
#include "external/src/win/wdl/misc.c"
#include "external/src/win/wdl/path.c"
#include "external/src/win/wdl/string.c"
#include "external/src/win/wdl/strokestyle.c"
*/
import "C"
