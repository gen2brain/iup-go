/** \file
 * \brief Text Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_mask.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_key.h"
#include "iup_dialog.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_str.h"


/* Not defined for Cygwin and MingW */
#ifndef PFN_ARABIC
#define PFN_ARABIC		2
#define PFN_LCLETTER	3
#define	PFN_UCLETTER	4
#define	PFN_LCROMAN		5
#define	PFN_UCROMAN		6
#endif

#ifndef PFNS_PAREN
#define PFNS_PAREN		0x000
#define	PFNS_PARENS		0x100
#define PFNS_PERIOD		0x200
#define PFNS_PLAIN		0x300
#define PFNS_NONUMBER	0x400
#endif

#ifndef CFM_BACKCOLOR
#define CFM_BACKCOLOR		0x04000000
#define	CFM_UNDERLINETYPE	0x00800000
#define	CFM_WEIGHT			0x00400000
#define CFM_DISABLED		0x2000
#define	CFE_DISABLED		CFM_DISABLED
#endif

#ifndef CFU_UNDERLINEDOTTED
#define	CFU_UNDERLINEDOTTED				4
#define	CFU_UNDERLINEDOUBLE				3
#define CFU_UNDERLINE					1
#define CFU_UNDERLINENONE				0
#endif

#ifndef SES_UPPERCASE
#define SES_UPPERCASE			512
#define	SES_LOWERCASE			1024
#endif

#ifndef EM_SETCUEBANNER 
#define ECM_FIRST               0x1500      /* Edit control messages */
#define	EM_SETCUEBANNER	    (ECM_FIRST + 1)
#endif
/*   End Cygwin/MingW */


#define WM_IUPCARET WM_APP+1   /* Custom IUP message */


void iupdrvTextAddSpin(Ihandle* ih, int *w, int h)
{
  *w += h;
  (void)ih;  
}

void iupdrvTextAddBorders(Ihandle* ih, int *w, int *h)
{
  /* Used also by IupCalendar and IupDatePick in Windows */
  /* LAYOUT_DECORATION_ESTIMATE */
  int border_size = 2 * 3;
  (*w) += border_size;
  (*h) += border_size;
  (void)ih;  
}

static void winTextParseParagraphFormat(Ihandle* formattag, PARAFORMAT2 *paraformat, int convert2twips)
{
  int val;
  char* format;

  ZeroMemory(paraformat, sizeof(PARAFORMAT2));
  paraformat->cbSize = sizeof(PARAFORMAT2);

  format = iupAttribGet(formattag, "NUMBERING");
  if (format)
  {
    paraformat->dwMask |= PFM_NUMBERING;

    if (iupStrEqualNoCase(format, "BULLET"))
      paraformat->wNumbering = PFN_BULLET;
    else if (iupStrEqualNoCase(format, "ARABIC"))
      paraformat->wNumbering = PFN_ARABIC;
    else if (iupStrEqualNoCase(format, "LCLETTER"))
      paraformat->wNumbering = PFN_LCLETTER;
    else if (iupStrEqualNoCase(format, "UCLETTER"))
      paraformat->wNumbering = PFN_UCLETTER;
    else if (iupStrEqualNoCase(format, "LCROMAN"))
      paraformat->wNumbering = PFN_LCROMAN;
    else if (iupStrEqualNoCase(format, "UCROMAN"))
      paraformat->wNumbering = PFN_UCROMAN;
    else
      paraformat->wNumbering = 0;  /* "NONE" */

    format = iupAttribGet(formattag, "NUMBERINGSTYLE");
    if (format)
    {
      paraformat->dwMask |= PFM_NUMBERINGSTYLE;

      if (iupStrEqualNoCase(format, "RIGHTPARENTHESIS"))
        paraformat->wNumberingStyle = PFNS_PAREN;
      else if (iupStrEqualNoCase(format, "PARENTHESES"))
        paraformat->wNumberingStyle = PFNS_PARENS;
      else if (iupStrEqualNoCase(format, "PERIOD"))
        paraformat->wNumberingStyle = PFNS_PERIOD;
      else if (iupStrEqualNoCase(format, "NONUMBER"))
        paraformat->wNumberingStyle = PFNS_NONUMBER;
      else 
        paraformat->wNumberingStyle = PFNS_PLAIN;  /* "NONE" */
    }
   
    format = iupAttribGet(formattag, "NUMBERINGTAB");
    if (format && iupStrToInt(format, &val))
    {
      paraformat->dwMask |= PFM_NUMBERINGTAB;
      paraformat->wNumberingTab = (WORD)(val*convert2twips);
    }
  }

  format = iupAttribGet(formattag, "INDENT");
  if (format && iupStrToInt(format, &val))
  {
    paraformat->dwMask |= PFM_STARTINDENT|PFM_RIGHTINDENT|PFM_OFFSET;
    paraformat->dxStartIndent = val*convert2twips;

    format = iupAttribGet(formattag, "INDENTRIGHT");
    if (format && iupStrToInt(format, &val))
      paraformat->dxRightIndent = val*convert2twips;
    else
      paraformat->dxRightIndent = paraformat->dxStartIndent;
      
    format = iupAttribGet(formattag, "INDENTOFFSET");
    if (format && iupStrToInt(format, &val))
      paraformat->dxOffset = val*convert2twips;
    else
      paraformat->dxOffset = 0;
  }

  format = iupAttribGet(formattag, "ALIGNMENT");
  if (format)
  {
    paraformat->dwMask |= PFM_ALIGNMENT;

    if (iupStrEqualNoCase(format, "JUSTIFY"))
      paraformat->wAlignment = PFA_JUSTIFY;
    else if (iupStrEqualNoCase(format, "RIGHT"))
      paraformat->wAlignment = PFA_RIGHT;
    else if (iupStrEqualNoCase(format, "CENTER"))
      paraformat->wAlignment = PFA_CENTER;
    else
      paraformat->wAlignment = PFA_LEFT;  /* "LEFT" */
  }

  format = iupAttribGet(formattag, "TABSARRAY");
  if (format)
  {
    int pos, align, i = 0;
    LONG tab;
    char* str;

    paraformat->dwMask |= PFM_TABSTOPS;

    while (format)
    {
      str = iupStrDupUntil((const char**)&format, ' ');
      if (!str) break;
      pos = atoi(str)*convert2twips;
      free(str);

      str = iupStrDupUntil((const char**)&format, ' ');
      if (!str) break;

      if (iupStrEqualNoCase(str, "DECIMAL"))
        align = 3;
      else if (iupStrEqualNoCase(str, "RIGHT"))
        align = 2;
      else if (iupStrEqualNoCase(str, "CENTER"))
        align = 1;
      else /* "LEFT" */
        align = 0;
      free(str);

      tab = (pos&0xFFF)|(align<<24);
      paraformat->rgxTabs[i] = tab;
      i++;
      if (i == 32) break;
    }
    paraformat->cTabCount = (SHORT)i;
  }

  format = iupAttribGet(formattag, "SPACEBEFORE");
  if (format && iupStrToInt(format, &val))
  {
    paraformat->dwMask |= PFM_SPACEBEFORE;
    paraformat->dySpaceBefore = val*convert2twips;
  }

  format = iupAttribGet(formattag, "SPACEAFTER");
  if (format && iupStrToInt(format, &val))
  {
    paraformat->dwMask |= PFM_SPACEAFTER;
    paraformat->dySpaceAfter = val*convert2twips;
  }

  format = iupAttribGet(formattag, "LINESPACING");
  if (format)
  {
    paraformat->dwMask |= PFM_LINESPACING;

    if (iupStrEqualNoCase(format, "SINGLE"))
      paraformat->bLineSpacingRule = 0;
    else if (iupStrEqualNoCase(format, "ONEHALF"))
      paraformat->bLineSpacingRule = 1;
    else if (iupStrEqualNoCase(format, "DOUBLE"))
      paraformat->bLineSpacingRule = 2;
    else if (iupStrToInt(format, &val))
    {
      paraformat->bLineSpacingRule = 3;
      paraformat->dyLineSpacing = val*convert2twips;
    }
  }
}

static void winTextParseCharacterFormat(Ihandle* formattag, CHARFORMAT2 *charformat, int pixel2twips)
{
  int val;
  char* format;

  ZeroMemory(charformat, sizeof(CHARFORMAT2));
  charformat->cbSize = sizeof(CHARFORMAT2);

  format = iupAttribGet(formattag, "DISABLED");
  if (format)
  {
    charformat->dwMask |= CFM_DISABLED;
    if (iupStrBoolean(format))
      charformat->dwEffects |= CFE_DISABLED;
  }

  format = iupAttribGet(formattag, "RISE");
  if (format)
  {
    if (iupStrEqualNoCase(format, "SUPERSCRIPT"))
    {
      charformat->dwMask |= CFM_SUPERSCRIPT;
      charformat->dwEffects |= CFE_SUPERSCRIPT;
    }
    else if (iupStrEqualNoCase(format, "SUBSCRIPT"))
    {
      charformat->dwMask |= CFM_SUBSCRIPT;
      charformat->dwEffects |= CFE_SUBSCRIPT;
    } 
    else if (iupStrToInt(format, &val))
    {
      charformat->dwMask |= CFM_OFFSET;
      charformat->yOffset = val;
    }
  }

  format = iupAttribGet(formattag, "ITALIC");
  if (format)
  {
    charformat->dwMask |= CFM_ITALIC;
    if (iupStrBoolean(format))
      charformat->dwEffects |= CFE_ITALIC;
  }

  format = iupAttribGet(formattag, "STRIKEOUT");
  if (format)
  {
    charformat->dwMask |= CFM_STRIKEOUT;
    if (iupStrBoolean(format))
      charformat->dwEffects |= CFE_STRIKEOUT;
  }

  format = iupAttribGet(formattag, "PROTECTED");
  if (format)
  {
    charformat->dwMask |= CFM_PROTECTED;
    if (iupStrBoolean(format))
      charformat->dwEffects |= CFE_PROTECTED;
  }

  format = iupAttribGet(formattag, "FONTSIZE");
  if (format && iupStrToInt(format, &val))
  {
    /* (1/1440 of an inch, or 1/20 of a printer's point) */
    charformat->dwMask |= CFM_SIZE;
    if (val < 0)  /* in pixels */
      charformat->yHeight = (-val)*pixel2twips;
    else
      charformat->yHeight = val*20;
  }

  format = iupAttribGet(formattag, "FONTSCALE");
  if (format && charformat->yHeight != 0)
  {
    double fval = 0;
    if (iupStrEqualNoCase(format, "XX-SMALL"))
      fval = 0.5787037037037;
    else if (iupStrEqualNoCase(format, "X-SMALL"))
      fval = 0.6444444444444;
    else if (iupStrEqualNoCase(format, "SMALL"))
      fval = 0.8333333333333;
    else if (iupStrEqualNoCase(format, "MEDIUM"))
      fval = 1.0;
    else if (iupStrEqualNoCase(format, "LARGE"))
      fval = 1.2;
    else if (iupStrEqualNoCase(format, "X-LARGE"))
      fval = 1.4399999999999;
    else if (iupStrEqualNoCase(format, "XX-LARGE"))
      fval = 1.728;
    else 
      iupStrToDouble(format, &fval);

    if (fval > 0)
    {
      fval = charformat->yHeight * fval;
      charformat->yHeight = iupROUND(fval);
    }
  }

  format = iupAttribGet(formattag, "FONTFACE");
  if (format)
  {
    /* Map standard names to native names */
    const char* mapped_name = iupFontGetWinName(format);
    if (mapped_name)
      iupwinStrCopy(charformat->szFaceName, mapped_name, sizeof(charformat->szFaceName));
    else
      iupwinStrCopy(charformat->szFaceName, format, sizeof(charformat->szFaceName));
    charformat->dwMask |= CFM_FACE;
  }

  format = iupAttribGet(formattag, "FGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
    {
      charformat->dwMask |= CFM_COLOR;
      charformat->crTextColor = RGB(r, g, b);
    }
  }

  format = iupAttribGet(formattag, "BGCOLOR");
  if (format)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(format, &r, &g, &b))
    {
      charformat->dwMask |= CFM_BACKCOLOR;
      charformat->crBackColor = RGB(r, g, b);
    }
  }

  format = iupAttribGet(formattag, "UNDERLINE");
  if (format)
  {
    charformat->dwMask |= CFM_UNDERLINETYPE;

    if (iupStrEqualNoCase(format, "SINGLE"))
      charformat->bUnderlineType = CFU_UNDERLINE;
    else if (iupStrEqualNoCase(format, "DOUBLE"))
      charformat->bUnderlineType = CFU_UNDERLINEDOUBLE;
    else if (iupStrEqualNoCase(format, "DOTTED"))
      charformat->bUnderlineType = CFU_UNDERLINEDOTTED;
    else /* "NONE" */
      charformat->bUnderlineType = CFU_UNDERLINENONE;

    if (charformat->bUnderlineType != CFU_UNDERLINENONE)
    {
      charformat->dwMask |= CFM_UNDERLINE;
      charformat->dwEffects |= CFE_UNDERLINE;
    }
  }

  format = iupAttribGet(formattag, "WEIGHT");
  if (format)
  {
    charformat->dwMask |= CFM_WEIGHT;

    if (iupStrEqualNoCase(format, "EXTRALIGHT"))
      charformat->wWeight = FW_EXTRALIGHT;
    else if (iupStrEqualNoCase(format, "LIGHT"))
      charformat->wWeight = FW_LIGHT;
    else if (iupStrEqualNoCase(format, "SEMIBOLD"))
      charformat->wWeight = FW_SEMIBOLD;
    else if (iupStrEqualNoCase(format, "BOLD"))
      charformat->wWeight = FW_BOLD;
    else if (iupStrEqualNoCase(format, "EXTRABOLD"))
      charformat->wWeight = FW_EXTRABOLD;
    else if (iupStrEqualNoCase(format, "HEAVY"))
      charformat->wWeight = FW_HEAVY;
    else /* "NORMAL" */
      charformat->wWeight = FW_NORMAL;

    if (charformat->wWeight != FW_NORMAL)
    {
      charformat->dwMask |= CFM_BOLD;
      charformat->dwEffects |= CFE_BOLD;
    }
  }
}                      

static void winTextUpdateFontFormat(CHARFORMAT2* charformat, const char* value)
{
  int size = 0;
  int is_bold = 0,
    is_italic = 0, 
    is_underline = 0,
    is_strikeout = 0;
  char typeface[1024];
  const char* mapped_name;

  if (!iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout))
    return;

  /* Map standard names to native names */
  mapped_name = iupFontGetWinName(typeface);
  if (mapped_name)
    strcpy(typeface, mapped_name);

  charformat->dwMask |= CFM_FACE;
  iupwinStrCopy(charformat->szFaceName, typeface, sizeof(charformat->szFaceName));

  /* (1/1440 of an inch, or 1/20 of a printer's point) */
  charformat->dwMask |= CFM_SIZE;
  if (size < 0)  /* in pixels */
  {
    int pixel2twips = 1440/iupwinGetScreenRes();
    charformat->yHeight = (-size)*pixel2twips;
  }
  else
    charformat->yHeight = size*20;

  charformat->dwMask |= CFM_WEIGHT|CFM_BOLD;
  if (is_bold)
  {
    charformat->wWeight = FW_BOLD;
    charformat->dwEffects |= CFE_BOLD;
  }
  else
    charformat->wWeight = FW_NORMAL;

  charformat->dwMask |= CFM_ITALIC;
  if (is_italic)
    charformat->dwEffects |= CFE_ITALIC;

  charformat->dwMask |= CFM_UNDERLINETYPE;
  if (is_underline)
  {
    charformat->bUnderlineType = CFU_UNDERLINE;
    charformat->dwMask |= CFM_UNDERLINE;
    charformat->dwEffects |= CFE_UNDERLINE;
  }

  charformat->dwMask |= CFM_STRIKEOUT;
  if (is_strikeout)
    charformat->dwEffects |= CFE_STRIKEOUT;
}

static int winTextSetLinColToPosition(Ihandle *ih, int lin, int col)
{
  int linmax, colmax, lineindex, wpos;

  lin--; /* IUP starts at 1 */
  col--;
    
  linmax = (int)SendMessage(ih->handle, EM_GETLINECOUNT, 0, 0L);
  if (lin > linmax-1)
    lin = linmax-1;

  lineindex = (int)SendMessage(ih->handle, EM_LINEINDEX, (WPARAM)lin, 0L);

  colmax = (int)SendMessage(ih->handle, EM_LINELENGTH, (WPARAM)lineindex, 0L);
  if (col > colmax)
    col = colmax;    /* after the last character */

  /* pos here includes the line breaks in 1 or 2 configuration */
  wpos = lineindex + col;
  return wpos;
}

static int winTextGetLastPosition(Ihandle *ih)
{
  int lincount = (int)SendMessage(ih->handle, EM_GETLINECOUNT, 0, 0L);
  int lineindex = (int)SendMessage(ih->handle, EM_LINEINDEX, (WPARAM)(lincount - 1), 0L);
  int colmax = (int)SendMessage(ih->handle, EM_LINELENGTH, (WPARAM)lineindex, 0L);
  int wpos;

  /* when formatting or single line text uses only one char per line end */
  if (ih->data->is_multiline && !ih->data->has_formatting)
    lineindex -= lincount - 1;  /* remove \r characters from count */

  /* pos here includes the line breaks in 1 or 2 configuration */
  wpos = lineindex + colmax;
  return wpos;
}

static void winTextGetLinColFromPosition(Ihandle* ih, int wpos, int* lin, int* col)
{
  /* here "pos" must contains the extra chars if the case */
  /* pos here includes the line breaks in 1 or 2 configuration */
  int lineindex;

  if (ih->data->has_formatting)
    *lin = (int)SendMessage(ih->handle, EM_EXLINEFROMCHAR, (WPARAM)0, (LPARAM)wpos);
  else
    *lin = (int)SendMessage(ih->handle, EM_LINEFROMCHAR, (WPARAM)wpos, (LPARAM)0L);

  lineindex = (int)SendMessage(ih->handle, EM_LINEINDEX, (WPARAM)(*lin), (LPARAM)0L);
  *col = wpos - lineindex;  /* lineindex is at the first character of the line */

  (*lin)++; /* IUP starts at 1 */
  (*col)++;
}

static int winTextRemoveExtraChars(Ihandle* ih, int wpos)
{
  /* called only if not single line and not formatting */
  int lin = (int)SendMessage(ih->handle, EM_LINEFROMCHAR, (WPARAM)wpos, (LPARAM)0L);
  int pos = wpos - lin;  /* remove \r characters from count */
  return pos;
}

static int winTextAddExtraChars(Ihandle* ih, int pos)
{
  /* called only if not single line and not formatting */
  int lin, clin, wpos;

  clin = (int)SendMessage(ih->handle, EM_LINEFROMCHAR, (WPARAM)pos, (LPARAM)0L);

  /* pos is smaller than the actual pos (missing the \r count),
     so we must calculate the line until the returned value is the same as the expected. */ 
  do
  {
    lin = clin;
    clin = (int)SendMessage(ih->handle, EM_LINEFROMCHAR, (WPARAM)(pos + lin + 1), (LPARAM)0L);   /* add one because we can be at the last character */
  } while (clin != lin);                                                                /* and it will not change to the next line by 1 */

  wpos = pos + lin;  /* add \r characters from count */
  return wpos;
}

static int winTextGetCaretPosition(Ihandle* ih)
{
  int wpos = 0;
  POINT point;

  if (GetFocus() != ih->handle || !GetCaretPos(&point))
  {
    /* if does not have the focus, or could not get caret position,
       then use the selection start position */
    SendMessage(ih->handle, EM_GETSEL, (WPARAM)&wpos, 0);
  }
  else
  {
    if (ih->data->has_formatting)
    {
      wpos = (int)SendMessage(ih->handle, EM_CHARFROMPOS, 0, (LPARAM)&point);
    }
    else if(point.x < 0 && point.y < 0)
    {
    /* if the caret position is located outside the visible client area,
       then use the selection start position */
      SendMessage(ih->handle, EM_GETSEL, (WPARAM)&wpos, 0);
    }
    else
    {
      LRESULT ret;

      /* Workaround for weird behavior because of the return value in GetCaretPos */
      if (ih->data->is_multiline && point.y < 5)
        point.y += 5;

      ret = SendMessage(ih->handle, EM_CHARFROMPOS, 0, MAKELPARAM(point.x, point.y));
      wpos = LOWORD(ret);
    }
  }

  /* pos here includes the line breaks in 1 or 2 configuration */
  return wpos;
}

static int winTextGetCaret(Ihandle* ih, int *lin, int *col)
{
  int pos;
  int wpos = winTextGetCaretPosition(ih);

  if (ih->data->is_multiline)
  {
    winTextGetLinColFromPosition(ih, wpos, lin, col);

    if (!ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
      pos = winTextRemoveExtraChars(ih, wpos);
    else
      pos = wpos;
  }
  else
  {
    pos = wpos;
    *col = wpos;
    (*col)++;  /* IUP starts at 1 */
    *lin = 1;
  }

  return pos;
}

static int winTextGetSelection(Ihandle* ih, int *start, int *end)
{
  *start = 0;
  *end = 0;

  SendMessage(ih->handle, EM_GETSEL, (WPARAM)start, (LPARAM)end);

  if (ih->data->is_multiline && !ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
  {
    if (*start == *end)
      (*end) = (*start) = winTextRemoveExtraChars(ih, *start);
    else
    {
      (*start) = winTextRemoveExtraChars(ih, *start);
      (*end) = winTextRemoveExtraChars(ih, *end);
    }
  }

  if (*start == *end)
    return 0;
  return 1;
}

static void winTextSetSelection(Ihandle* ih, int start, int end)
{
  if (ih->data->is_multiline && !ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
  {
    if (start == end)
      end = start = winTextAddExtraChars(ih, start);
    else
    {
      start = winTextAddExtraChars(ih, start);
      end = winTextAddExtraChars(ih, end);
    }
  }

  SendMessage(ih->handle, EM_SETSEL, (WPARAM)start, (LPARAM)end);
}

void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos)
{
  int wpos = winTextSetLinColToPosition(ih, lin, col);

  if (!ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
    *pos = winTextRemoveExtraChars(ih, wpos);
  else
    *pos = wpos;
}

void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col)
{
  int wpos;

  if (!ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
    wpos = winTextAddExtraChars(ih, pos);
  else
    wpos = pos;

  winTextGetLinColFromPosition(ih, wpos, lin, col);
}

static int winTextConvertXYToPos(Ihandle* ih, int x, int y)
{
  int wpos, pos;

  if (ih->data->has_formatting)
  {
    POINT point;
    point.x = x;
    point.y = y;
    wpos = (int)SendMessage(ih->handle, EM_CHARFROMPOS, 0, (LPARAM)&point);
  }
  else
  {
    LRESULT ret = SendMessage(ih->handle, EM_CHARFROMPOS, 0, MAKELPARAM(x, y));
    wpos = LOWORD(ret);
  }

  if (ih->data->is_multiline)
  {
    if (!ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
      pos = winTextRemoveExtraChars(ih, wpos);
    else
      pos = wpos;
  }
  else
    pos = wpos;

  return pos;
}

static TCHAR* winTextStrConvertToSystem(Ihandle* ih, const char* str)
{
  if (ih->data->is_multiline)
  {
    if (ih->data->has_formatting)
    {
      if (strchr(str, '\n') != NULL)
      {
        str = iupStrReturnStr(str);
        iupStrToMac((char*)str);  /* replace inline */
      }
    }
    else
    {
      TCHAR* wstr;
      char* dos_str = iupStrToDos(str);
#ifdef UNICODE
      wstr = iupwinStrToSystem(dos_str);
#else
      if (dos_str != str) 
        wstr = iupStrReturnStr(dos_str);
      else
        wstr = dos_str;
#endif
      if (dos_str != str) free(dos_str);
      return wstr;
    }
  }

  return iupwinStrToSystem(str);
}

static DWORD CALLBACK winTextWriteStreamCallback(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
  HANDLE hFile = (HANDLE)dwCookie;

  if (WriteFile(hFile, lpBuff, cb, (DWORD *)pcb, NULL))
    return 0;

  return (DWORD)-1;
}

static BOOL winTextWriteRtfToFile(HWND hwnd, TCHAR* pszFileName)
{
  BOOL fSuccess = FALSE;

  HANDLE hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    EDITSTREAM es = { 0 };

    es.pfnCallback = winTextWriteStreamCallback;
    es.dwCookie = (DWORD_PTR)hFile;

    if (SendMessage(hwnd, EM_STREAMOUT, SF_RTF, (LPARAM)&es) && es.dwError == 0)
      fSuccess = TRUE;

    CloseHandle(hFile);
  }

  return fSuccess;

}

static DWORD CALLBACK winTextReadStreamCallback(DWORD_PTR dwCookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
  HANDLE hFile = (HANDLE)dwCookie;

  if (ReadFile(hFile, lpBuff, cb, (DWORD *)pcb, NULL))
    return 0;

  return (DWORD)-1;
}

static BOOL winTextReadRtfFromFile(HWND hwnd, TCHAR* pszFileName)
{
  BOOL fSuccess = FALSE;

  HANDLE hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    EDITSTREAM es = { 0 };

    es.pfnCallback = winTextReadStreamCallback;
    es.dwCookie = (DWORD_PTR)hFile;

    if (SendMessage(hwnd, EM_STREAMIN, SF_RTF, (LPARAM)&es) && es.dwError == 0)
      fSuccess = TRUE;

    CloseHandle(hFile);
  }

  return fSuccess;

}


/***********************************************************************************************/


static int winTextSetValueAttrib(Ihandle* ih, const char* value)
{
  TCHAR* str;
  if (!value) value = "";
  str = winTextStrConvertToSystem(ih, value);
  ih->data->disable_callbacks = 1;
  SetWindowText(ih->handle, str);
  ih->data->disable_callbacks = 0;
  return 0;
}

static char* winTextGetValueAttrib(Ihandle* ih)
{
  char* str = iupStrReturnStr(iupwinStrFromSystem(iupwinGetWindowText(ih->handle)));  
  if (str)
  {
    /* notice that GetWindowText always returns in DOS format */
    if (ih->data->is_multiline)
      iupStrToUnix(str);
    return str;
  }
  else
    return "";
}

static int winTextSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &(ih->data->horiz_padding), &(ih->data->vert_padding), 'x');
  ih->data->vert_padding = 0;
  if (ih->handle)
  {
    SendMessage(ih->handle, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELPARAM(ih->data->horiz_padding, ih->data->horiz_padding));
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int winTextHasSelection(Ihandle* ih)
{
  int start = 0, end = 0;
  SendMessage(ih->handle, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  if (start == end)
    return 0;
  return 1;
}

static int winTextSetSelectedTextAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    TCHAR* str;
    
    if (!winTextHasSelection(ih))
      return 0;

    str = winTextStrConvertToSystem(ih, value);

    ih->data->disable_callbacks = 1;
    SendMessage(ih->handle, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)str);
    ih->data->disable_callbacks = 0;
  }
  return 0;
}

static char* winTextGetSelectedTextAttrib(Ihandle* ih)
{
  int start = 0, end = 0;
  TCHAR* tstr;
    
  SendMessage(ih->handle, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  if (start == end)
    return NULL;

  if (ih->data->has_formatting)
  {
    tstr = (TCHAR*)iupStrGetMemory((end-start+1)*sizeof(TCHAR));
    SendMessage(ih->handle, EM_GETSELTEXT, 0, (LPARAM)tstr);
  }
  else
  {
    tstr = iupwinGetWindowText(ih->handle);  
    if (!tstr)
      return NULL;

    /* returns only the selected text */
    tstr[end] = 0;
    tstr += start;
  }

  {
    char* str = iupStrReturnStr(iupwinStrFromSystem(tstr));

    /* notice that GetWindowText always returns in DOS format */
    if (ih->data->is_multiline)
      iupStrToUnix(str);
    return str;
  }
}

static int winTextSetNCAttrib(Ihandle* ih, const char* value)
{
  if (!iupStrToInt(value, &ih->data->nc))
    ih->data->nc = 0;

  if (ih->handle)
  {
    if (ih->data->has_formatting)
      SendMessage(ih->handle, EM_EXLIMITTEXT, 0, ih->data->nc);   /* so it can be larger than 64k */
    else
      SendMessage(ih->handle, EM_LIMITTEXT, ih->data->nc, 0L);

    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static char* winTextGetCountAttrib(Ihandle* ih)
{
  int count = winTextGetLastPosition(ih);
  return iupStrReturnInt(count);
}

static char* winTextGetLineCountAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int linecount = (int)SendMessage(ih->handle, EM_GETLINECOUNT, 0, 0L);
    return iupStrReturnInt(linecount);
  }
  else
    return "1";
}

static char* winTextGetLineValueAttrib(Ihandle* ih)
{
  if (ih->data->is_multiline)
  {
    int len, lin, col;
    TCHAR* str = (TCHAR*)iupStrGetMemory(256*sizeof(TCHAR));
    WORD* wstr = (WORD*)str;
    *wstr = 256;
    winTextGetCaret(ih, &lin, &col); 
    lin--; /* from IUP to Win */
    len = (int)SendMessage(ih->handle, EM_GETLINE, (WPARAM)lin, (LPARAM)str);
    str[len]=0;
    return iupStrReturnStr(iupwinStrFromSystem(str));
  }
  else
    return winTextGetValueAttrib(ih);
}

static int winTextSetSelectionAttrib(Ihandle* ih, const char* value)
{
  int start=1, end=1;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    SendMessage(ih->handle, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    SendMessage(ih->handle, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    return 0;
  }

  if (ih->data->is_multiline)
  {
    int lin_start=1, col_start=1, lin_end=1, col_end=1;

    if (sscanf(value, "%d,%d:%d,%d", &lin_start, &col_start, &lin_end, &col_end)!=4) return 0;
    if (lin_start<1 || col_start<1 || lin_end<1 || col_end<1) return 0;

    start = winTextSetLinColToPosition(ih, lin_start, col_start);
    end = winTextSetLinColToPosition(ih, lin_end, col_end);
  }
  else
  {
    if (iupStrToIntInt(value, &start, &end, ':')!=2) 
      return 0;

    if(start<1 || end<1) 
      return 0;

    start--; /* IUP starts at 1 */
    end--;
  }

  SendMessage(ih->handle, EM_SETSEL, (WPARAM)start, (LPARAM)end);

  return 0;
}

static char* winTextGetSelectionAttrib(Ihandle* ih)
{
  int start = 0, end = 0;

  SendMessage(ih->handle, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
  if (start == end)
    return NULL;

  if (ih->data->is_multiline)
  {
    int start_col, start_lin, end_col, end_lin;
    winTextGetLinColFromPosition(ih, start, &start_lin, &start_col);
    winTextGetLinColFromPosition(ih, end,   &end_lin,   &end_col);
    return iupStrReturnStrf("%d,%d:%d,%d", start_lin, start_col, end_lin, end_col);
  }
  else
  {
    start++; /* IUP starts at 1 */
    end++;
    return iupStrReturnIntInt(start, end, ':');
  }
}

static int winTextSetSelectionPosAttrib(Ihandle* ih, const char* value)
{
  int start=0, end=0;

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    SendMessage(ih->handle, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
    return 0;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    SendMessage(ih->handle, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
    return 0;
  }

  if (iupStrToIntInt(value, &start, &end, ':')!=2) 
    return 0;

  if(start<0 || end<0) 
    return 0;

  winTextSetSelection(ih, start, end);
  return 0;
}

static char* winTextGetSelectionPosAttrib(Ihandle* ih)
{
  int start = 0, end = 0;
  if (!winTextGetSelection(ih, &start, &end))
    return NULL;
  return iupStrReturnIntInt(start, end, ':');
}

static int winTextSetInsertAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle)  /* do not do the action before map */
    return 0;
  if (value)
  {
    ih->data->disable_callbacks = 1;
    SendMessage(ih->handle, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)winTextStrConvertToSystem(ih, value));
    ih->data->disable_callbacks = 0;
  }
  return 0;
}

static int winTextSetAppendAttrib(Ihandle* ih, const char* value)
{
  int wpos;
  TCHAR* str;

  if (!ih->handle)  /* do not do the action before map */
    return 0;

  if (!value) value = "";
  str = winTextStrConvertToSystem(ih, value);
  
  wpos = GetWindowTextLength(ih->handle)+1;
  SendMessage(ih->handle, EM_SETSEL, (WPARAM)wpos, (LPARAM)wpos);

  ih->data->disable_callbacks = 1;

  if (ih->data->is_multiline && ih->data->append_newline && wpos != 1)
  {
    if (ih->data->has_formatting)
      SendMessage(ih->handle, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)TEXT("\r"));
    else
      SendMessage(ih->handle, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)TEXT("\r\n"));
  }
  SendMessage(ih->handle, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)str);

  ih->data->disable_callbacks = 0;

  return 0;
}

static int winTextSetReadOnlyAttrib(Ihandle* ih, const char* value)
{
  SendMessage(ih->handle, EM_SETREADONLY, (WPARAM)iupStrBoolean(value), 0);
  return 0;
}

static char* winTextGetReadOnlyAttrib(Ihandle* ih)
{
  DWORD style = GetWindowLong(ih->handle, GWL_STYLE);
  return iupStrReturnBoolean (style & ES_READONLY); 
}

static int winTextSetTabSizeAttrib(Ihandle* ih, const char* value)
{
  int tabsize;
  if (!ih->data->is_multiline)
    return 0;

  iupStrToInt(value, &tabsize);
  tabsize *= 4;
  SendMessage(ih->handle, EM_SETTABSTOPS, (WPARAM)1L, (LPARAM)&tabsize);
  iupdrvRedrawNow(ih);
  return 1;
}

static int winTextSetCaretAttrib(Ihandle* ih, const char* value)
{
  int wpos;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    int lin = 1, col = 1;
    iupStrToIntInt(value, &lin, &col, ',');  /* be permissive in SetCaret, do not abort if invalid */
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;

    wpos = winTextSetLinColToPosition(ih, lin, col);
  }
  else
  {
    int pos = 1;
    iupStrToInt(value, &pos);
    if (pos < 1) pos = 1;
    pos--; /* IUP starts at 1 */
    wpos = pos;
  }

  SendMessage(ih->handle, EM_SETSEL, (WPARAM)wpos, (LPARAM)wpos);
  SendMessage(ih->handle, EM_SCROLLCARET, 0L, 0L);

  return 0;
}

static char* winTextGetCaretAttrib(Ihandle* ih)
{
  int col, lin;
  winTextGetCaret(ih, &lin, &col);

  if (ih->data->is_multiline)
    return iupStrReturnIntInt(lin, col, ',');
  else
    return iupStrReturnInt(col);
}

static int winTextSetCaretPosAttrib(Ihandle* ih, const char* value)
{
  int pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);    /* be permissive in SetCaret, do not abort if invalid */
  if (pos < 0) pos = 0;

  winTextSetSelection(ih, pos, pos);
  SendMessage(ih->handle, EM_SCROLLCARET, 0L, 0L);
  return 0;
}

static char* winTextGetCaretPosAttrib(Ihandle* ih)
{
  int pos;
  int wpos = winTextGetCaretPosition(ih);
  if (ih->data->is_multiline && !ih->data->has_formatting)  /* when formatting or single line text uses only one char per line end */
    pos = winTextRemoveExtraChars(ih, wpos);
  else
    pos = wpos;
  return iupStrReturnInt(pos);
}

static void winTextScrollTo(Ihandle* ih, int lin, int col)
{
  DWORD old_lin = (DWORD)SendMessage(ih->handle, EM_GETFIRSTVISIBLELINE, 0, 0);
  if (ih->data->has_formatting)
  {
    SendMessage(ih->handle, EM_LINESCROLL, 0, (LPARAM)(lin - old_lin - 1));
    SendMessage(ih->handle, EM_SCROLL, (WPARAM)SB_LINEDOWN, 0);  /* to force an update of the scrollbars */
  }
  else  /* How to get the current horizontal position?????  */
    SendMessage(ih->handle, EM_LINESCROLL, (WPARAM)col, (LPARAM)(lin-old_lin));
}

static int winTextSetScrollToAttrib(Ihandle* ih, const char* value)
{
  int lin = 1, col = 1;

  if (!value)
    return 0;

  if (ih->data->is_multiline)
  {
    iupStrToIntInt(value, &lin, &col, ',');
    if (lin < 1) lin = 1;
    if (col < 1) col = 1;
  }
  else
  {
    iupStrToInt(value, &col);
    if (col < 1) col = 1;
  }

  lin--;  /* return to Windows referece */
  col--;

  winTextScrollTo(ih, lin, col);
  return 0;
}

static int winTextSetScrollToPosAttrib(Ihandle* ih, const char* value)
{
  int lin, col, pos = 0;

  if (!value)
    return 0;

  iupStrToInt(value, &pos);
  if (pos < 0) pos = 0;

  iupdrvTextConvertPosToLinCol(ih, pos, &lin, &col);

  lin--;  /* return to Windows referece */
  col--;

  winTextScrollTo(ih, lin, col);

  return 0;
}

static int winTextSetFilterAttrib(Ihandle *ih, const char *value)
{
  int style = 0;

  if (iupStrEqualNoCase(value, "LOWERCASE"))
  {
    if (ih->data->has_formatting)
    {
      SendMessage(ih->handle, EM_SETEDITSTYLE, SES_LOWERCASE, SES_LOWERCASE);
      return 1;
    }
    style = ES_LOWERCASE;
  }
  else if (iupStrEqualNoCase(value, "NUMBER"))
    style = ES_NUMBER;
  else if (iupStrEqualNoCase(value, "UPPERCASE"))
  {
    if (ih->data->has_formatting)
    {
      SendMessage(ih->handle, EM_SETEDITSTYLE, SES_UPPERCASE, SES_UPPERCASE);
      return 1;
    }
    style = ES_UPPERCASE;
  }

  if (style)
    iupwinMergeStyle(ih, ES_LOWERCASE|ES_NUMBER|ES_UPPERCASE, style);

  return 1;
}

static int winTextSetClipboardAttrib(Ihandle *ih, const char *value)
{
  UINT msg = 0;

  if (iupStrEqualNoCase(value, "COPY"))
    msg = WM_COPY;
  else if (iupStrEqualNoCase(value, "CUT"))
    msg = WM_CUT;
  else if (iupStrEqualNoCase(value, "PASTE"))
    msg = WM_PASTE;
  else if (iupStrEqualNoCase(value, "CLEAR"))
    msg = WM_CLEAR;
  else if (iupStrEqualNoCase(value, "UNDO"))
    msg = WM_UNDO;
  else if (ih->data->has_formatting && iupStrEqualNoCase(value, "REDO"))
    msg = EM_REDO;

  if (msg)
    SendMessage(ih->handle, msg, 0, 0);

  return 0;
}

static int winTextSetFgColorAttrib(Ihandle *ih, const char *value)
{
  (void)value;
  iupdrvPostRedraw(ih);
  return 1;
}

static int winTextSetBgColorAttrib(Ihandle *ih, const char *value)
{
  if (ih->data->has_formatting)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      COLORREF color;
      color = RGB(r,g,b);
      SendMessage(ih->handle, EM_SETBKGNDCOLOR, 0, (LPARAM)color);
    }
  }
  iupdrvPostRedraw(ih);
  return 1;
}

static int winTextSetCueBannerAttrib(Ihandle *ih, const char *value)
{
  if (!ih->data->is_multiline && iupwin_comctl32ver6)
  {
    WCHAR* wstr = iupwinStrChar2Wide(value);
    SendMessage(ih->handle, EM_SETCUEBANNER, (WPARAM)FALSE, (LPARAM)wstr);  /* always an Unicode string here */
    free(wstr);
    return 1;
  }
  return 0;
}

static int winTextSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  int new_style;

  if (iupStrEqualNoCase(value, "ARIGHT"))
    new_style = ES_RIGHT;
  else if (iupStrEqualNoCase(value, "ACENTER"))
    new_style = ES_CENTER;
  else /* "ALEFT" */
    new_style = ES_LEFT;

  iupwinMergeStyle(ih, ES_LEFT|ES_CENTER|ES_RIGHT, new_style);

  return 1;
}

static int winTextSetFontAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->has_formatting && ih->handle)
  {
    if (!iupAttribGet(ih, "_IUPWIN_FONTUPDATECHECK"))
    {
      /* avoid setting the same font because this will clear the font formattags. */
      char* cur_value = iupAttribGetStr(ih, "FONT");
      if (iupStrEqual(value, cur_value))
        return 0;
    }
    iupAttribSet(ih, "_IUPWIN_FONTUPDATECHECK", NULL);
  }

  return iupdrvSetFontAttrib(ih, value);
}

typedef struct
{
  int eventMask;
  DWORD line;
  CHARRANGE oldRange;
} formatTagBulkState;

void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  formatTagBulkState* state = (formatTagBulkState*) malloc(sizeof(formatTagBulkState));

  state->line = (int)SendMessage(ih->handle, EM_GETFIRSTVISIBLELINE, 0, 0);  /* save scrollbar */
  SendMessage(ih->handle, EM_EXGETSEL, 0, (LPARAM)&state->oldRange);  /* save selection */
  state->eventMask = (int)SendMessage(ih->handle, EM_SETEVENTMASK, 0, 0);  /* disable events */
  SendMessage(ih->handle, WM_SETREDRAW, FALSE, 0);  /* disable redraw */

  return state;
}

void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* stateOpaque)
{
  formatTagBulkState* state = (formatTagBulkState*) stateOpaque;
  DWORD line = (DWORD)SendMessage(ih->handle, EM_GETFIRSTVISIBLELINE, 0, 0);

  SendMessage(ih->handle, EM_EXSETSEL, 0, (LPARAM)&state->oldRange);
  SendMessage(ih->handle, EM_LINESCROLL, 0, state->line - line);
  SendMessage(ih->handle, EM_SETEVENTMASK, 0, state->eventMask);
  SendMessage(ih->handle, WM_SETREDRAW, TRUE, 0);

  free(state);

  iupdrvRedrawNow(ih);
}

void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  int convert2twips, pixel2twips;
  char *selection, *units;
  PARAFORMAT2 paraformat;
  CHARFORMAT2 charformat;
  formatTagBulkState* state = NULL;

  /* one twip is 1/1440 inch */
  /* twip = (pixel*1440)/(pixel/inch) */
  pixel2twips = 1440/iupwinGetScreenRes();

  /* default is PIXELS */
  convert2twips = pixel2twips;
  units = iupAttribGet(formattag, "UNITS");
  if (units)
  {
    int val;
    if (iupStrEqualNoCase(units, "TWIPS"))
      convert2twips = 1;
    else if (iupStrToInt(units, &val))
      convert2twips = val;
  }

  /* save state here if not applying a bulk */
  if (!bulk) 
    state = (formatTagBulkState*)iupdrvTextAddFormatTagStartBulk(ih);

  selection = iupAttribGet(formattag, "SELECTION");
  if (selection)
  {
    /* In Windows, the format message use the current selection */
    winTextSetSelectionAttrib(ih, selection);
  }
  else
  {
    char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
    if (selectionpos)
    {
      /* In Windows, the format message use the current selection */
      winTextSetSelectionPosAttrib(ih, selectionpos);
    }
  }

  if (iupAttribGet(formattag, "FONTSCALE") && !iupAttribGet(formattag, "FONTSIZE"))
    iupAttribSet(formattag, "FONTSIZE", iupGetFontSizeAttrib(ih));

  winTextParseParagraphFormat(formattag, &paraformat, convert2twips);
  if (paraformat.dwMask != 0)
    SendMessage(ih->handle, EM_SETPARAFORMAT, 0, (LPARAM)&paraformat);

  winTextParseCharacterFormat(formattag, &charformat, pixel2twips);
  if (charformat.dwMask != 0)
    SendMessage(ih->handle, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charformat);

  /* restore state here if not applying a bulk */
  if (!bulk) 
    iupdrvTextAddFormatTagStopBulk(ih, state);
}

static int winTextSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  PARAFORMAT2 paraformat;
  CHARFORMAT2 charformat;
  COLORREF colorref;

  if (!ih->data->has_formatting)
    return 0;

  ZeroMemory(&paraformat, sizeof(PARAFORMAT2));
  paraformat.cbSize = sizeof(PARAFORMAT2);
  paraformat.dwMask = PFM_NUMBERING|PFM_STARTINDENT|PFM_RIGHTINDENT|PFM_OFFSET|
                      PFM_ALIGNMENT|PFM_SPACEBEFORE|PFM_SPACEAFTER|PFM_LINESPACING;
  paraformat.wAlignment = PFA_LEFT;

  ZeroMemory(&charformat, sizeof(CHARFORMAT2));
  charformat.cbSize = sizeof(CHARFORMAT2);
  charformat.dwMask = CFM_DISABLED|CFM_OFFSET|CFM_PROTECTED;

  winTextUpdateFontFormat(&charformat, IupGetAttribute(ih, "FONT"));

  if (iupwinGetColorRef(ih, "FGCOLOR", &colorref))
  {
    charformat.dwMask |= CFM_COLOR;
    charformat.crTextColor = colorref;
  }

  if (iupwinGetColorRef(ih, "BGCOLOR", &colorref))
  {
    charformat.dwMask |= CFM_BACKCOLOR;
    charformat.crBackColor = colorref;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    CHARRANGE oldRange;
    SendMessage(ih->handle, EM_EXGETSEL, 0, (LPARAM)&oldRange);
    SendMessage(ih->handle, EM_SETSEL, (WPARAM)-1, (LPARAM)0);
    SendMessage(ih->handle, EM_SETPARAFORMAT, 0, (LPARAM)&paraformat);  /* always for the current selection */
    SendMessage(ih->handle, EM_EXSETSEL, 0, (LPARAM)&oldRange);

    SendMessage(ih->handle, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&charformat);
  }
  else
  {
    SendMessage(ih->handle, EM_SETPARAFORMAT, 0, (LPARAM)&paraformat);
    SendMessage(ih->handle, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charformat);
  }

  return 0;
}

static int winTextSetLoadRtfAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    if (winTextReadRtfFromFile(ih->handle, iupwinStrToSystemFilename(value)))
      iupAttribSet(ih, "LOADRTFSTATUS", "OK");
    else
      iupAttribSet(ih, "LOADRTFSTATUS", "FAILED");
  }
  return 0;
}

static int winTextSetSaveRtfAttrib(Ihandle* ih, const char* value)
{
  if (value)
  {
    if (winTextWriteRtfToFile(ih->handle, iupwinStrToSystemFilename(value)))
      iupAttribSet(ih, "SAVERTFSTATUS", "OK");
    else
      iupAttribSet(ih, "SAVERTFSTATUS", "FAILED");
  }
  return 0;
}

static int winTextSetOverwriteAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->has_formatting)
    return 0;

  if (iupAttribGetBoolean(ih, "OVERWRITE"))
  {
    if (!iupStrBoolean(value))
      SendMessage(ih->handle, WM_KEYDOWN, VK_INSERT, 0);  /* toggle from ON to OFF */
  }
  else
  {
    if (iupStrBoolean(value))
      SendMessage(ih->handle, WM_KEYDOWN, VK_INSERT, 0);  /* toggle from OFF to ON */
  }
  return 1;
}

static char* winTextGetOverwriteAttrib(Ihandle* ih)
{
  if (!ih->data->has_formatting)
    return NULL;

  return iupStrReturnChecked(iupAttribGetBoolean(ih, "OVERWRITE"));
}

static int winTextSetVisibleAttrib(Ihandle* ih, const char* value)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
    ShowWindow(hSpin, iupStrBoolean(value)? SW_SHOWNORMAL: SW_HIDE);

  return iupBaseSetVisibleAttrib(ih, value);
}

static int winTextSetActiveAttrib(Ihandle* ih, const char* value)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
    EnableWindow(hSpin, iupStrBoolean(value));

  return iupBaseSetActiveAttrib(ih, value);
}

static void winTextCropSpinValue(Ihandle* ih, HWND hSpin, int min, int max)
{
  /* refresh if internally cropped, but text still shows an invalid value */
  int pos = (int)SendMessage(hSpin, UDM_GETPOS32, 0, 0);
  ih->data->disable_callbacks = 1;
  if (pos <= min)  
    SendMessage(hSpin, UDM_SETPOS32, 0, min);
  if (pos >= max)
    SendMessage(hSpin, UDM_SETPOS32, 0, max);
  ih->data->disable_callbacks = 0;
}

static int winTextSetSpinMinAttrib(Ihandle* ih, const char* value)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    int min;
    if (iupStrToInt(value, &min))
    {
      int max = iupAttribGetInt(ih, "SPINMAX");
      SendMessage(hSpin, UDM_SETRANGE32, min, max);

      winTextCropSpinValue(ih, hSpin, min, max);
    }
  }
  return 1;
}

static int winTextSetSpinMaxAttrib(Ihandle* ih, const char* value)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    int max;
    if (iupStrToInt(value, &max))
    {
      int min = iupAttribGetInt(ih, "SPINMIN");
      SendMessage(hSpin, UDM_SETRANGE32, min, max);

      winTextCropSpinValue(ih, hSpin, min, max);
    }
  }
  return 1;
}

static int winTextSetSpinIncAttrib(Ihandle* ih, const char* value)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    int inc;
    if (iupStrToInt(value, &inc))
    {
      UDACCEL paAccels[3];
      paAccels[0].nInc = inc;
      paAccels[0].nSec = 0;
      paAccels[1].nInc = inc*5;
      paAccels[1].nSec = 2;
      paAccels[2].nInc = inc*20;
      paAccels[2].nSec = 5;
      SendMessage(hSpin, UDM_SETACCEL, 3, (LPARAM)paAccels);
    }
  }
  return 1;
}

static int winTextSetSpinValueAttrib(Ihandle* ih, const char* value)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    int pos;
    if (iupStrToInt(value, &pos))
    {
      ih->data->disable_callbacks = 1;
      SendMessage(hSpin, UDM_SETPOS32, 0, pos);
      ih->data->disable_callbacks = 0;
    }
  }
  return 1;
}

static char* winTextGetSpinValueAttrib(Ihandle* ih)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    int pos = (int)SendMessage(hSpin, UDM_GETPOS32, 0, 0);
    return iupStrReturnInt(pos);
  }
  return NULL;
}


/****************************************************************************************/


static int winTextCtlColor(Ihandle* ih, HDC hdc, LRESULT *result)
{
  COLORREF cr;

  if (iupwinGetColorRef(ih, "FGCOLOR", &cr))
    SetTextColor(hdc, cr);

  if (iupwinGetColorRef(ih, "BGCOLOR", &cr))
  {
    SetBkColor(hdc, cr);
    SetDCBrushColor(hdc, cr);
    *result = (LRESULT)GetStockObject(DC_BRUSH);
    return 1;
  }
  return 0;
}

static void winTextCallCaretCb(Ihandle* ih)
{
  int col, lin, pos;

  IFniii cb = (IFniii)IupGetCallback(ih, "CARET_CB");
  if (!cb) return;

  pos = winTextGetCaret(ih, &lin, &col);

  if (pos != ih->data->last_caret_pos)
  {
    ih->data->last_caret_pos = pos;

    cb(ih, lin, col, pos);
  }
}

static int winTextCallActionCb(Ihandle* ih, char* insert_value, int remove_dir)
{
  int ret, start, end;
  IFnis cb;

  if (ih->data->disable_callbacks)
    return 1;

  cb = (IFnis)IupGetCallback(ih, "ACTION");
  winTextGetSelection(ih, &start, &end);
  ret = iupEditCallActionCb(ih, cb, insert_value, start, end, (char*)ih->data->mask, ih->data->nc, remove_dir, iupwinStrGetUTF8Mode());
  if (ret == 0)
    return 0;
  if (ret != -1)
  {
    WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDWNDPROC_CB");
    CallWindowProc(oldProc, ih->handle, WM_CHAR, ret, 0);  /* replace key */
    return 0;
  }
  return 1;
}

static int winTextSpinWmNotify(Ihandle* ih, NMHDR* msg_info, int *result)
{
  if (msg_info->code == UDN_DELTAPOS)
  {
    int min, max;
    NMUPDOWN *updown = (NMUPDOWN*)msg_info;
    HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
    int old_pos = (int)SendMessage(hSpin, UDM_GETPOS32, 0, 0);
    int pos = updown->iPos + updown->iDelta;
    SendMessage(hSpin, UDM_GETRANGE32, (WPARAM)&min, (LPARAM)&max);
    if (pos < min) pos = min;
    if (pos > max) pos = max;
    if (pos != old_pos)
    {
      IFni cb = (IFni) IupGetCallback(ih, "SPIN_CB");
      if (cb) 
      {
        int ret = cb(ih, pos);
        if (ret == IUP_IGNORE)
        {
          *result = 1;
          return 1;
        }
      }
    }
  }
  
  (void)result;
  return 0; /* result not used */
}

static int winTextMsgProc(Ihandle* ih, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result)
{
  int ret = 0;

  if (msg==WM_KEYDOWN) /* process K_ANY before text callbacks */
  {
    ret = iupwinBaseMsgProc(ih, msg, wp, lp, result);
    if (!iupObjectCheck(ih))
    {
      *result = 0;
      return 1;
    }

    if (ret)
    {
      iupAttribSet(ih, "_IUPWIN_IGNORE_CHAR", "1");
      *result = 0;
      return 1;
    }
    else
      iupAttribSet(ih, "_IUPWIN_IGNORE_CHAR", NULL);
  }

  switch (msg)
  {
  case WM_CHAR:
    {
      TCHAR c = (TCHAR)wp;

      /* even aborting WM_KEYDOWN, a WM_CHAR will be sent, so ignore it also */
      /* if a dialog was shown, the loop will be processed, so ignore out of focus WM_CHAR messages */
      if (GetFocus() != ih->handle || iupAttribGet(ih, "_IUPWIN_IGNORE_CHAR"))
      {
        iupAttribSet(ih, "_IUPWIN_IGNORE_CHAR", NULL);
        *result = 0;
        return 1;
      }

      if (c == TEXT('\b'))
      {              
        if (!winTextCallActionCb(ih, NULL, -1))  /* backspace */
          ret = 1;
      }
      else if (c == TEXT('\n') || c == TEXT('\r'))
      {
        if (ih->data->is_multiline && 
            !ih->data->has_formatting && !(GetKeyState(VK_CONTROL) & 0x8000)) /* when formatting is processed in WM_KEYDOWN */
        {
          char insert_value[2];
          insert_value[0] = '\n';
          insert_value[1] = 0;

          if (!winTextCallActionCb(ih, insert_value, 0))  /* insert new line */
            ret = 1;
        }
      }
      else if (wp != VK_ESCAPE)
      {
        int has_ctrl = GetKeyState(VK_CONTROL) & 0x8000;
        int has_alt = GetKeyState(VK_MENU) & 0x8000;
        int has_sys = (GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000);

        if ((!has_ctrl && !has_alt && !has_sys) ||  /* only process when no modifiers are used */
            (has_ctrl && has_alt && !has_sys))      /* except when Ctrl and Alt are pressed at the same time */
        {
          TCHAR insert_value[2];
          insert_value[0] = c;
          insert_value[1] = 0;

          if (!winTextCallActionCb(ih, iupwinStrFromSystem(insert_value), 0))  /* insert */
            ret = 1;
        }
      }

      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);

      if (!ih->data->is_multiline && 
          (wp==VK_RETURN || wp==VK_ESCAPE || wp==VK_TAB))  /* the keys have the same definitions as the chars */
        ret = 1;  /* abort default processing to avoid beep */

      if (ih->data->is_multiline && 
          (c==TEXT('\n') && (GetKeyState(VK_CONTROL) & 0x8000)))
        ret = 1;  /* abort default processing to avoid inserting a new line */

      break;
    }
  case WM_KEYDOWN:
    {
      if (wp == VK_DELETE) /* Del does not generates a WM_CHAR */
      {
        if (!winTextCallActionCb(ih, NULL, 1))  /* delete */
          ret = 1;
      }
      else if (wp == VK_INSERT && ih->data->has_formatting)
      {
        if (iupAttribGetBoolean(ih, "OVERWRITE"))
          iupAttribSet(ih, "OVERWRITE", "OFF"); /* toggle from ON to OFF */
        else
          iupAttribSet(ih, "OVERWRITE", "ON");  /* toggle from OFF to ON */
      }
      else if (wp == 'A' && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000))   /* Ctrl+A = Select All (no Alt key) */
      {
        SendMessage(ih->handle, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
      }
      else if (wp == VK_RETURN && ih->data->has_formatting && !(GetKeyState(VK_CONTROL) & 0x8000))
      {
        char insert_value[2];
        insert_value[0] = '\n';
        insert_value[1] = 0;

        if (!winTextCallActionCb(ih, insert_value, 0))  /* insert new line */
          ret = 1;
      }

      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);

      if (ret)       /* if abort processing, then the result is 0 */
      {
        *result = 0;
        return 1;
      }
      else
        return 0;  /* already processed at the beginning of this function */
    }
  case WM_KEYUP:
    {
      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_CLEAR:
    {
      if (!winTextCallActionCb(ih, NULL, 1))  /* same as delete */
        ret = 1;

      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_CUT:
    {
      if (!winTextCallActionCb(ih, NULL, 1))  /* same as delete */
        ret = 1;

      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_PASTE:
    {
      if (IupGetCallback(ih,"ACTION") || ih->data->mask) /* test before to avoid alocate clipboard text memory */
      {
        Ihandle* clipboard = IupClipboard();
        char* insert_value = IupGetAttribute(clipboard, "TEXT");
        IupDestroy(clipboard);
        if (insert_value)
        {
          if (!winTextCallActionCb(ih, insert_value, 0))  /* insert */
            ret = 1;
        }
      }

      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_UNDO:
    {
      IFnis cb = (IFnis)IupGetCallback(ih, "ACTION");
      if (cb)
      {
        char* value;
        WNDPROC oldProc = (WNDPROC)IupGetCallback(ih, "_IUPWIN_OLDWNDPROC_CB");
        CallWindowProc(oldProc, ih->handle, WM_UNDO, 0, 0);

        value = winTextGetValueAttrib(ih);
        cb(ih, 0, (char*)value);

        ret = 1;
      }

      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
    {
      iupwinFlagButtonDown(ih, msg);

      if (iupwinButtonDown(ih, msg, wp, lp)==-1)
      {
        *result = 0;
        return 1;
      }
      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
  case WM_LBUTTONUP:
    {
      if (!iupwinFlagButtonUp(ih, msg))
      {
        *result = 0;
        return 1;
      }

      if (iupwinButtonUp(ih, msg, wp, lp)==-1)
      {
        *result = 0;
        return 1;
      }
      PostMessage(ih->handle, WM_IUPCARET, 0, 0L);
      break;
    }
  case WM_IUPCARET:
    {
      winTextCallCaretCb(ih);
      break;
    }
  case WM_MOUSEMOVE:
    {
      iupwinMouseMove(ih, msg, wp, lp);
      break;
    }
  case WM_VSCROLL:
  case WM_HSCROLL:
    {
      if (ih->data->has_formatting)
      {
        /* fix weird behavior when dialog has COMPOSITE=YES, 
           scrollbars are not updated when dragging */
        if (LOWORD(wp) == SB_THUMBTRACK)
          SendMessage(ih->handle, EM_SHOWSCROLLBAR, msg==WM_VSCROLL? SB_VERT: SB_HORZ, TRUE);
      }
      break;
    }
  }

  if (ret)       /* if abort processing, then the result is 0 */
  {
    *result = 0;
    return 1;
  }
  else
    return iupwinBaseMsgProc(ih, msg, wp, lp, result);
}

static void winTextCreateSpin(Ihandle* ih)
{
  HWND hSpin;
  DWORD dwStyle = WS_CHILD|WS_CLIPSIBLINGS|UDS_ARROWKEYS|UDS_HOTTRACK|UDS_NOTHOUSANDS;
  int serial = iupDialogGetChildId(ih);

  if (iupStrEqualNoCase(iupAttribGetStr(ih, "SPINALIGN"), "LEFT"))
    dwStyle |= UDS_ALIGNLEFT;
  else
    dwStyle |= UDS_ALIGNRIGHT;

  if (iupAttribGetBoolean(ih, "SPINWRAP"))
    dwStyle |= UDS_WRAP;

  if (iupAttribGetBoolean(ih, "SPINAUTO"))
    dwStyle |= UDS_SETBUDDYINT;

  hSpin = iupwinCreateWindowEx(GetParent(ih->handle), UPDOWN_CLASS, 0, dwStyle, serial, NULL);
  if (!hSpin)
    return;

  iupwinHandleAdd(ih, hSpin);

  /* Process WM_NOTIFY */
  IupSetCallback(ih, "_IUPWIN_NOTIFY_CB", (Icallback)winTextSpinWmNotify);

  SendMessage(hSpin, UDM_SETBUDDY, (WPARAM)ih->handle, 0);
  iupAttribSet(ih, "_IUPWIN_SPIN", (char*)hSpin);

  /* default values, make sure limits are set before value */
  ih->data->disable_callbacks = 1;
  SendMessage(hSpin, UDM_SETRANGE32, iupAttribGetInt(ih, "SPINMIN"), iupAttribGetInt(ih, "SPINMAX"));
  winTextSetSpinIncAttrib(ih, iupAttribGetStr(ih, "SPININC"));
  SendMessage(hSpin, UDM_SETPOS32, 0, iupAttribGetInt(ih, "SPINMIN"));
  ih->data->disable_callbacks = 0;
}

static int winTextWmCommand(Ihandle* ih, WPARAM wp, LPARAM lp)
{
  int cmd = HIWORD(wp);
  switch (cmd)
  {
  case EN_CHANGE:
    {
      if (ih->data->disable_callbacks)
        return 0;

      iupBaseCallValueChangedCb(ih);
      break;
    }
  }

  (void)lp;
  return 0; /* not used */
}

static void winTextLayoutUpdateMethod(Ihandle* ih)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    int cur_x, cur_y, cur_w, cur_h, spin_w;
    RECT wrect;
    POINT pt;
    GetWindowRect(ih->handle, &wrect);
    pt.x = wrect.left;
    pt.y = wrect.top;
    ScreenToClient(GetParent(ih->handle), &pt);

    cur_x = pt.x;
    cur_y = pt.y;
    cur_w = wrect.right - wrect.left;
    cur_h = wrect.bottom - wrect.top;

    spin_w = (3 * ih->currentheight) / 4;

    if (iupStrEqualNoCase(iupAttribGetStr(ih, "SPINALIGN"), "LEFT"))
    {
      if (cur_x != ih->x + ih->currentheight - 1 || cur_y != ih->y ||
          cur_w != ih->currentwidth - ih->currentheight || cur_h != ih->currentheight)
      {
        SetWindowPos(ih->handle, NULL, ih->x + spin_w, ih->y, ih->currentwidth - spin_w, ih->currentheight,
          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

        SetWindowPos(hSpin, NULL, ih->x, ih->y, spin_w, ih->currentheight,
          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
      }
    }
    else
    {
      if (cur_x != ih->x || cur_y != ih->y ||
          cur_w != ih->currentwidth - spin_w || cur_h != ih->currentheight)
      {
        SetWindowPos(ih->handle, NULL, ih->x, ih->y, ih->currentwidth - spin_w, ih->currentheight,
          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

        SetWindowPos(hSpin, NULL, ih->x + ih->currentwidth - spin_w, ih->y, spin_w, ih->currentheight,
          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
      }
    }
  }
  else
    iupdrvBaseLayoutUpdateMethod(ih);
}

static void winTextUnMapMethod(Ihandle* ih)
{
  HWND hSpin = (HWND)iupAttribGet(ih, "_IUPWIN_SPIN");
  if (hSpin)
  {
    iupwinHandleRemove(hSpin);
    DestroyWindow(hSpin);
    iupAttribSet(ih, "_IUPWIN_SPIN", NULL);
  }

  iupdrvBaseUnMapMethod(ih);
}

static int winTextMapMethod(Ihandle* ih)
{
  DWORD dwStyle = WS_CHILD|WS_CLIPSIBLINGS, 
      dwExStyle = 0;
  char* value;
  TCHAR* winclass = WC_EDIT;

  if (!ih->parent)
    return IUP_ERROR;

  if (iupAttribGetBoolean(ih, "CANFOCUS"))
    dwStyle |= WS_TABSTOP;

  if (ih->data->has_formatting)
  {
    /* enable richedit 3.0 */
    static HMODULE richedit = NULL;
    if (!richedit)
      richedit = LoadLibrary(TEXT("Riched20.dll"));
    if (!richedit)
      return IUP_ERROR;

    winclass = RICHEDIT_CLASS;
  }

  if (ih->data->is_multiline)
  {
    dwStyle |= ES_AUTOVSCROLL|ES_MULTILINE|ES_WANTRETURN;

    if (iupAttribGetBoolean(ih, "WORDWRAP"))
    {
      ih->data->sb &= ~IUP_SB_HORIZ;  /* must remove the horizontal scrollbar
                                         and do not specify ES_AUTOHSCROLL, 
                                         the control automatically wraps words */
    }
    else                           
      dwStyle |= ES_AUTOHSCROLL;   

    if (ih->data->sb & IUP_SB_HORIZ)
      dwStyle |= WS_HSCROLL;
    if (ih->data->sb & IUP_SB_VERT)
      dwStyle |= WS_VSCROLL;

    if (ih->data->has_formatting && ih->data->sb != IUP_SB_NONE)
    {
      if (!iupAttribGetBoolean(ih, "AUTOHIDE"))
        dwStyle |= ES_DISABLENOSCROLL;
    }
  }
  else
  {
    dwStyle |= ES_AUTOHSCROLL;

    if (iupAttribGetBoolean(ih, "PASSWORD"))
      dwStyle |= ES_PASSWORD;
  }

  value = iupAttribGet(ih, "ALIGNMENT");
  if (value)
  {
    if (iupStrEqualNoCase(value, "ARIGHT"))
      dwStyle |= ES_RIGHT;
    else if (iupStrEqualNoCase(value, "ACENTER"))
      dwStyle |= ES_CENTER;
    else /* default "ALEFT" */
      dwStyle |= ES_LEFT;
  }

  if (iupAttribGetBoolean(ih, "NOHIDESEL"))
    dwStyle |= ES_NOHIDESEL;

  if (iupAttribGetBoolean(ih, "BORDER"))
    dwExStyle |= WS_EX_CLIENTEDGE;

  if (!iupwinCreateWindow(ih, winclass, dwExStyle, dwStyle, NULL))
    return IUP_ERROR;

  /* Process ACTION_CB and CARET_CB */
  IupSetCallback(ih, "_IUPWIN_CTRLMSGPROC_CB", (Icallback)winTextMsgProc);

  /* Process background color */
  IupSetCallback(ih, "_IUPWIN_CTLCOLOR_CB", (Icallback)winTextCtlColor);

  /* Process WM_COMMAND */
  IupSetCallback(ih, "_IUPWIN_COMMAND_CB", (Icallback)winTextWmCommand);

  /* set defaults */
  SendMessage(ih->handle, EM_LIMITTEXT, 0, 0L);
  {
    int tabsize = 8*4;
    SendMessage(ih->handle, EM_SETTABSTOPS, (WPARAM)1L, (LPARAM)&tabsize);
  }

  if (!ih->data->is_multiline && iupAttribGetBoolean(ih, "SPIN"))
    winTextCreateSpin(ih);

  /* configure for DROP of files */
  if (IupGetCallback(ih, "DROPFILES_CB"))
    iupAttribSet(ih, "DROPFILESTARGET", "YES");

  if (ih->data->has_formatting)
  {
    SendMessage(ih->handle, EM_SETTEXTMODE, (WPARAM)(TM_RICHTEXT|TM_MULTILEVELUNDO|TM_SINGLECODEPAGE), 0);
    SendMessage(ih->handle, EM_SETEVENTMASK, 0, ENM_CHANGE);

    /* must update FONT before updating the formattags */
    iupAttribSet(ih, "_IUPWIN_FONTUPDATECHECK", "1");
    iupUpdateFontAttrib(ih);

    if (ih->data->formattags)
      iupTextUpdateFormatTags(ih);
  }

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)winTextConvertXYToPos);

  return IUP_NOERROR;
}

void iupdrvTextInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = winTextMapMethod;
  ic->LayoutUpdate = winTextLayoutUpdateMethod;
  ic->UnMap = winTextUnMapMethod;

  /* Driver Dependent Attribute functions */

  iupClassRegisterAttribute(ic, "FONT", NULL, winTextSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);  /* inherited */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, winTextSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);  
  iupClassRegisterAttribute(ic, "VISIBLE", iupBaseGetVisibleAttrib, winTextSetVisibleAttrib, "YES", "NO", IUPAF_NO_SAVE|IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, winTextSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, winTextSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTFGCOLOR", IUPAF_NOT_MAPPED);  /* usually black */    
  iupClassRegisterAttribute(ic, "AUTOREDRAW", NULL, iupwinSetAutoRedrawAttrib, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  /* IupText only */
  iupClassRegisterAttribute(ic, "PADDING", iupTextGetPaddingAttrib, winTextSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "VALUE", winTextGetValueAttrib, winTextSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINEVALUE", winTextGetLineValueAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTEDTEXT", winTextGetSelectedTextAttrib, winTextSetSelectedTextAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTION", winTextGetSelectionAttrib, winTextSetSelectionAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SELECTIONPOS", winTextGetSelectionPosAttrib, winTextSetSelectionPosAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARET", winTextGetCaretAttrib, winTextSetCaretAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CARETPOS", winTextGetCaretPosAttrib, winTextSetCaretPosAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_SAVE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "INSERT", NULL, winTextSetInsertAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "APPEND", NULL, winTextSetAppendAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "READONLY", winTextGetReadOnlyAttrib, winTextSetReadOnlyAttrib, NULL, NULL, IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "NC", iupTextGetNCAttrib, winTextSetNCAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NOT_MAPPED);
  iupClassRegisterAttribute(ic, "CLIPBOARD", NULL, winTextSetClipboardAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTO", NULL, winTextSetScrollToAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SCROLLTOPOS", NULL, winTextSetScrollToPosAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMIN", NULL, winTextSetSpinMinAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINMAX", NULL, winTextSetSpinMaxAttrib, IUPAF_SAMEASSYSTEM, "100", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPININC", NULL, winTextSetSpinIncAttrib, IUPAF_SAMEASSYSTEM, "1", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SPINVALUE", winTextGetSpinValueAttrib, winTextSetSpinValueAttrib, IUPAF_SAMEASSYSTEM, "0", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", winTextGetCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LINECOUNT", winTextGetLineCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  /* IupText Windows and GTK only */
  iupClassRegisterAttribute(ic, "ADDFORMATTAG", NULL, iupTextSetAddFormatTagAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDFORMATTAG_HANDLE", NULL, iupTextSetAddFormatTagHandleAttrib, NULL, NULL, IUPAF_IHANDLE | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, winTextSetAlignmentAttrib, IUPAF_SAMEASSYSTEM, "ALEFT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATTING", iupTextGetFormattingAttrib, iupTextSetFormattingAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "OVERWRITE", winTextGetOverwriteAttrib, winTextSetOverwriteAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "REMOVEFORMATTING", NULL, winTextSetRemoveFormattingAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABSIZE", NULL, winTextSetTabSizeAttrib, IUPAF_SAMEASSYSTEM, "8", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "PASSWORD", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  /* IupText Windows only */
  iupClassRegisterAttribute(ic, "CUEBANNER", NULL, winTextSetCueBannerAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTER", NULL, winTextSetFilterAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NOHIDESEL", NULL, NULL, IUPAF_SAMEASSYSTEM, "Yes", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LOADRTF", NULL, winTextSetLoadRtfAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVERTF", NULL, winTextSetSaveRtfAttrib, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LOADRTFSTATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SAVERTFSTATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CONTROLID", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
