/** \file
 * \brief IupPrintDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupdraw.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"
#include "iup_register.h"
#include "iup_predialogs.h"
#include "iup_childtree.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_drvdraw.h"
#include "iup_draw.h"
#include "iup_draw_svg.h"


IUP_API Ihandle* IupPrintDlg(void)
{
  return IupCreate("printdlg");
}

IUP_SDK_API const char* iupPrintDlgGetJobName(Ihandle* ih)
{
  const char* name = iupAttribGet(ih, "JOBNAME");
  if (!name)
    name = iupAttribGet(ih, "TITLE");
  if (!name)
    name = "IUP";
  return name;
}

#define IPRINTDLG_HPAD 6
#define IPRINTDLG_VPAD 3
#define IPRINTDLG_INDENT 20

enum { IPRINTDLG_CANVAS, IPRINTDLG_TABLE, IPRINTDLG_LINES };

typedef struct _IprintDlgTable {
  Ihandle* element;
  int is_matrix, numlin, first_col, last_col;
  int* widths;
  char* capped;
  int total_width, line_height, row_height, rows_per_page;
  double scale;
} IprintDlgTable;

typedef struct _IprintDlgLines {
  Ihandle* element;
  char** lines;
  int* indents;
  int count, alloc, line_height, lines_per_page;
} IprintDlgLines;

static Ihandle* iPrintDlgGetElement(Ihandle* ih)
{
  Ihandle* element = IupGetAttributeHandle(ih, "ELEMENT");
  if (element && iupObjectCheck(element))
    return element;
  return NULL;
}

static int iPrintDlgElementKind(Ihandle* element)
{
  Iclass* ic = element->iclass;
  if (iupClassMatch(ic, "table") || iupClassMatch(ic, "matrix"))
    return IPRINTDLG_TABLE;
  if (iupClassMatch(ic, "text") || iupClassMatch(ic, "multiline") || iupClassMatch(ic, "list") ||
      iupClassMatch(ic, "flatlist") || iupClassMatch(ic, "tree") || iupClassMatch(ic, "flattree") ||
      iupClassMatch(ic, "label"))
    return IPRINTDLG_LINES;
  return IPRINTDLG_CANVAS;
}

static void iPrintDlgTableFree(Ihandle* ih)
{
  IprintDlgTable* table = (IprintDlgTable*)iupAttribGet(ih, "_IUP_PRINT_TABLE");
  if (table)
  {
    free(table->widths);
    free(table->capped);
    free(table);
    iupAttribSet(ih, "_IUP_PRINT_TABLE", NULL);
  }
}

static void iPrintDlgLinesFree(Ihandle* ih)
{
  IprintDlgLines* lines = (IprintDlgLines*)iupAttribGet(ih, "_IUP_PRINT_LINES");
  if (lines)
  {
    int i;
    for (i = 0; i < lines->count; i++)
      free(lines->lines[i]);
    free(lines->lines);
    free(lines->indents);
    free(lines);
    iupAttribSet(ih, "_IUP_PRINT_LINES", NULL);
  }
}

static void iPrintDlgDestroyMethod(Ihandle* ih)
{
  iPrintDlgTableFree(ih);
  iPrintDlgLinesFree(ih);
}

static const char* iPrintDlgTableCell(IprintDlgTable* table, int lin, int col)
{
  if (table->is_matrix)
    return IupGetAttributeId2(table->element, "CELL", lin, col);
  if (lin == 0)
    return IupGetAttributeId(table->element, "TITLE", col);
  return IupGetAttributeId2(table->element, "IDVALUE", lin, col);
}

static const char* iPrintDlgTableAlignment(IprintDlgTable* table, int col)
{
  const char* value = IupGetAttributeId(table->element, "ALIGNMENT", col);
  if (value && iupStrEqualNoCasePartial(value, "ARIGHT"))
    return "ARIGHT";
  if (value && iupStrEqualNoCasePartial(value, "ACENTER"))
    return "ACENTER";
  return "ALEFT";
}

static int iPrintDlgTableHidden(IprintDlgTable* table, int col)
{
  if (!table->is_matrix)
    return 0;
  return iupStrEqual(IupGetAttributeId(table->element, "WIDTH", col), "0") ||
         iupStrEqual(IupGetAttributeId(table->element, "RASTERWIDTH", col), "0");
}

static IprintDlgTable* iPrintDlgTableLayout(Ihandle* ih, Ihandle* element, int width, int height)
{
  IprintDlgTable* table = (IprintDlgTable*)calloc(1, sizeof(IprintDlgTable));
  const char* font = IupGetAttribute(element, "FONT");
  int lin, col, numcol;

  table->element = element;
  table->is_matrix = iupClassMatch(element->iclass, "matrix");
  table->numlin = IupGetInt(element, "NUMLIN");
  numcol = IupGetInt(element, "NUMCOL");
  table->first_col = 1;
  if (table->is_matrix)
  {
    const char* title = iPrintDlgTableCell(table, 1, 0);
    if (title && *title)
      table->first_col = 0;
  }
  table->last_col = numcol;
  table->widths = (int*)calloc(numcol + 1, sizeof(int));
  table->capped = (char*)calloc(numcol + 1, sizeof(char));

  iupdrvFontGetFontDim(font, NULL, &table->line_height, NULL, NULL);
  table->row_height = table->line_height + 2 * IPRINTDLG_VPAD;

  for (col = table->first_col; col <= table->last_col; col++)
  {
    int col_width = 0;

    if (iPrintDlgTableHidden(table, col))
      continue;

    for (lin = 0; lin <= table->numlin; lin++)
    {
      const char* text = iPrintDlgTableCell(table, lin, col);
      if (text && *text)
      {
        int w = 0, h = 0;
        iupdrvFontGetTextSize(font, text, (int)strlen(text), &w, &h);
        if (w > col_width)
          col_width = w;
      }
    }

    col_width += col_width / 20 + 2 * IPRINTDLG_HPAD;
    if (col_width > width)
    {
      col_width = width;
      table->capped[col] = 1;
    }
    table->widths[col] = col_width;
    table->total_width += col_width;
  }

  table->scale = 1;
  if (table->total_width > width)
    table->scale = (double)width / (double)table->total_width;

  table->rows_per_page = (int)((height / table->scale - table->row_height) / table->row_height);
  if (table->rows_per_page < 1)
    table->rows_per_page = 1;

  iPrintDlgTableFree(ih);
  iupAttribSet(ih, "_IUP_PRINT_TABLE", (char*)table);
  return table;
}

static void iPrintDlgTableDrawLine(Ihandle* ih, IprintDlgTable* table, int lin, int y)
{
  int col, x = 0;

  for (col = table->first_col; col <= table->last_col; col++)
  {
    int w = table->widths[col];
    const char* text;

    if (!w)
      continue;

    text = iPrintDlgTableCell(table, lin, col);
    if (text && *text)
    {
      IupSetAttribute(ih, "DRAWTEXTALIGNMENT", iPrintDlgTableAlignment(table, col));
      IupSetAttribute(ih, "DRAWTEXTELLIPSIS", table->capped[col] ? "YES" : "NO");
      IupDrawText(ih, text, -1, x + IPRINTDLG_HPAD, y + IPRINTDLG_VPAD, w - 2 * IPRINTDLG_HPAD, table->line_height);
    }
    x += w;
  }
}

static void iPrintDlgTableDrawPage(Ihandle* ih, IprintDlgTable* table, int page)
{
  int first = (page - 1) * table->rows_per_page + 1;
  int last = first + table->rows_per_page - 1;
  int lin, y;

  if (last > table->numlin)
    last = table->numlin;

  IupDrawBegin(ih);
  IupDrawSave(ih);

  if (table->scale < 1)
    IupDrawScale(ih, table->scale, table->scale);

  IupSetStrAttribute(ih, "DRAWFONT", IupGetAttribute(table->element, "FONT"));
  IupSetAttribute(ih, "DRAWLINEWIDTH", "1");

  IupSetAttribute(ih, "DRAWSTYLE", "FILL");
  IupSetAttribute(ih, "DRAWCOLOR", "230 230 230");
  IupDrawRectangle(ih, 0, 0, table->total_width - 1, table->row_height - 1);

  IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
  iPrintDlgTableDrawLine(ih, table, 0, 0);

  IupSetAttribute(ih, "DRAWSTYLE", "STROKE");
  IupSetAttribute(ih, "DRAWCOLOR", "128 128 128");
  IupDrawLine(ih, 0, table->row_height - 1, table->total_width - 1, table->row_height - 1);

  y = table->row_height;
  for (lin = first; lin <= last; lin++)
  {
    IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
    iPrintDlgTableDrawLine(ih, table, lin, y);
    y += table->row_height;

    IupSetAttribute(ih, "DRAWCOLOR", "210 210 210");
    IupDrawLine(ih, 0, y - 1, table->total_width - 1, y - 1);
  }

  IupDrawRestore(ih);
  IupDrawEnd(ih);
}

static void iPrintDlgLinesAppend(IprintDlgLines* lines, const char* str, int len, int indent)
{
  char* line;

  if (lines->count == lines->alloc)
  {
    lines->alloc = lines->alloc ? 2 * lines->alloc : 64;
    lines->lines = (char**)realloc(lines->lines, lines->alloc * sizeof(char*));
    lines->indents = (int*)realloc(lines->indents, lines->alloc * sizeof(int));
  }

  line = (char*)malloc(len + 1);
  memcpy(line, str, len);
  line[len] = 0;

  lines->lines[lines->count] = line;
  lines->indents[lines->count] = indent;
  lines->count++;
}

static int iPrintDlgUtf8Len(const char* str)
{
  unsigned char c = (unsigned char)*str;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 1;
}

static int iPrintDlgTextWidth(const char* font, const char* str, int len)
{
  int w = 0, h = 0;
  if (len > 0)
    iupdrvFontGetTextSize(font, str, len, &w, &h);
  return w;
}

static void iPrintDlgLinesAddWrapped(IprintDlgLines* lines, const char* font, const char* str, int len, int indent, int width)
{
  const char* end = str + len;
  int avail;

  if (indent > width / 2)
    indent = width / 2;
  avail = (width - indent) * 95 / 100;

  if (len == 0 || iPrintDlgTextWidth(font, str, len) <= avail)
  {
    iPrintDlgLinesAppend(lines, str, len, indent);
    return;
  }

  while (str < end)
  {
    const char* p = str;
    const char* space = NULL;

    while (p < end)
    {
      const char* next = p + iPrintDlgUtf8Len(p);
      if (next > end)
        next = end;
      if (iPrintDlgTextWidth(font, str, (int)(next - str)) > avail)
        break;
      if (*p == ' ')
        space = p;
      p = next;
    }

    if (p >= end)
    {
      iPrintDlgLinesAppend(lines, str, (int)(end - str), indent);
      break;
    }

    if (space && space > str)
    {
      iPrintDlgLinesAppend(lines, str, (int)(space - str), indent);
      str = space + 1;
    }
    else
    {
      if (p == str)
        p = str + iPrintDlgUtf8Len(str);
      iPrintDlgLinesAppend(lines, str, (int)(p - str), indent);
      str = p;
    }
  }
}

static void iPrintDlgLinesAddText(IprintDlgLines* lines, const char* font, const char* text, int indent, int width)
{
  char* value = iupStrDup(text);
  char* line = value;

  if (!value)
    return;

  while (line)
  {
    char* next = strchr(line, '\n');
    char* c;
    int len;

    if (next)
      *next++ = 0;

    for (c = line; *c; c++)
    {
      if (*c == '\t')
        *c = ' ';
    }

    len = (int)strlen(line);
    if (len > 0 && line[len - 1] == '\r')
      len--;

    iPrintDlgLinesAddWrapped(lines, font, line, len, indent, width);
    line = next;
  }

  free(value);
}

static IprintDlgLines* iPrintDlgLinesLayout(Ihandle* ih, Ihandle* element, int width, int height)
{
  IprintDlgLines* lines = (IprintDlgLines*)calloc(1, sizeof(IprintDlgLines));
  const char* font = IupGetAttribute(element, "FONT");
  Iclass* ic = element->iclass;
  int i, count;

  lines->element = element;
  iupdrvFontGetFontDim(font, NULL, &lines->line_height, NULL, NULL);
  if (lines->line_height < 1)
    lines->line_height = 1;

  if (iupClassMatch(ic, "list") || iupClassMatch(ic, "flatlist"))
  {
    count = IupGetInt(element, "COUNT");
    for (i = 1; i <= count; i++)
    {
      const char* item = IupGetAttributeId(element, "", i);
      iPrintDlgLinesAddText(lines, font, item ? item : "", 0, width);
    }
  }
  else if (iupClassMatch(ic, "tree") || iupClassMatch(ic, "flattree"))
  {
    count = IupGetInt(element, "COUNT");
    for (i = 0; i < count; i++)
    {
      const char* title = IupGetAttributeId(element, "TITLE", i);
      iPrintDlgLinesAddText(lines, font, title ? title : "", IupGetIntId(element, "DEPTH", i) * IPRINTDLG_INDENT, width);
    }
  }
  else if (iupClassMatch(ic, "label"))
  {
    const char* title = IupGetAttribute(element, "TITLE");
    if (title)
    {
      char* text = iupStrProcessMnemonic(title, NULL, 0);
      iPrintDlgLinesAddText(lines, font, text, 0, width);
      if (text != title)
        free(text);
    }
  }
  else
  {
    const char* text = IupGetAttribute(element, "VALUE");
    if (text)
      iPrintDlgLinesAddText(lines, font, text, 0, width);
  }

  lines->lines_per_page = height / lines->line_height;
  if (lines->lines_per_page < 1)
    lines->lines_per_page = 1;

  iPrintDlgLinesFree(ih);
  iupAttribSet(ih, "_IUP_PRINT_LINES", (char*)lines);
  return lines;
}

static void iPrintDlgLinesDrawPage(Ihandle* ih, IprintDlgLines* lines, int width, int page)
{
  int first = (page - 1) * lines->lines_per_page;
  int last = first + lines->lines_per_page;
  int i, y = 0;

  if (last > lines->count)
    last = lines->count;

  IupDrawBegin(ih);
  IupDrawSave(ih);

  IupSetStrAttribute(ih, "DRAWFONT", IupGetAttribute(lines->element, "FONT"));
  IupSetAttribute(ih, "DRAWCOLOR", "0 0 0");
  IupSetAttribute(ih, "DRAWTEXTALIGNMENT", "ALEFT");
  IupSetAttribute(ih, "DRAWTEXTELLIPSIS", "NO");

  for (i = first; i < last; i++)
  {
    if (lines->lines[i][0])
      IupDrawText(ih, lines->lines[i], -1, lines->indents[i], y, width - lines->indents[i], lines->line_height);
    y += lines->line_height;
  }

  IupDrawRestore(ih);
  IupDrawEnd(ih);
}

static void iPrintDlgDrawCanvas(Ihandle* element, void* context, int width, int height)
{
  IFnff cb = (IFnff)IupGetCallback(element, "ACTION");
  double scale = 1;
  int w = 0, h = 0;

  if (!cb || iupAttribGet(element, "_IUP_DRAW_DC"))
    return;

  IupGetIntInt(element, "DRAWSIZE", &w, &h);
  if (w <= 0 || h <= 0)
    return;

  if (w > width)
    scale = (double)width / (double)w;
  if (h * scale > height)
    scale = (double)height / (double)h;

  iupAttribSet(element, "_IUP_PRINT_CONTEXT", (char*)context);
  iupAttribSetInt(element, "_IUP_PRINT_WIDTH", w);
  iupAttribSetInt(element, "_IUP_PRINT_HEIGHT", h);
  if (scale < 1)
    iupAttribSetDouble(element, "_IUP_PRINT_SCALE", scale);

  cb(element, IupGetFloat(element, "POSX"), IupGetFloat(element, "POSY"));

  iupAttribSet(element, "_IUP_PRINT_CONTEXT", NULL);
  iupAttribSet(element, "_IUP_PRINT_WIDTH", NULL);
  iupAttribSet(element, "_IUP_PRINT_HEIGHT", NULL);
  iupAttribSet(element, "_IUP_PRINT_SCALE", NULL);
}

IUP_SDK_API int iupPrintDlgPaginate(Ihandle* ih, int width, int height)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "PAGINATE_CB");
  Ihandle* element = iPrintDlgGetElement(ih);
  int count;

  iupAttribSetStrf(ih, "PAGESIZE", "%dx%d", width, height);

  if (cb)
    count = cb(ih, width, height);
  else if (element && !IupGetCallback(ih, "PRINTPAGE_CB"))
  {
    int kind = iPrintDlgElementKind(element);
    if (kind == IPRINTDLG_TABLE)
    {
      IprintDlgTable* table = iPrintDlgTableLayout(ih, element, width, height);
      count = (table->numlin + table->rows_per_page - 1) / table->rows_per_page;
    }
    else if (kind == IPRINTDLG_LINES)
    {
      IprintDlgLines* lines = iPrintDlgLinesLayout(ih, element, width, height);
      count = (lines->count + lines->lines_per_page - 1) / lines->lines_per_page;
    }
    else
      count = 1;
  }
  else
    count = iupAttribGetInt(ih, "PAGECOUNT");

  if (count < 1)
    count = 1;
  return count;
}

IUP_SDK_API int iupPrintDlgDrawPage(Ihandle* ih, void* context, int page, int width, int height)
{
  IFniii cb = (IFniii)IupGetCallback(ih, "PRINTPAGE_CB");
  Ihandle* element = iPrintDlgGetElement(ih);
  int ret = IUP_DEFAULT;

  if (!cb && !element)
    return IUP_DEFAULT;

  if (!cb && iPrintDlgElementKind(element) == IPRINTDLG_CANVAS)
  {
    iPrintDlgDrawCanvas(element, context, width, height);
    return IUP_DEFAULT;
  }

  iupAttribSet(ih, "_IUP_PRINT_CONTEXT", (char*)context);
  iupAttribSetInt(ih, "_IUP_PRINT_WIDTH", width);
  iupAttribSetInt(ih, "_IUP_PRINT_HEIGHT", height);

  if (cb)
    ret = cb(ih, page, width, height);
  else if (iPrintDlgElementKind(element) == IPRINTDLG_TABLE)
  {
    IprintDlgTable* table = (IprintDlgTable*)iupAttribGet(ih, "_IUP_PRINT_TABLE");
    if (!table || table->element != element)
      table = iPrintDlgTableLayout(ih, element, width, height);
    iPrintDlgTableDrawPage(ih, table, page);
  }
  else
  {
    IprintDlgLines* lines = (IprintDlgLines*)iupAttribGet(ih, "_IUP_PRINT_LINES");
    if (!lines || lines->element != element)
      lines = iPrintDlgLinesLayout(ih, element, width, height);
    iPrintDlgLinesDrawPage(ih, lines, width, page);
  }

  iupAttribSet(ih, "_IUP_PRINT_CONTEXT", NULL);
  iupAttribSet(ih, "_IUP_PRINT_WIDTH", NULL);
  iupAttribSet(ih, "_IUP_PRINT_HEIGHT", NULL);

  return ret;
}

IUP_SDK_API char* iupPrintDlgGetPageSvg(Ihandle* ih, int page, int width, int height)
{
  Ihandle* element = iPrintDlgGetElement(ih);
  Ihandle* target = ih;
  iSvgCanvas* svg;
  const char* str;
  char* result = NULL;
  int w = width, h = height;

  if (element && !IupGetCallback(ih, "PRINTPAGE_CB") && iPrintDlgElementKind(element) == IPRINTDLG_CANVAS)
  {
    if (!IupGetCallback(element, "ACTION"))
      return NULL;
    target = element;
    IupGetIntInt(element, "DRAWSIZE", &w, &h);
  }

  if (w <= 0 || h <= 0 || iupAttribGet(target, "_IUP_DRAW_DC"))
    return NULL;

  svg = iupSvgDrawCreateCanvas(target, w, h);
  if (!svg)
    return NULL;

  iupAttribSet(target, "_IUP_SVG_CANVAS", (char*)svg);

  if (target == element)
  {
    IFnff cb = (IFnff)IupGetCallback(element, "ACTION");
    cb(element, IupGetFloat(element, "POSX"), IupGetFloat(element, "POSY"));
  }
  else
    iupPrintDlgDrawPage(ih, NULL, page, width, height);

  str = iupSvgDrawGetString(svg);
  if (str)
    result = iupStrDup(str);

  iupAttribSet(target, "_IUP_SVG_CANVAS", NULL);
  iupSvgDrawKillCanvas(svg);

  return result;
}

IUP_SDK_API int iupPrintDlgPageSelected(Ihandle* ih, int page)
{
  const char* pages = iupAttribGet(ih, "_IUP_PRINT_PAGES");
  char* end;

  if (!pages)
    return 1;

  while (*pages)
  {
    int first, last;

    while (*pages == ' ' || *pages == ',')
      pages++;
    if (!*pages)
      break;

    if (*pages == '-')
      first = 1;
    else
    {
      first = (int)strtol(pages, &end, 10);
      if (end == pages)
        return 0;
      pages = end;
    }
    last = first;

    while (*pages == ' ')
      pages++;
    if (*pages == '-')
    {
      pages++;
      last = (int)strtol(pages, &end, 10);
      if (end == pages)
        last = page;
      pages = end;
    }

    if (page >= first && page <= last)
      return 1;

    while (*pages && *pages != ',')
      pages++;
  }

  return 0;
}

/******************************************************************************
 * Custom Print Dialog
 *****************************************************************************/

static const IprintDlgDriver* iprintdlg_driver = NULL;

static int iPrintDlgIsFile(Ihandle* printer_list)
{
  return IupGetInt(printer_list, "VALUE") == IupGetInt(printer_list, "COUNT");
}

static char* iPrintDlgGetPrinter(Ihandle* printer_list)
{
  if (iPrintDlgIsFile(printer_list))
    return NULL;
  return IupGetAttributeId(printer_list, "", IupGetInt(printer_list, "VALUE"));
}

static void iPrintDlgUpdatePrinter(Ihandle* ih)
{
  Ihandle* printer_list = IupGetDialogChild(ih, "PRINTER");
  Ihandle* paper_list = IupGetDialogChild(ih, "PAPER");
  int is_file = iPrintDlgIsFile(printer_list);

  IupSetAttribute(paper_list, "1", NULL);
  iprintdlg_driver->FillPapers(paper_list, iPrintDlgGetPrinter(printer_list));

  IupSetAttribute(IupGetDialogChild(ih, "FILENAME"), "ACTIVE", is_file ? "YES" : "NO");
  IupSetAttribute(IupGetDialogChild(ih, "FILEBUT"), "ACTIVE", is_file ? "YES" : "NO");
}

static int iPrintDlgPrinter_CB(Ihandle* list, char* text, int item, int state)
{
  (void)text;
  (void)item;
  if (state == 1)
    iPrintDlgUpdatePrinter(IupGetDialog(list));
  return IUP_DEFAULT;
}

static int iPrintDlgPages_CB(Ihandle* toggle, int state)
{
  Ihandle* pages = IupGetDialogChild(toggle, "PAGES");
  IupSetAttribute(pages, "ACTIVE", state ? "YES" : "NO");
  if (state)
    IupSetFocus(pages);
  return IUP_DEFAULT;
}

static int iPrintDlgButtonFile_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  Ihandle* filename = IupGetDialogChild(dlg, "FILENAME");
  Ihandle* filedlg = IupFileDlg();

  IupSetAttribute(filedlg, "DIALOGTYPE", "SAVE");
  IupSetAttribute(filedlg, "EXTFILTER", "PDF|*.pdf|");
  IupSetStrAttribute(filedlg, "FILE", IupGetAttribute(filename, "VALUE"));
  IupSetAttributeHandle(filedlg, "PARENTDIALOG", dlg);

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(filedlg, "STATUS") != -1)
    IupSetStrAttribute(filename, "VALUE", IupGetAttribute(filedlg, "VALUE"));

  IupDestroy(filedlg);
  return IUP_DEFAULT;
}

static int iPrintDlgPagesValid(const char* pages)
{
  int digits = 0;

  for (; *pages; pages++)
  {
    if (*pages >= '0' && *pages <= '9')
      digits = 1;
    else if (*pages != ' ' && *pages != ',' && *pages != '-')
      return 0;
  }

  return digits;
}

static int iPrintDlgButtonPrint_CB(Ihandle* self)
{
  Ihandle* dlg = IupGetDialog(self);
  Ihandle* printer_list = IupGetDialogChild(dlg, "PRINTER");
  Ihandle* filename = IupGetDialogChild(dlg, "FILENAME");
  Ihandle* pages = IupGetDialogChild(dlg, "PAGES");
  char* printer = iupStrDup(iPrintDlgGetPrinter(printer_list));
  char* file = iupStrDup(IupGetAttribute(filename, "VALUE"));
  char* paper = iupStrDup(IupGetAttribute(IupGetDialogChild(dlg, "PAPER"), "VALUESTRING"));
  int landscape = IupGetInt(IupGetDialogChild(dlg, "ORIENTATION"), "VALUE") == 2;
  int copies = IupGetInt(IupGetDialogChild(dlg, "COPIES"), "SPINVALUE");
  int ret = IUP_DEFAULT;

  if (!printer && (!file || !*file))
    IupSetFocus(filename);
  else if (IupGetInt(IupGetDialogChild(dlg, "RANGEPAGES"), "VALUE") && !iPrintDlgPagesValid(IupGetAttribute(pages, "VALUE")))
    IupSetFocus(pages);
  else
  {
    int status;

    if (IupGetInt(IupGetDialogChild(dlg, "RANGEPAGES"), "VALUE"))
      iupAttribSetStr(dlg, "_IUP_PRINT_PAGES", IupGetAttribute(pages, "VALUE"));
    else
      iupAttribSet(dlg, "_IUP_PRINT_PAGES", NULL);

    iupAttribSet(dlg, "ORIENTATION", landscape ? "LANDSCAPE" : "PORTRAIT");

    IupSetAttribute(dlg, "ACTIVE", "NO");
    status = iprintdlg_driver->Print(dlg, printer, file, paper, landscape, copies > 0 ? copies : 1);
    IupSetAttribute(dlg, "ACTIVE", "YES");

    iupAttribSet(dlg, "_IUP_PRINT_PAGES", NULL);
    iupAttribSet(dlg, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
    ret = IUP_CLOSE;
  }

  free(printer);
  free(file);
  free(paper);
  return ret;
}

static int iPrintDlgButtonCancel_CB(Ihandle* self)
{
  iupAttribSet(IupGetDialog(self), "STATUS", "0");
  return IUP_CLOSE;
}

static int iPrintDlgShow_CB(Ihandle* ih, int state)
{
  if (state == IUP_SHOW)
  {
    Ihandle* filename = IupGetDialogChild(ih, "FILENAME");

    iupAttribSet(ih, "STATUS", "0");

    if (iupStrEqualNoCase(iupAttribGetStr(ih, "ORIENTATION"), "LANDSCAPE"))
      IupSetAttribute(IupGetDialogChild(ih, "ORIENTATION"), "VALUE", "2");
    else
      IupSetAttribute(IupGetDialogChild(ih, "ORIENTATION"), "VALUE", "1");

    if (!IupGetAttribute(filename, "VALUE") || !*IupGetAttribute(filename, "VALUE"))
    {
      char* title = iupStrReturnStrf("%s.pdf", iupPrintDlgGetJobName(ih));
      char* file = iupStrFileMakeFileName(iupdrvGetCurrentDirectory(), title);
      IupSetStrAttribute(filename, "VALUE", file ? file : title);
      free(file);
    }

    iPrintDlgUpdatePrinter(ih);
  }
  return IUP_DEFAULT;
}

static Ihandle* iPrintDlgLabel(const char* title)
{
  return IupSetAttributes(IupLabel(title), "ALIGNMENT=ARIGHT:ACENTER, EXPAND=HORIZONTAL");
}

static int iPrintDlgCreateMethod(Ihandle* ih, void** params)
{
  Ihandle *print_bt, *cancel_bt, *buttons, *printer_list, *filename, *file_bt, *paper_list, *orient_list;
  Ihandle *all_tg, *range_tg, *pages, *copies, *grid;

  print_bt = IupButton("_@IUP_PRINT");
  IupSetStrAttribute(print_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(print_bt, "ACTION", (Icallback)iPrintDlgButtonPrint_CB);
  IupSetAttributeHandle(ih, "DEFAULTENTER", print_bt);

  cancel_bt = IupButton("_@IUP_CANCEL");
  IupSetStrAttribute(cancel_bt, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetCallback(cancel_bt, "ACTION", (Icallback)iPrintDlgButtonCancel_CB);
  IupSetAttributeHandle(ih, "DEFAULTESC", cancel_bt);

  printer_list = IupList();
  IupSetAttribute(printer_list, "DROPDOWN", "YES");
  IupSetAttribute(printer_list, "EXPAND", "HORIZONTAL");
  IupSetAttribute(printer_list, "NAME", "PRINTER");
  IupSetCallback(printer_list, "ACTION", (Icallback)iPrintDlgPrinter_CB);

  filename = IupText();
  IupSetAttribute(filename, "EXPAND", "HORIZONTAL");
  IupSetAttribute(filename, "VISIBLECOLUMNS", "25");
  IupSetAttribute(filename, "NAME", "FILENAME");

  file_bt = IupButton("...");
  IupSetAttribute(file_bt, "NAME", "FILEBUT");
  IupSetCallback(file_bt, "ACTION", (Icallback)iPrintDlgButtonFile_CB);

  paper_list = IupList();
  IupSetAttribute(paper_list, "DROPDOWN", "YES");
  IupSetAttribute(paper_list, "EXPAND", "HORIZONTAL");
  IupSetAttribute(paper_list, "NAME", "PAPER");

  orient_list = IupList();
  IupSetAttribute(orient_list, "DROPDOWN", "YES");
  IupSetAttribute(orient_list, "EXPAND", "HORIZONTAL");
  IupSetAttribute(orient_list, "NAME", "ORIENTATION");
  IupSetStrAttribute(orient_list, "1", "_@IUP_PORTRAIT");
  IupSetStrAttribute(orient_list, "2", "_@IUP_LANDSCAPE");
  IupSetAttribute(orient_list, "VALUE", "1");

  all_tg = IupToggle("_@IUP_ALLPAGES");
  range_tg = IupToggle("_@IUP_PAGERANGE");
  IupSetAttribute(range_tg, "NAME", "RANGEPAGES");
  IupSetCallback(range_tg, "ACTION", (Icallback)iPrintDlgPages_CB);

  pages = IupText();
  IupSetAttribute(pages, "EXPAND", "HORIZONTAL");
  IupSetAttribute(pages, "NAME", "PAGES");
  IupSetAttribute(pages, "ACTIVE", "NO");

  copies = IupText();
  IupSetAttribute(copies, "SPIN", "YES");
  IupSetAttribute(copies, "SPINMIN", "1");
  IupSetAttribute(copies, "SPINMAX", "999");
  IupSetAttribute(copies, "SPINVALUE", "1");
  IupSetAttribute(copies, "VISIBLECOLUMNS", "4");
  IupSetAttribute(copies, "NAME", "COPIES");

  iprintdlg_driver->FillPrinters(printer_list);
  IupSetStrAttributeId(printer_list, "", IupGetInt(printer_list, "COUNT") + 1, "_@IUP_PRINTTOFILE");
  if (IupGetInt(printer_list, "VALUE") <= 0)
    IupSetInt(printer_list, "VALUE", IupGetInt(printer_list, "COUNT"));
  iprintdlg_driver->FillPapers(paper_list, iPrintDlgGetPrinter(printer_list));

  grid = IupGridBox(
    iPrintDlgLabel("_@IUP_PRINTER"), printer_list,
    iPrintDlgLabel("_@IUP_FILE"), IupSetAttributes(IupHbox(filename, file_bt, NULL), "MARGIN=0x0, GAP=5, ALIGNMENT=ACENTER"),
    iPrintDlgLabel("_@IUP_PAPERSIZE"), paper_list,
    iPrintDlgLabel("_@IUP_ORIENTATION"), orient_list,
    iPrintDlgLabel("_@IUP_PAGES"), IupRadio(IupSetAttributes(IupHbox(all_tg, range_tg, pages, NULL), "MARGIN=0x0, GAP=10, ALIGNMENT=ACENTER")),
    iPrintDlgLabel("_@IUP_COPIES"), copies,
    NULL);
  IupSetAttribute(grid, "NUMDIV", "2");
  IupSetAttribute(grid, "ORIENTATION", "HORIZONTAL");
  IupSetAttribute(grid, "SIZECOL", "-1");
  IupSetAttribute(grid, "SIZELIN", "-1");
  IupSetAttribute(grid, "ALIGNMENTLIN", "ACENTER");
  IupSetAttribute(grid, "GAPLIN", "8");
  IupSetAttribute(grid, "GAPCOL", "10");
  IupSetAttribute(grid, "MARGIN", "0x0");

  if (iupDialogButtonOrder() == IUP_BUTTON_ORDER_CANCEL_FIRST)
    buttons = IupHbox(IupFill(), cancel_bt, print_bt, NULL);
  else
    buttons = IupHbox(IupFill(), print_bt, cancel_bt, NULL);
  IupSetAttribute(buttons, "GAP", "5");
  IupSetAttribute(buttons, "MARGIN", "0x0");
  IupSetAttribute(buttons, "NORMALIZESIZE", "HORIZONTAL");

  iupChildTreeAppend(ih, IupSetAttributes(IupVbox(grid,
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"),
    buttons,
    NULL), "MARGIN=10x10, GAP=10"));

  IupSetStrAttribute(ih, "TITLE", "_@IUP_PRINTDLG");
  IupSetAttribute(ih, "RESIZE", "NO");
  IupSetAttribute(ih, "MINBOX", "NO");
  IupSetAttribute(ih, "MAXBOX", "NO");
  IupSetCallback(ih, "SHOW_CB", (Icallback)iPrintDlgShow_CB);

  (void)params;
  return IUP_NOERROR;
}

IUP_SDK_API void iupPrintDlgCustomInitClass(Iclass* ic, const IprintDlgDriver* driver)
{
  iprintdlg_driver = driver;
  ic->Create = iPrintDlgCreateMethod;
}

static int iPrintDlgPopupNotSupported(Ihandle* ih, int x, int y)
{
  (void)x;
  (void)y;
  iupAttribSet(ih, "STATUS", "-1");
  return IUP_ERROR;
}

Iclass* iupPrintDlgNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("dialog"));

  ic->name = "printdlg";
  ic->cons = "PrintDlg";
  ic->nativetype = IUP_TYPEDIALOG;
  ic->is_interactive = 1;

  ic->New = iupPrintDlgNewClass;
  ic->Destroy = iPrintDlgDestroyMethod;

  iupdrvPrintDlgInitClass(ic);

  if (!ic->Create)
  {
    ic->parent->LayoutUpdate = NULL;
    ic->parent->SetChildrenPosition = NULL;
    ic->parent->Map = NULL;
    ic->parent->UnMap = NULL;

    if (!ic->DlgPopup)
      ic->DlgPopup = iPrintDlgPopupNotSupported;
  }

  iupClassRegisterCallback(ic, "PAGINATE_CB", "ii");
  iupClassRegisterCallback(ic, "PRINTPAGE_CB", "iii");

  iupClassRegisterAttribute(ic, "ELEMENT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "JOBNAME", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ORIENTATION", NULL, NULL, IUPAF_SAMEASSYSTEM, "PORTRAIT", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGECOUNT", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PAGESIZE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);
  iupClassRegisterAttribute(ic, "STATUS", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT|IUPAF_READONLY);

  iupDrawRegisterAttributes(ic);

  return ic;
}
