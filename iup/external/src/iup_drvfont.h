/** \file
 * \brief Driver Font Management
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_DRVFONT_H 
#define __IUP_DRVFONT_H

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup drvfont Driver Font Interface 
 * \par
 * Each driver must export the symbols defined here.
 * \par
 * See \ref iup_drvfont.h 
 * \ingroup drv */

/* Called only from IupOpen/IupClose. */
void iupdrvFontInit(void);
void iupdrvFontFinish(void);

/** Retrieve the character size for the selected font.
 * Should be used only to calculate the SIZE attribute.
 * \ingroup drvfont */
IUP_SDK_API void iupdrvFontGetCharSize(Ihandle* ih, int *charwidth, int *charheight);

/** Retrieve the string width for the selected font.
 * \ingroup drvfont */
IUP_SDK_API int iupdrvFontGetStringWidth(Ihandle* ih, const char* str);

/** Retrieve the multi-lined string size for the selected font. \n
 * Width is the maximum line width. \n
 * Height is charheight*number_of_lines (this will avoid line size variations).
 * \ingroup drvfont */
IUP_SDK_API void iupdrvFontGetMultiLineStringSize(Ihandle* ih, const char* str, int *w, int *h);

/** Same as \ref iupdrvFontGetMultiLineStringSize but not associated with a control. 
 * Used in IupDraw.\n
 *\ingroup drvfont */
IUP_SDK_API void iupdrvFontGetTextSize(const char* font, const char* str, int len, int *w, int *h);

/** Returns information about the font. \n
 *\ingroup drvfont */
IUP_SDK_API void iupdrvFontGetFontDim(const char* font, int *max_width, int *line_height, int *ascent, int *descent);

/** Returns the System default font.
 * \ingroup drvfont */
IUP_SDK_API char* iupdrvGetSystemFont(void);

/** FONT attribute set function.
 * \ingroup drvfont */
IUP_SDK_API int iupdrvSetFontAttrib(Ihandle* ih, const char* value);



/** Compensates IupMatrix limitation in Get FONT.
  * \ingroup drvfont */
IUP_SDK_API char* iupGetFontValue(Ihandle* ih);

/** Parse the font format description.
 * Returns a non zero value if successful.
 * \ingroup drvfont */
IUP_SDK_API int iupGetFontInfo(const char* font, char* typeface, int *size, int *is_bold, int *is_italic, int *is_underline, int *is_strikeout);

/** Parse the Pango font format description.
 * Returns a non zero value if successful.
 * \ingroup drvfont */
IUP_SDK_API int iupFontParsePango(const char *value, char* typeface, int *size, int *bold, int *italic, int *underline, int *strikeout);

/** Parse the old IUP Windows font format description.
 * Returns a non zero value if successful.
 * \ingroup drvfont */
IUP_SDK_API int iupFontParseWin(const char *value, char* typeface, int *size, int *bold, int *italic, int *underline, int *strikeout);

/** Parse the X-Windows font format description.
 * Returns a non zero value if successful.
 * \ingroup drvfont */
IUP_SDK_API int iupFontParseX(const char *value, char *typeface, int *size, int *bold, int *italic, int *underline, int *strikeout);


/** Changes the FONT style only.
 * \ingroup attribfunc */
IUP_SDK_API int iupSetFontStyleAttrib(Ihandle* ih, const char* value);

/** Changes the FONT size only.
 * \ingroup attribfunc */
IUP_SDK_API int iupSetFontSizeAttrib(Ihandle* ih, const char* value);

/** Changes the FONT face only.
* \ingroup attribfunc */
IUP_SDK_API int iupSetFontFaceAttrib(Ihandle* ih, const char* value);

/** Returns the FONT style.
 * \ingroup attribfunc */
IUP_SDK_API char* iupGetFontStyleAttrib(Ihandle* ih);

/** Returns the FONT size.
 * \ingroup attribfunc */
IUP_SDK_API char* iupGetFontSizeAttrib(Ihandle* ih);

/** Returns the FONT face.
 * \ingroup attribfunc */
IUP_SDK_API char* iupGetFontFaceAttrib(Ihandle* ih);

/* Used in Global Attributes */
void  iupSetDefaultFontSizeGlobalAttrib(const char* value);
char* iupGetDefaultFontSizeGlobalAttrib(void);
int   iupSetDefaultFontStyleGlobalAttrib(const char* value);
char* iupGetDefaultFontStyleGlobalAttrib(void);
int   iupSetDefaultFontFaceGlobalAttrib(const char* value);
char* iupGetDefaultFontFaceGlobalAttrib(void);


/* Updates the FONT attrib.
 * Called only from IupMap.
 */
void iupUpdateFontAttrib(Ihandle* ih);

/* Used to map foreign names into native names */
const char* iupFontGetWinName(const char* typeface);
const char* iupFontGetXName(const char* typeface);
const char* iupFontGetPangoName(const char* typeface);
const char* iupFontGetMacName(const char* typeface);



#ifdef __cplusplus
}
#endif

#endif
