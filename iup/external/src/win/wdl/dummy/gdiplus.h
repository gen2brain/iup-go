/*
 * WinDrawLib
 * Copyright (c) 2015 Martin Mitas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef DUMMY_GDIPLUS_H
#define DUMMY_GDIPLUS_H

#include <windows.h>


/* MSDN documentation for <gdiplus/gdiplusflat.h> sucks. This one is better:
 * http://www.jose.it-berater.org/gdiplus/iframe/index.htm
 */


/* Note we don't declare any functions here: We load them dynamically anyway.
 */


typedef DWORD dummy_ARGB;

typedef INT dummy_GpPixelFormat;
#define    dummy_PixelFormatGDI          0x00020000 // Is a GDI-supported format
#define    dummy_PixelFormatAlpha        0x00040000 // Has an alpha component
#define    dummy_PixelFormatPAlpha       0x00080000 // Pre-multiplied alpha
#define    dummy_PixelFormatCanonical    0x00200000 
#define    dummy_PixelFormat24bppRGB        (8 | (24 << 8) | dummy_PixelFormatGDI)
#define    dummy_PixelFormat32bppARGB       (10 | (32 << 8) | dummy_PixelFormatAlpha | dummy_PixelFormatGDI | dummy_PixelFormatCanonical)
#define    dummy_PixelFormat32bppPARGB      (11 | (32 << 8) | dummy_PixelFormatAlpha | dummy_PixelFormatPAlpha | dummy_PixelFormatGDI)

#define    dummy_ImageLockModeWrite 2

/*****************************
 ***  Helper Enumerations  ***
 *****************************/

typedef enum dummy_GpMatrixOrder_tag dummy_GpMatrixOrder;
enum dummy_GpMatrixOrder_tag {
    dummy_MatrixOrderPrepend = 0,
    dummy_MatrixOrderAppend = 1
};

typedef enum dummy_GpCombineMode_tag dummy_GpCombineMode;
enum dummy_GpCombineMode_tag {
    dummy_CombineModeReplace = 0,
    dummy_CombineModeIntersect = 1,
    dummy_CombineModeUnion = 2,
    dummy_CombineModeXor = 3,
    dummy_CombineModeExclude = 4,
    dummy_CombineModeComplement = 5
};

typedef enum dummy_GpPixelOffsetMode_tag dummy_GpPixelOffsetMode;
enum dummy_GpPixelOffsetMode_tag {
    dummy_PixelOffsetModeInvalid = -1,
    dummy_PixelOffsetModeDefault = 0,
    dummy_PixelOffsetModeHighSpeed = 1,
    dummy_PixelOffsetModeHighQuality = 2,
    dummy_PixelOffsetModeNone = 3,
    dummy_PixelOffsetModeHalf = 4
};

typedef enum dummy_GpSmoothingMode_tag dummy_GpSmoothingMode;
enum dummy_GpSmoothingMode_tag {
    dummy_SmoothingModeInvalid = -1,
    dummy_SmoothingModeDefault = 0,
    dummy_SmoothingModeHighSpeed = 1,
    dummy_SmoothingModeHighQuality = 2,
    dummy_SmoothingModeNone = 3,
    dummy_SmoothingModeAntiAlias8x4 = 4,
    dummy_SmoothingModeAntiAlias = 4,
    dummy_SmoothingModeAntiAlias8x8 = 5
};

typedef enum dummy_GpUnit_tag dummy_GpUnit;
enum dummy_GpUnit_tag {
    dummy_UnitWorld = 0,
    dummy_UnitDisplay = 1,
    dummy_UnitPixel = 2,
    dummy_UnitPoint = 3,
    dummy_UnitInch = 4,
    dummy_UnitDocument = 5,
    dummy_UnitMillimeter = 6
};

typedef enum dummy_GpFillMode_tag dummy_GpFillMode;
enum dummy_GpFillMode_tag {
    dummy_FillModeAlternate = 0,
    dummy_FillModeWinding = 1
};

typedef enum dummy_GpStringAlignment_tag dummy_GpStringAlignment;
enum dummy_GpStringAlignment_tag {
    dummy_StringAlignmentNear = 0,
    dummy_StringAlignmentCenter = 1,
    dummy_StringAlignmentFar = 2
};

typedef enum dummy_GpStringFormatFlags_tag dummy_GpStringFormatFlags;
enum dummy_GpStringFormatFlags_tag {
    dummy_StringFormatFlagsDirectionRightToLeft = 0x00000001,
    dummy_StringFormatFlagsDirectionVertical = 0x00000002,
    dummy_StringFormatFlagsNoFitBlackBox = 0x00000004,
    dummy_StringFormatFlagsDisplayFormatControl = 0x00000020,
    dummy_StringFormatFlagsNoFontFallback = 0x00000400,
    dummy_StringFormatFlagsMeasureTrailingSpaces = 0x00000800,
    dummy_StringFormatFlagsNoWrap = 0x00001000,
    dummy_StringFormatFlagsLineLimit = 0x00002000,
    dummy_StringFormatFlagsNoClip = 0x00004000
};

typedef enum dummy_GpStringTrimming_tag dummy_GpStringTrimming;
enum dummy_GpStringTrimming_tag {
    dummy_StringTrimmingNone = 0,
    dummy_StringTrimmingCharacter = 1,
    dummy_StringTrimmingWord = 2,
    dummy_StringTrimmingEllipsisCharacter = 3,
    dummy_StringTrimmingEllipsisWord = 4,
    dummy_StringTrimmingEllipsisPath = 5
};

typedef enum dummy_GpLineCap_tag dummy_GpLineCap;
enum dummy_GpLineCap_tag {
  dummy_LineCapFlat = 0,
  dummy_LineCapSquare = 1,
  dummy_LineCapRound = 2,
  dummy_LineCapTriangle = 3,
};

typedef enum dummy_GpLineJoin_tag dummy_GpLineJoin;
enum dummy_GpLineJoin_tag {
  dummy_LineJoinMiter = 0,
  dummy_LineJoinBevel = 1,
  dummy_LineJoinRound = 2
};

typedef enum dummy_GpDashStyle_tag dummy_GpDashStyle;
enum dummy_GpDashStyle_tag {
  dummy_DashStyleSolid = 0,
  dummy_DashStyleDash = 1,
  dummy_DashStyleDot = 2,
  dummy_DashStyleDashDot = 3,
  dummy_DashStyleDashDotDot = 4,
  dummy_DashStyleCustom = 5
};


/***************************
 ***  Helper Structures  ***
 ***************************/

typedef struct dummy_GpStartupInput_tag dummy_GpStartupInput;
struct dummy_GpStartupInput_tag {
    UINT32 GdiplusVersion;
    void* DebugEventCallback;  /* DebugEventProc (not used) */
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
};

typedef struct dummy_GpPointF_tag dummy_GpPointF;
struct dummy_GpPointF_tag {
    float x;
    float y;
};

typedef struct dummy_GpRectF_tag dummy_GpRectF;
struct dummy_GpRectF_tag {
    float x;
    float y;
    float w;
    float h;
};

typedef struct dummy_GpRectI_tag dummy_GpRectI;
struct dummy_GpRectI_tag {
    INT x;
    INT y;
    INT w;
    INT h;
};

typedef struct dummy_GpBitmapData_tag dummy_GpBitmapData;
struct dummy_GpBitmapData_tag {
    UINT width;
    UINT height;
    INT Stride;
    dummy_GpPixelFormat PixelFormat;
    void *Scan0;
    UINT_PTR Reserved;
};


/**********************
 ***  GDI+ Objects  ***
 **********************/

typedef struct dummy_GpBrush_tag        dummy_GpBrush;
typedef struct dummy_GpCachedBitmap_tag dummy_GpCachedBitmap;
typedef struct dummy_GpFont_tag         dummy_GpFont;
typedef struct dummy_GpGraphics_tag     dummy_GpGraphics;
typedef struct dummy_GpImage_tag        dummy_GpImage;
typedef struct dummy_GpPath_tag         dummy_GpPath;
typedef struct dummy_GpPen_tag          dummy_GpPen;
typedef struct dummy_GpStringFormat_tag dummy_GpStringFormat;

/* These are "derived" from the types aboves (more specialized). */
typedef struct dummy_GpImage_tag        dummy_GpBitmap;
typedef struct dummy_GpBrush_tag        dummy_GpSolidFill;


#endif  /* DUMMY_GDIPLUS_H */
