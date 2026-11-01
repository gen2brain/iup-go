/** \file
 * \brief IupPrintDlg - WebAssembly driver
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"

#define IUPWASM_PRINT_MARGIN 48

EM_JS(int, iupwasmJsPrintLetter, (void), {
  var lang = (typeof navigator !== 'undefined' && navigator.language) ? navigator.language : "";
  return /-(US|CA|MX|PH|CL|CO|VE|GT|CR|PR)$/i.test(lang) ? 1 : 0;
})

EM_JS(void, iupwasmJsPrint, (const char* html, const char* title, int landscape, int letter), {
  var req = { op: 'print', html: UTF8ToString(html), title: UTF8ToString(title), landscape: landscape, letter: letter };
  if (typeof document === 'undefined')
    globalThis.__iupReadSync(req);
  else
    globalThis.__iupPrintPages(req);
})

static void wasmPrintDlgAppend(char** buffer, int* len, int* size, const char* str)
{
  int n = (int)strlen(str);
  if (*len + n + 1 > *size)
  {
    *size = (*len + n + 1) * 2;
    *buffer = (char*)realloc(*buffer, *size);
  }
  memcpy(*buffer + *len, str, n + 1);
  *len += n;
}

static int wasmPrintDlgPopup(Ihandle* ih, int x, int y)
{
  int landscape = iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE");
  int letter = iupwasmJsPrintLetter();
  int paper_w = letter ? 816 : 794, paper_h = letter ? 1056 : 1123;
  int width, height, count, page, len = 0, size = 0;
  char* html = NULL;
  char div[100];

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (landscape)
  {
    int tmp = paper_w;
    paper_w = paper_h;
    paper_h = tmp;
  }

  width = paper_w - 2 * IUPWASM_PRINT_MARGIN;
  height = paper_h - 2 * IUPWASM_PRINT_MARGIN;
  count = iupPrintDlgPaginate(ih, width, height);

  wasmPrintDlgAppend(&html, &len, &size, "");

  for (page = 1; page <= count; page++)
  {
    char* svg = iupPrintDlgGetPageSvg(ih, page, width, height);

    snprintf(div, sizeof(div), "<div class=\"iup-page\" style=\"width:%dpx;height:%dpx\">", width, height);
    wasmPrintDlgAppend(&html, &len, &size, div);
    if (svg)
    {
      wasmPrintDlgAppend(&html, &len, &size, svg);
      free(svg);
    }
    wasmPrintDlgAppend(&html, &len, &size, "</div>");
  }

  iupwasmJsPrint(html, iupPrintDlgGetJobName(ih), landscape, letter);
  free(html);

  iupAttribSet(ih, "STATUS", "1");
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvPrintDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = wasmPrintDlgPopup;
}
