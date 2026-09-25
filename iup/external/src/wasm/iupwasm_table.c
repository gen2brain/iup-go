/** \file
 * \brief WebAssembly Table
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_table.h"
#include "iup_key.h"

#include "iupwasm_drv.h"


EM_JS(int, iupwasmJsTableCreate, (void), {
  if (!globalThis.__iup) globalThis.__iup = { els: {}, next: 1 };
  var id = globalThis.__iup.next++;
  globalThis.__iupApply({ op: 'tablecreate', id: id });
  return id;
})

EM_JS(void, iupwasmJsTableSetGrid, (int id, int show), {
  globalThis.__iupApply({ op: 'tablegrid', id: id, show: show });
})

EM_JS(void, iupwasmJsTableBuild, (int id, int numlin, int numcol), {
  globalThis.__iupApply({ op: 'tablebuild', id: id, numlin: numlin, numcol: numcol });
})

EM_JS(void, iupwasmJsTableFeatures, (int id, int reorder, int resize, int dragdrop), {
  globalThis.__iupApply({ op: 'tablefeatures', id: id, reorder: reorder, resize: resize, dragdrop: dragdrop });
})

EM_JS(void, iupwasmJsTableMoveRow, (int id, int from, int to), {
  globalThis.__iupApply({ op: 'tablemoverow', id: id, from: from, to: to });
})

EM_JS(void, iupwasmJsTableReorderCols, (int id, int from, int to), {
  globalThis.__iupApply({ op: 'tablereordercols', id: id, from: from, to: to });
})

EM_JS(void, iupwasmJsTableEditOpen, (int id, int lin, int col, const char* text), {
  globalThis.__iupApply({ op: 'tableeditopen', id: id, lin: lin, col: col, text: UTF8ToString(text) });
})

EM_JS(void, iupwasmJsTableColTitle, (int id, int col, const char* title), {
  globalThis.__iupApply({ op: 'tablecoltitle', id: id, col: col, title: UTF8ToString(title) });
})

EM_JS(void, iupwasmJsTableColWidth, (int id, int col, int w), {
  globalThis.__iupApply({ op: 'tablecolwidth', id: id, col: col, w: w });
})

EM_JS(int, iupwasmJsTableColWidthGet, (int id, int col), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'tablecolwidthget', id: id, col: col });
  var wrap = globalThis.__iup.els[id]; if (!wrap) return 0;
  var th = wrap.__iupHead.children[col - 1];
  return th ? th.offsetWidth : 0;
})

EM_JS(void, iupwasmJsTableRender, (int id, int lin, int col), {
  globalThis.__iupApply({ op: 'tablerender', id: id, lin: lin, col: col });
})

EM_JS(void, iupwasmJsTableSetCell, (int id, int lin, int col, const char* value), {
  globalThis.__iupApply({ op: 'tablesetcell', id: id, lin: lin, col: col, value: UTF8ToString(value) });
})

EM_JS(int, iupwasmJsTableGetCell, (int id, int lin, int col), {
  var s;
  if (typeof document === 'undefined') s = globalThis.__iupReadSync({ op: 'tablegetcell', id: id, lin: lin, col: col });
  else { var wrap = globalThis.__iup.els[id]; s = ""; if (wrap) { var tr = wrap.__iupBody.children[lin - 1]; if (tr) { var td = tr.children[col - 1]; if (td) s = td.__iupText || ""; } } }
  var len = lengthBytesUTF8(s) + 1;
  var ptr = _malloc(len);
  stringToUTF8(s, ptr, len);
  return ptr;
})

EM_JS(void, iupwasmJsTableSetFitImage, (int id, int fit, int maxH), {
  globalThis.__iupApply({ op: 'tablefitimage', id: id, fit: fit, maxH: maxH });
})

EM_JS(void, iupwasmJsTableSetCellImage, (int id, int lin, int col, int imgId), {
  globalThis.__iupApply({ op: 'tablecellimage', id: id, lin: lin, col: col, imgId: imgId });
})

EM_JS(void, iupwasmJsTableStripe, (int id, int alt, const char* even, const char* odd), {
  globalThis.__iupApply({ op: 'tablestripe', id: id, alt: alt, even: UTF8ToString(even), odd: UTF8ToString(odd) });
})

EM_JS(void, iupwasmJsTableColAlign, (int id, int col, const char* css), {
  globalThis.__iupApply({ op: 'tablecolalign', id: id, col: col, css: UTF8ToString(css) });
})

EM_JS(void, iupwasmJsTableCellColor, (int id, int lin, int col, const char* bg, const char* fg), {
  globalThis.__iupApply({ op: 'tablecellcolor', id: id, lin: lin, col: col, bg: UTF8ToString(bg), fg: UTF8ToString(fg) });
})

EM_JS(int, iupwasmJsTableVScrollTop, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'tablevscrolltop', id: id });
  var w = globalThis.__iup.els[id]; return w ? (w.scrollTop | 0) : 0;
})

EM_JS(int, iupwasmJsTableVClientH, (int id), {
  if (typeof document === 'undefined') return globalThis.__iupReadSync({ op: 'tablevclienth', id: id });
  var w = globalThis.__iup.els[id]; return w ? (w.clientHeight | 0) : 0;
})

EM_JS(void, iupwasmJsTableVirtualInit, (int id), {
  globalThis.__iupApply({ op: 'tablevirtualinit', id: id });
})

EM_JS(void, iupwasmJsTableVWindow, (int id, int count, int numCol, int top, int bot), {
  globalThis.__iupApply({ op: 'tablevwindow', id: id, count: count, numCol: numCol, top: top, bot: bot });
})

EM_JS(void, iupwasmJsTableVCell, (int id, int rowIdx, int col, int lin, const char* str, int imgId, const char* bg, const char* fg), {
  globalThis.__iupApply({ op: 'tablevcell', id: id, rowIdx: rowIdx, col: col, lin: lin, str: UTF8ToString(str), imgId: imgId, bg: UTF8ToString(bg), fg: UTF8ToString(fg) });
})

EM_JS(void, iupwasmJsTableVStripe, (int id, int rowIdx, int lin), {
  globalThis.__iupApply({ op: 'tablevstripe', id: id, rowIdx: rowIdx, lin: lin });
})

EM_JS(void, iupwasmJsTableFocus, (int id, int lin, int col, int select, int focusrect), {
  globalThis.__iupApply({ op: 'tablefocus', id: id, lin: lin, col: col, select: select, focusrect: focusrect });
})

EM_JS(void, iupwasmJsTableSelect, (int id, int lin, int select), {
  globalThis.__iupApply({ op: 'tableselect', id: id, lin: lin, select: select });
})

EM_JS(void, iupwasmJsTableScrollTo, (int id, int lin, int col), {
  globalThis.__iupApply({ op: 'tablescrollto', id: id, lin: lin, col: col });
})

static void wasmTableApplyColors(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int alt = iupAttribGetBoolean(ih, "ALTERNATECOLOR");
  char* even = iupAttribGetStr(ih, "EVENROWCOLOR");
  char* odd = iupAttribGetStr(ih, "ODDROWCOLOR");
  if (id)
    iupwasmJsTableStripe(id, alt, even ? even : "#ffffff", odd ? odd : "#f0f0f0");
}

static const char* wasmTableAlignCss(const char* a)
{
  if (a && (iupStrEqualNoCase(a, "ARIGHT") || iupStrEqualNoCase(a, "RIGHT"))) return "right";
  if (a && (iupStrEqualNoCase(a, "ACENTER") || iupStrEqualNoCase(a, "CENTER"))) return "center";
  return "left";
}

static void wasmTableApplyAlign(Ihandle* ih)
{
  int id = iupwasmIdOf(ih), col;
  if (!id) return;
  for (col = 1; col <= ih->data->num_col; col++)
  {
    char name[16];
    snprintf(name, sizeof(name), "ALIGNMENT%d", col);
    iupwasmJsTableColAlign(id, col, wasmTableAlignCss(iupAttribGet(ih, name)));
  }
}

static int wasmTableCellCss(Ihandle* ih, int lin, int col, char* bgcss, char* fgcss)
{
  unsigned char r, g, b;
  char* bg = iupAttribGetId2(ih, "BGCOLOR", lin, col);
  char* fg = iupAttribGetId2(ih, "FGCOLOR", lin, col);
  if (!bg) bg = iupAttribGetId2(ih, "BGCOLOR", 0, col);
  if (!bg) bg = iupAttribGetId2(ih, "BGCOLOR", lin, 0);
  if (!fg) fg = iupAttribGetId2(ih, "FGCOLOR", 0, col);
  if (!fg) fg = iupAttribGetId2(ih, "FGCOLOR", lin, 0);
  bgcss[0] = 0;
  fgcss[0] = 0;
  if (bg && iupStrToRGB(bg, &r, &g, &b))
    snprintf(bgcss, 20, "rgb(%d,%d,%d)", r, g, b);
  if (fg && iupStrToRGB(fg, &r, &g, &b))
    snprintf(fgcss, 20, "rgb(%d,%d,%d)", r, g, b);
  return bgcss[0] || fgcss[0];
}

static void wasmTableApplyCellColors(Ihandle* ih)
{
  int id = iupwasmIdOf(ih), lin, col;
  char bgcss[20], fgcss[20];
  if (!id || iupAttribGetBoolean(ih, "VIRTUALMODE")) return;
  for (lin = 1; lin <= ih->data->num_lin; lin++)
    for (col = 1; col <= ih->data->num_col; col++)
    {
      if (wasmTableCellCss(ih, lin, col, bgcss, fgcss))
        iupwasmJsTableCellColor(id, lin, col, bgcss, fgcss);
    }
}

static void wasmTableVirtualRender(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  int rowH, numLin, numCol, first, count, i, c, showImage;
  char bgcss[20], fgcss[20];
  sIFnii value_cb, image_cb;

  if (!id || !iupAttribGetBoolean(ih, "VIRTUALMODE"))
    return;

  rowH = iupdrvTableGetRowHeight(ih);
  numLin = ih->data->num_lin;
  numCol = ih->data->num_col;

  first = iupwasmJsTableVScrollTop(id) / rowH - 4;
  if (first < 0) first = 0;
  count = iupwasmJsTableVClientH(id) / rowH + 9;
  if (first + count > numLin) count = numLin - first;
  if (count < 0) count = 0;

  iupwasmJsTableVWindow(id, count, numCol, first * rowH, (numLin - first - count) * rowH);

  value_cb = (sIFnii)IupGetCallback(ih, "VALUE_CB");
  image_cb = (sIFnii)IupGetCallback(ih, "IMAGE_CB");
  showImage = ih->data->show_image;
  for (i = 0; i < count; i++)
  {
    int lin = first + i + 1;
    for (c = 1; c <= numCol; c++)
    {
      char* v;
      int imgId = 0;
      if (showImage && image_cb)
      {
        char* name = image_cb(ih, lin, c);
        if (name && name[0])
          imgId = (int)(intptr_t)iupImageGetImage(name, ih, 0, NULL);
      }
      wasmTableCellCss(ih, lin, c, bgcss, fgcss);
      v = value_cb ? value_cb(ih, lin, c) : NULL;  /* must be the last string dispatch before use: result lives in a single recycled slot */
      iupwasmJsTableVCell(id, i, c - 1, lin, v ? v : "", imgId, bgcss, fgcss);
    }
    iupwasmJsTableVStripe(id, i, lin);
  }
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableVScroll(int id)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  if (ih)
    wasmTableVirtualRender(ih);
}

static void wasmTableSortIndicator(Ihandle* ih, int col, int ascending)
{
  int id = iupwasmIdOf(ih);
  int c;

  for (c = 1; c <= ih->data->num_col; c++)
  {
    char* title = iupAttribGetId(ih, "_IUPWASM_COLTITLE", c);
    if (c == col)
    {
      char buf[256];
      snprintf(buf, sizeof(buf), "%s %s", title ? title : "", ascending ? "\xE2\x96\xB2" : "\xE2\x96\xBC");
      iupwasmJsTableColTitle(id, c, buf);
    }
    else
      iupwasmJsTableColTitle(id, c, title ? title : "");
  }
}

/* moves rows the same way a row drag-drop does, so the per-line attributes follow them */
static void wasmTableSortRows(Ihandle* ih, int col, int ascending)
{
  int id = iupwasmIdOf(ih);
  int n = ih->data->num_lin;
  char** values;
  int* order, *pos, *at;
  int i, j;

  if (n < 2)
    return;

  values = (char**)malloc(n * sizeof(char*));
  order = (int*)malloc(n * sizeof(int));
  pos = (int*)malloc(n * sizeof(int));
  at = (int*)malloc(n * sizeof(int));

  for (i = 0; i < n; i++)
  {
    char* v = iupdrvTableGetCellValue(ih, i + 1, col);
    values[i] = iupStrDup(v ? v : "");
    order[i] = i;
    pos[i] = i;
    at[i] = i;
  }

  for (i = 1; i < n; i++)
  {
    int cur = order[i];
    for (j = i - 1; j >= 0; j--)
    {
      int cmp = iupStrCompare(values[order[j]], values[cur], 0, 1);
      if (!ascending)
        cmp = -cmp;
      if (cmp <= 0)
        break;
      order[j + 1] = order[j];
    }
    order[j + 1] = cur;
  }

  for (i = 0; i < n; i++)
  {
    int want = order[i];
    int from = pos[want];
    if (from == i)
      continue;

    iupwasmJsTableMoveRow(id, from + 1, i + 1);

    for (j = from; j > i; j--)
    {
      at[j] = at[j - 1];
      pos[at[j]] = j;
    }
    at[i] = want;
    pos[want] = i;
  }

  {
    int sel_count = 0, focus = iupAttribGetInt(ih, "_IUPWASM_FOCUSLIN");
    int* selected = iupdrvTableGetSelectedLins(ih, &sel_count);

    for (i = 0; i < n; i++)
    {
      pos[order[i]] = i + 1;
      order[i]++;
    }

    for (i = 0; i < sel_count; i++)
      iupAttribSetId(ih, "_IUPWASM_TABLESEL", selected[i], NULL);
    iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", 0);
    iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", 0);
    for (i = 0; i < sel_count; i++)
    {
      int lin = pos[selected[i] - 1];
      iupAttribSetId(ih, "_IUPWASM_TABLESEL", lin, "1");
      if (iupAttribGetInt(ih, "_IUPWASM_TABLESELFIRST") < 1 || lin < iupAttribGetInt(ih, "_IUPWASM_TABLESELFIRST"))
        iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", lin);
      if (lin > iupAttribGetInt(ih, "_IUPWASM_TABLESELLAST"))
        iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", lin);
    }
    if (selected)
      free(selected);

    if (focus > 0 && focus <= n)
      iupAttribSetInt(ih, "_IUPWASM_FOCUSLIN", pos[focus - 1]);

    iupTableSortLinAttribs(ih, order);
  }

  for (i = 0; i < n; i++)
    free(values[i]);
  free(values);
  free(order);
  free(pos);
  free(at);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableHeaderClick(int id, int col)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFni sort_cb;
  int ascending;

  if (!ih || !IupGetInt(ih, "SORTABLE"))
    return;

  if (iupAttribGetInt(ih, "_IUPWASM_SORTCOL") == col)
    ascending = !iupAttribGetInt(ih, "_IUPWASM_SORTASC");
  else
    ascending = 1;

  sort_cb = (IFni)IupGetCallback(ih, "SORT_CB");
  if (sort_cb && sort_cb(ih, col) == IUP_IGNORE)
    return;

  iupAttribSetInt(ih, "_IUPWASM_SORTCOL", col);
  iupAttribSetInt(ih, "_IUPWASM_SORTASC", ascending);
  wasmTableSortIndicator(ih, col, ascending);

  if (!iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    wasmTableSortRows(ih, col, ascending);
    wasmTableApplyCellColors(ih);
  }
}

static void wasmTableUpdateFocus(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih);
  int select = !iupStrEqualNoCase(IupGetAttribute(ih, "SELECTIONMODE"), "NONE");
  int focusrect = iupAttribGetBoolean(ih, "FOCUSRECT");
  if (id && !iupAttribGetBoolean(ih, "VIRTUALMODE"))  /* virtual rows aren't lin-indexed in the DOM */
    iupwasmJsTableFocus(id, lin, col, select, focusrect);
}

/* the selected rows are kept as per-row attributes, the bounds keep a virtual table off a full scan */
static void wasmTableSelectRow(Ihandle* ih, int lin, int select)
{
  int id = iupwasmIdOf(ih);

  if (lin < 1 || lin > ih->data->num_lin)
    return;

  iupAttribSetId(ih, "_IUPWASM_TABLESEL", lin, select ? "1" : NULL);

  if (select)
  {
    int first = iupAttribGetInt(ih, "_IUPWASM_TABLESELFIRST");
    int last = iupAttribGetInt(ih, "_IUPWASM_TABLESELLAST");

    if (first < 1 || lin < first)
      iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", lin);
    if (lin > last)
      iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", lin);
  }

  if (id && !iupAttribGetBoolean(ih, "VIRTUALMODE"))
    iupwasmJsTableSelect(id, lin, select);
}

static void wasmTableSelectedRange(Ihandle* ih, int* first, int* last)
{
  *first = iupAttribGetInt(ih, "_IUPWASM_TABLESELFIRST");
  *last = iupAttribGetInt(ih, "_IUPWASM_TABLESELLAST");

  if (*first < 1)
    *first = 1;
  if (*last > ih->data->num_lin)
    *last = ih->data->num_lin;
}

static void wasmTableSelectOnly(Ihandle* ih, int lin)
{
  int l, first, last;

  wasmTableSelectedRange(ih, &first, &last);

  for (l = first; l <= last; l++)
  {
    if (l != lin && iupAttribGetId(ih, "_IUPWASM_TABLESEL", l))
      wasmTableSelectRow(ih, l, 0);
  }

  iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", 0);
  iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", 0);

  wasmTableSelectRow(ih, lin, 1);
}

static void wasmTableSetFocus(Ihandle* ih, int lin, int col)
{
  iupAttribSetInt(ih, "_IUPWASM_FOCUSLIN", lin);
  iupAttribSetInt(ih, "_IUPWASM_FOCUSCOL", col);
  wasmTableUpdateFocus(ih, lin, col);
}

static void wasmTableRebuild(Ihandle* ih, int columns)
{
  int id = iupwasmIdOf(ih);
  if (!id) return;
  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    if (columns)
      iupwasmJsTableBuild(id, 0, ih->data->num_col);
    wasmTableVirtualRender(ih);
    return;
  }
  iupwasmJsTableBuild(id, ih->data->num_lin, ih->data->num_col);
  wasmTableApplyColors(ih);
  wasmTableApplyAlign(ih);
  wasmTableApplyCellColors(ih);
}

static void wasmTableShiftColTitles(Ihandle* ih, int from, int to)
{
  int c, step = (from < to) ? 1 : -1;
  char* title = iupStrDup(iupAttribGetId(ih, "_IUPWASM_COLTITLE", from));
  for (c = from; c != to; c += step)
    iupAttribSetStrId(ih, "_IUPWASM_COLTITLE", c, iupAttribGetId(ih, "_IUPWASM_COLTITLE", c + step));
  iupAttribSetStrId(ih, "_IUPWASM_COLTITLE", to, title);
  if (title)
    free(title);
}

static int wasmTableShiftPos(int cur, int pos, int delta)
{
  return (cur >= pos && !(delta < 0 && cur == pos)) ? cur + delta : cur;
}

static void wasmTableFollowSel(Ihandle* ih, int pos, int delta, int num_lin)
{
  int count, i;
  int* lins = iupdrvTableGetSelectedLins(ih, &count);

  for (i = 0; i < count; i++)
    iupAttribSetId(ih, "_IUPWASM_TABLESEL", lins[i], NULL);
  iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", 0);
  iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", 0);

  for (i = 0; i < count; i++)
  {
    int lin = lins[i];
    if (delta < 0 && lin == pos)
      continue;
    lin = wasmTableShiftPos(lin, pos, delta);
    if (lin > num_lin)
      continue;
    iupAttribSetId(ih, "_IUPWASM_TABLESEL", lin, "1");
    if (iupAttribGetInt(ih, "_IUPWASM_TABLESELFIRST") < 1)
      iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", lin);
    iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", lin);
  }

  if (lins)
    free(lins);
}

static void wasmTableFollowFocus(Ihandle* ih, int lin_pos, int lin_delta, int col_pos, int col_delta)
{
  int lin = iupAttribGetInt(ih, "_IUPWASM_FOCUSLIN");
  int col = iupAttribGetInt(ih, "_IUPWASM_FOCUSCOL");
  int new_lin, new_col;

  if (lin < 1 || col < 1)
    return;

  new_lin = wasmTableShiftPos(lin, lin_pos, lin_delta);
  new_col = wasmTableShiftPos(col, col_pos, col_delta);
  if (new_lin > ih->data->num_lin)
    new_lin = ih->data->num_lin;
  if (new_col > ih->data->num_col)
    new_col = ih->data->num_col;

  if (new_lin < 1 || new_col < 1)
  {
    iupAttribSet(ih, "_IUPWASM_FOCUSLIN", NULL);
    iupAttribSet(ih, "_IUPWASM_FOCUSCOL", NULL);
    return;
  }

  if ((lin_delta < 0 && lin == lin_pos) || (col_delta < 0 && col == col_pos) ||
      new_lin != wasmTableShiftPos(lin, lin_pos, lin_delta) || new_col != wasmTableShiftPos(col, col_pos, col_delta))
    wasmTableSetFocus(ih, new_lin, new_col);
  else
  {
    iupAttribSetInt(ih, "_IUPWASM_FOCUSLIN", new_lin);
    iupAttribSetInt(ih, "_IUPWASM_FOCUSCOL", new_col);
  }
}

IUP_SDK_API void iupdrvTableSetNumLin(Ihandle* ih, int num_lin)
{
  wasmTableFollowSel(ih, num_lin + 1, 0, num_lin);
  ih->data->num_lin = num_lin;
  wasmTableRebuild(ih, 0);
  wasmTableFollowFocus(ih, num_lin + 1, 0, ih->data->num_col + 1, 0);
}

IUP_SDK_API void iupdrvTableSetNumCol(Ihandle* ih, int num_col)
{
  int c;
  for (c = num_col + 1; c <= ih->data->num_col; c++)
    iupAttribSetId(ih, "_IUPWASM_COLTITLE", c, NULL);
  ih->data->num_col = num_col;
  wasmTableRebuild(ih, 1);
  wasmTableFollowFocus(ih, ih->data->num_lin + 1, 0, num_col + 1, 0);
}

IUP_SDK_API void iupdrvTableAddLin(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  wasmTableFollowSel(ih, pos, 1, ih->data->num_lin + 1);
  ih->data->num_lin++;
  wasmTableRebuild(ih, 0);
  if (id && !iupAttribGetBoolean(ih, "VIRTUALMODE") && pos < ih->data->num_lin)
  {
    iupwasmJsTableMoveRow(id, ih->data->num_lin, pos);
    wasmTableApplyCellColors(ih);
  }
  wasmTableFollowFocus(ih, pos, 1, ih->data->num_col + 1, 0);
}

IUP_SDK_API void iupdrvTableDelLin(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  wasmTableFollowSel(ih, pos, -1, ih->data->num_lin - 1);
  if (id && !iupAttribGetBoolean(ih, "VIRTUALMODE") && pos < ih->data->num_lin)
    iupwasmJsTableMoveRow(id, pos, ih->data->num_lin);
  ih->data->num_lin--;
  wasmTableRebuild(ih, 0);
  wasmTableFollowFocus(ih, pos, -1, ih->data->num_col + 1, 0);
}

IUP_SDK_API void iupdrvTableAddCol(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  ih->data->num_col++;
  wasmTableRebuild(ih, 1);
  if (pos < ih->data->num_col)
  {
    wasmTableShiftColTitles(ih, ih->data->num_col, pos);
    if (id)
      iupwasmJsTableReorderCols(id, ih->data->num_col, pos);
    wasmTableApplyAlign(ih);
  }
  wasmTableFollowFocus(ih, ih->data->num_lin + 1, 0, pos, 1);
}

IUP_SDK_API void iupdrvTableDelCol(Ihandle* ih, int pos)
{
  int id = iupwasmIdOf(ih);
  if (pos < ih->data->num_col)
  {
    wasmTableShiftColTitles(ih, pos, ih->data->num_col);
    if (id)
      iupwasmJsTableReorderCols(id, pos, ih->data->num_col);
  }
  iupAttribSetId(ih, "_IUPWASM_COLTITLE", ih->data->num_col, NULL);
  ih->data->num_col--;
  wasmTableRebuild(ih, 1);
  wasmTableFollowFocus(ih, ih->data->num_lin + 1, 0, pos, -1);
}

IUP_SDK_API void iupdrvTableSetCellValue(Ihandle* ih, int lin, int col, const char* value)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableSetCell(id, lin, col, value ? value : "");
}

IUP_SDK_API char* iupdrvTableGetCellValue(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih);
  char* ptr;
  char* ret;
  if (!id)
    return NULL;
  ptr = (char*)(intptr_t)iupwasmJsTableGetCell(id, lin, col);
  if (!ptr)
    return NULL;
  ret = iupStrReturnStr(ptr);
  free(ptr);
  return ret;
}

IUP_SDK_API void iupdrvTableSetCellImage(Ihandle* ih, int lin, int col, const char* image)
{
  int id = iupwasmIdOf(ih);
  void* handle;
  if (!id)
    return;
  handle = iupImageGetImage(image, ih, 0, NULL);
  if (handle)
    iupwasmJsTableSetCellImage(id, lin, col, (int)(intptr_t)handle);
}

/* core's TITLE setter returns 0, so attribute replay drops TITLE<col> from the hash: keep our own */
IUP_SDK_API void iupdrvTableSetColTitle(Ihandle* ih, int col, const char* title)
{
  int id = iupwasmIdOf(ih);
  iupAttribSetStrId(ih, "_IUPWASM_COLTITLE", col, title);
  if (id)
    iupwasmJsTableColTitle(id, col, title ? title : "");
}

IUP_SDK_API char* iupdrvTableGetColTitle(Ihandle* ih, int col)
{
  return iupAttribGetId(ih, "_IUPWASM_COLTITLE", col);
}

IUP_SDK_API void iupdrvTableSetSortSign(Ihandle* ih, int col, int sign)
{
  if (col < 1 || col > ih->data->num_col)
    return;

  iupAttribSetInt(ih, "_IUPWASM_SORTCOL", sign ? col : 0);
  iupAttribSetInt(ih, "_IUPWASM_SORTASC", sign > 0);

  wasmTableSortIndicator(ih, sign ? col : 0, sign > 0);
}

IUP_SDK_API int iupdrvTableGetSortSign(Ihandle* ih, int col)
{
  if (iupAttribGetInt(ih, "_IUPWASM_SORTCOL") != col)
    return 0;

  return iupAttribGetInt(ih, "_IUPWASM_SORTASC") ? 1 : -1;
}

IUP_SDK_API void iupdrvTableSetColWidth(Ihandle* ih, int col, int width)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableColWidth(id, col, width);
}

IUP_SDK_API int iupdrvTableGetColWidth(Ihandle* ih, int col)
{
  int id = iupwasmIdOf(ih);
  return id ? iupwasmJsTableColWidthGet(id, col) : 0;
}

IUP_SDK_API void iupdrvTableSetFocusCell(Ihandle* ih, int lin, int col)
{
  wasmTableSetFocus(ih, lin, col);

  if (!iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTIONMODE"), "NONE"))
  {
    wasmTableSelectOnly(ih, lin);
    iupAttribSetInt(ih, "_IUPWASM_TABLEANCHOR", lin);
  }
}

IUP_SDK_API int iupdrvTableIsLinSelected(Ihandle* ih, int lin)
{
  return iupAttribGetId(ih, "_IUPWASM_TABLESEL", lin) ? 1 : 0;
}

IUP_SDK_API void iupdrvTableSelectLin(Ihandle* ih, int lin, int select)
{
  wasmTableSelectRow(ih, lin, select);
}

IUP_SDK_API int* iupdrvTableGetSelectedLins(Ihandle* ih, int* count)
{
  int* lins;
  int lin, first, last, i = 0;

  *count = 0;

  if (ih->data->num_lin <= 0)
    return NULL;

  wasmTableSelectedRange(ih, &first, &last);
  if (last < first)
    return NULL;

  lins = (int*)malloc(sizeof(int) * (last - first + 1));

  for (lin = first; lin <= last; lin++)
  {
    if (iupAttribGetId(ih, "_IUPWASM_TABLESEL", lin))
      lins[i++] = lin;
  }

  if (i == 0)
  {
    free(lins);
    return NULL;
  }

  *count = i;
  return lins;
}

IUP_SDK_API void iupdrvTableGetFocusCell(Ihandle* ih, int* lin, int* col)
{
  if (lin) *lin = iupAttribGetInt(ih, "_IUPWASM_FOCUSLIN");
  if (col) *col = iupAttribGetInt(ih, "_IUPWASM_FOCUSCOL");
  if (lin && *lin == 0) *lin = 1;
  if (col && *col == 0) *col = 1;
}

IUP_SDK_API void iupdrvTableScrollToCell(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableScrollTo(id, lin, col);
}

IUP_SDK_API void iupdrvTableRedraw(Ihandle* ih)
{
  wasmTableApplyColors(ih);
  wasmTableApplyCellColors(ih);
  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
    wasmTableVirtualRender(ih);
}

IUP_SDK_API void iupdrvTableUpdateCellStyle(Ihandle* ih, int lin, int col)
{
  int id = iupwasmIdOf(ih), l, c;
  char bgcss[20], fgcss[20];

  if (!id)
    return;

  if ((lin < 1 && col < 1) || iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    iupdrvTableRedraw(ih);
    return;
  }

  for (l = (lin > 0 ? lin : 1); l <= (lin > 0 ? lin : ih->data->num_lin); l++)
    for (c = (col > 0 ? col : 1); c <= (col > 0 ? col : ih->data->num_col); c++)
    {
      wasmTableCellCss(ih, l, c, bgcss, fgcss);
      iupwasmJsTableCellColor(id, l, c, bgcss, fgcss);
    }
}

IUP_SDK_API void iupdrvTableSetShowGrid(Ihandle* ih, int show)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsTableSetGrid(id, show);
}

IUP_SDK_API int iupdrvTableGetBorderWidth(Ihandle* ih)
{
  (void)ih;
  return 1;
}

/* must match the rendered cell box: charheight + 4px padding + 1px border */
IUP_SDK_API int iupdrvTableGetRowHeight(Ihandle* ih)
{
  int h = 0;
  iupdrvFontGetCharSize(ih, NULL, &h);
  return h + 5;
}

IUP_SDK_API int iupdrvTableGetHeaderHeight(Ihandle* ih)
{
  int h = 0;
  iupdrvFontGetCharSize(ih, NULL, &h);
  return h + 5;  /* same box as a row */
}

IUP_SDK_API void iupdrvTableAddBorders(Ihandle* ih, int* w, int* h)
{
  (void)ih;
  if (w) *w += 2;
  if (h) *h += 2;
}

static void wasmTableLayoutUpdate(Ihandle* ih)
{
  int id = iupwasmIdOf(ih);
  if (id)
    iupwasmJsSetPos(id, ih->x, ih->y, ih->currentwidth, ih->currentheight);
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableCellClick(int id, int lin, int col, int mods)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniis click_cb;
  IFnii enter_cb;
  if (!ih)
    return;

  wasmTableSetFocus(ih, lin, col);

  {
    char* selmode = iupAttribGetStr(ih, "SELECTIONMODE");

    if (!iupStrEqualNoCase(selmode, "NONE"))
    {
      int anchor = iupAttribGetInt(ih, "_IUPWASM_TABLEANCHOR");

      if (iupStrEqualNoCase(selmode, "MULTIPLE") && (mods & 1) && anchor > 0)
      {
        int from = anchor < lin ? anchor : lin;
        int to = anchor < lin ? lin : anchor;
        int l, first, last;

        wasmTableSelectedRange(ih, &first, &last);

        for (l = first; l <= last; l++)
          wasmTableSelectRow(ih, l, 0);

        iupAttribSetInt(ih, "_IUPWASM_TABLESELFIRST", 0);
        iupAttribSetInt(ih, "_IUPWASM_TABLESELLAST", 0);

        for (l = from; l <= to; l++)
          wasmTableSelectRow(ih, l, 1);
      }
      else if (iupStrEqualNoCase(selmode, "MULTIPLE") && (mods & 2))
      {
        wasmTableSelectRow(ih, lin, !iupdrvTableIsLinSelected(ih, lin));
        iupAttribSetInt(ih, "_IUPWASM_TABLEANCHOR", lin);
      }
      else
      {
        wasmTableSelectOnly(ih, lin);
        iupAttribSetInt(ih, "_IUPWASM_TABLEANCHOR", lin);
      }
    }
  }

  enter_cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (enter_cb)
    enter_cb(ih, lin, col);

  click_cb = (IFniis)IupGetCallback(ih, "CLICK_CB");
  if (click_cb)
  {
    char status[IUPKEY_STATUS_SIZE];
    iupwasmFillStatus(status, mods);
    if (click_cb(ih, lin, col, status) == IUP_CLOSE)
      IupExitLoop();
  }

  iupTableCallMultiSelectionCb(ih);
}

static int wasmTableCellEditable(Ihandle* ih, int col)
{
  return iupAttribGetIntId(ih, "EDITABLE", col) || iupAttribGetBoolean(ih, "EDITABLE");
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableEditBegin(int id, int lin, int col)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnii cb;
  char* text;

  if (!ih)
    return;
  if (!wasmTableCellEditable(ih, col))
    return;
  cb = (IFnii)IupGetCallback(ih, "EDITBEGIN_CB");
  if (cb && cb(ih, lin, col) == IUP_IGNORE)
    return;

  text = iupdrvTableGetCellValue(ih, lin, col);
  iupwasmJsTableEditOpen(id, lin, col, text ? text : "");
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableEditEnd(int id, int lin, int col, const char* text, int apply)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFniisi editend_cb;
  IFniis edit_cb;
  IFnii vc_cb;
  if (!ih)
    return;

  editend_cb = (IFniisi)IupGetCallback(ih, "EDITEND_CB");
  if (editend_cb && editend_cb(ih, lin, col, (char*)text, apply) == IUP_IGNORE)
  {
    iupwasmJsTableRender(id, lin, col);
    return;
  }

  if (!apply)
  {
    iupwasmJsTableRender(id, lin, col);
    return;
  }

  edit_cb = (IFniis)IupGetCallback(ih, "EDITION_CB");
  if (edit_cb && edit_cb(ih, lin, col, (char*)text) == IUP_IGNORE)
  {
    iupwasmJsTableRender(id, lin, col);
    return;
  }

  iupdrvTableSetCellValue(ih, lin, col, text);

  vc_cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
  if (vc_cb)
    vc_cb(ih, lin, col);
}

int iupwasmTableKeyNav(Ihandle* ih, int code)
{
  int id = iupwasmIdOf(ih);
  int lin, col, new_lin, new_col;
  IFnii cb;

  if (!id)
    return 0;

  iupdrvTableGetFocusCell(ih, &lin, &col);
  new_lin = lin;
  new_col = col;

  switch (code)
  {
    case K_UP:    new_lin = lin - 1; break;
    case K_DOWN:  new_lin = lin + 1; break;
    case K_LEFT:  new_col = col - 1; break;
    case K_RIGHT: new_col = col + 1; break;
    case K_HOME:  new_col = 1; break;
    case K_END:   new_col = ih->data->num_col; break;
    case K_PGUP:  new_lin = 1; break;
    case K_PGDN:  new_lin = ih->data->num_lin; break;
    case K_TAB:
      if (col < ih->data->num_col)
        new_col = col + 1;
      else if (lin < ih->data->num_lin)
      {
        new_lin = lin + 1;
        new_col = 1;
      }
      break;
    case K_CR:
    case K_F2:
      if (wasmTableCellEditable(ih, col))
        iupwasmTableEditBegin(id, lin, col);
      return 1;
    case K_cC:
    case iup_XkeyCtrl(K_c):
    {
      char* value = iupdrvTableGetCellValue(ih, lin, col);
      if (value && *value)
        IupStoreGlobal("CLIPBOARD", value);
      return 1;
    }
    case K_cV:
    case iup_XkeyCtrl(K_v):
    {
      char* text = IupGetGlobal("CLIPBOARD");
      if (text && *text && !iupAttribGetBoolean(ih, "VIRTUALMODE") && wasmTableCellEditable(ih, col))
      {
        iupdrvTableSetCellValue(ih, lin, col, text);
        cb = (IFnii)IupGetCallback(ih, "VALUECHANGED_CB");
        if (cb)
          cb(ih, lin, col);
      }
      return 1;
    }
    default:
      return 0;
  }

  if (new_lin < 1) new_lin = 1;
  if (new_lin > ih->data->num_lin) new_lin = ih->data->num_lin;
  if (new_col < 1) new_col = 1;
  if (new_col > ih->data->num_col) new_col = ih->data->num_col;

  if (new_lin == lin && new_col == col)
    return 1;

  iupdrvTableSetFocusCell(ih, new_lin, new_col);
  iupdrvTableScrollToCell(ih, new_lin, new_col);

  cb = (IFnii)IupGetCallback(ih, "ENTERITEM_CB");
  if (cb)
    cb(ih, new_lin, new_col);

  return 1;
}


EMSCRIPTEN_KEEPALIVE void iupwasmTableReorder(int id, int oldCol, int newCol)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  IFnii cb;
  int ret;
  if (!ih || !ih->data->allow_reorder)
    return;
  cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  ret = cb ? cb(ih, oldCol, newCol) : IUP_DEFAULT;
  if (ret != IUP_IGNORE)
  {
    int c, step = (oldCol < newCol) ? 1 : -1;
    char* title = iupStrDup(iupAttribGetId(ih, "_IUPWASM_COLTITLE", oldCol));
    for (c = oldCol; c != newCol; c += step)
      iupAttribSetStrId(ih, "_IUPWASM_COLTITLE", c, iupAttribGetId(ih, "_IUPWASM_COLTITLE", c + step));
    iupAttribSetStrId(ih, "_IUPWASM_COLTITLE", newCol, title);
    if (title)
      free(title);

    iupTableMoveColAttribs(ih, oldCol, newCol);
    iupwasmJsTableReorderCols(id, oldCol, newCol);
    wasmTableApplyAlign(ih);

    if (iupAttribGetInt(ih, "_IUPWASM_SORTCOL") > 0)
    {
      int sort_col = iupTableMoveColPos(iupAttribGetInt(ih, "_IUPWASM_SORTCOL"), oldCol, newCol);
      iupdrvTableSetSortSign(ih, sort_col, iupAttribGetInt(ih, "_IUPWASM_SORTASC") ? 1 : -1);
    }
    if (iupAttribGetInt(ih, "_IUPWASM_FOCUSCOL") > 0)
      iupAttribSetInt(ih, "_IUPWASM_FOCUSCOL", iupTableMoveColPos(iupAttribGetInt(ih, "_IUPWASM_FOCUSCOL"), oldCol, newCol));
  }
  if (ret == IUP_CLOSE)
    IupExitLoop();
}

EMSCRIPTEN_KEEPALIVE void iupwasmTableRowDragDrop(int id, int from, int before)
{
  Ihandle* ih = iupwasmHandleFromId(id);
  int is_ctrl = 0;
  int to;

  if (!ih || !ih->data->show_dragdrop)
    return;

  if (iupTableCallDragDropCb(ih, from - 1, before - 1, &is_ctrl) != IUP_CONTINUE)
    return;

  to = (before > from) ? before - 1 : before;
  if (to > ih->data->num_lin) to = ih->data->num_lin;
  if (to < 1) to = 1;

  iupTableMoveLinAttribs(ih, from, to);
  iupwasmJsTableMoveRow(id, from, to);
  wasmTableApplyCellColors(ih);
  wasmTableUpdateFocus(ih, to, ih->data->num_col > 0 ? 1 : 0);
}

static int wasmTableMapMethod(Ihandle* ih)
{
  int id, col;

  id = iupwasmJsTableCreate();
  if (!id)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)(intptr_t)id;
  iupwasmRegisterHandle(id, ih);

  iupwasmJsTableFeatures(id, ih->data->allow_reorder, ih->data->user_resize, ih->data->show_dragdrop);

  {
    int charheight = 0;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    iupwasmJsTableSetFitImage(id, ih->data->fit_image, charheight);
  }

  if (iupAttribGetBoolean(ih, "VIRTUALMODE"))
  {
    iupwasmJsTableBuild(id, 0, ih->data->num_col);  /* header only; rows are windowed on scroll */
    wasmTableApplyColors(ih);
    iupwasmJsTableVirtualInit(id);
  }
  else
    iupwasmJsTableBuild(id, ih->data->num_lin, ih->data->num_col);

  for (col = 1; col <= ih->data->num_col; col++)
  {
    char name[32];
    snprintf(name, sizeof(name), "TITLE%d", col);
    {
      char* t = iupAttribGet(ih, name);
      if (t)
        iupwasmJsTableColTitle(id, col, t);
    }
  }

  wasmTableApplyAlign(ih);
  wasmTableApplyCellColors(ih);

  iupwasmAddToParent(ih);
  return IUP_NOERROR;
}

static char* wasmTableGetFitImageAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->fit_image);
}

static int wasmTableSetFitImageAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->fit_image = iupStrBoolean(value);
  if (id)
  {
    int charheight = 0;
    iupdrvFontGetCharSize(ih, NULL, &charheight);
    iupwasmJsTableSetFitImage(id, ih->data->fit_image, charheight);
  }
  return 1;
}

static int wasmTableSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->allow_reorder = iupStrBoolean(value);
  if (id)
    iupwasmJsTableFeatures(id, ih->data->allow_reorder, ih->data->user_resize, ih->data->show_dragdrop);
  return 0;
}

static int wasmTableSetUserResizeAttrib(Ihandle* ih, const char* value)
{
  int id = iupwasmIdOf(ih);
  ih->data->user_resize = iupStrBoolean(value);
  if (id)
    iupwasmJsTableFeatures(id, ih->data->allow_reorder, ih->data->user_resize, ih->data->show_dragdrop);
  return 0;
}

static int wasmTableSetSortableAttrib(Ihandle* ih, const char* value)
{
  ih->data->sortable = iupStrBoolean(value) ? 1 : 0;

  if (!ih->data->sortable && iupAttribGetInt(ih, "_IUPWASM_SORTCOL") > 0)
  {
    iupAttribSet(ih, "_IUPWASM_SORTCOL", NULL);
    iupAttribSet(ih, "_IUPWASM_SORTASC", NULL);
    if (ih->handle)
      wasmTableSortIndicator(ih, 0, 0);
  }
  return 0;
}

IUP_SDK_API void iupdrvTableInitClass(Iclass* ic)
{
  ic->Map = wasmTableMapMethod;
  ic->LayoutUpdate = wasmTableLayoutUpdate;
  ic->UnMap = iupdrvBaseUnMapMethod;

  iupClassRegisterAttribute(ic, "FITIMAGE", wasmTableGetFitImageAttrib, wasmTableSetFitImageAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SORTABLE", NULL, wasmTableSetSortableAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "ALLOWREORDER", NULL, wasmTableSetAllowReorderAttrib);
  iupClassRegisterReplaceAttribFunc(ic, "USERRESIZE", NULL, wasmTableSetUserResizeAttrib);
}
