/*
 * IupPlot element
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iupPlot.h"

#include "iup_plot.h"
#include "iupkey.h"

#include "iup_class.h"
#include "iup_register.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_assert.h"
#include "iup_linefile.h"

#include "iup_plot_ctrl.h"


static int iPlotSelectFile(Ihandle* parent, char* filename, const char* title, const char* extfilter)
{
  Ihandle* filedlg = IupFileDlg();

  IupSetStrAttribute(filedlg, "DIALOGTYPE", "SAVE");
  IupSetStrAttribute(filedlg, "EXTFILTER", extfilter);
  IupSetStrAttribute(filedlg, "TITLE", title);
  IupSetStrAttribute(filedlg, "FILE", filename);
  IupSetAttributeHandle(filedlg, "PARENTDIALOG", parent);

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(filedlg, "STATUS") != -1)
  {
    char* value = IupGetAttribute(filedlg, "VALUE");
    iupStrCopyN(filename, 4096, value);

    IupDestroy(filedlg);
    return 1;
  }

  IupDestroy(filedlg);
  return 0;
}

static double iPlotDataSetValuesMatrixNumericGetValue_CB(Ihandle *ih_matrix, int lin, int col)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_matrix, "PLOT");
  int plot_current = iupAttribGetInt(ih_matrix, "_IUP_PLOT_CURRENT");
  int ds = iupAttribGetInt(ih_matrix, "_IUP_DS");

  // make sure we are changing the right plot
  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);

  if (IupGetInt(ih, "DS_STRXDATA"))
  {
    if (col == 1)
      return 0;
    else
    {
      char* str_x;
      double y;
      IupPlotGetSampleStr(ih, ds, lin - 1, (const char**)&str_x, &y);
      return y;
    }
  }

  double x, y;
  IupPlotGetSample(ih, ds, lin - 1, &x, &y);

  if (col == 1)
    return x;
  else
    return y;
}

static char* iPlotDataSetValuesMatrixValue_CB(Ihandle *ih_matrix, int lin, int col)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_matrix, "PLOT");
  int plot_current = iupAttribGetInt(ih_matrix, "_IUP_PLOT_CURRENT");
  int ds = iupAttribGetInt(ih_matrix, "_IUP_DS");

  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);

  if (lin == 0 && col == 0)
    return (char*)"";

  if (lin == 0)
    return (col == 1) ? IupGetAttribute(ih, "AXS_XLABEL") : IupGetAttribute(ih, "AXS_YLABEL");

  if (col == 1 && IupGetInt(ih, "DS_STRXDATA"))
  {
    char* str_x;
    double y;
    IupPlotGetSampleStr(ih, ds, lin - 1, (const char**)&str_x, &y);
    return str_x;
  }

  if (col == 0)
  {
    static char str[30];
    snprintf(str, sizeof(str), "%d", lin - 1);
    return str;
  }

  return NULL;
}

static int iPlotDataSetValuesMatrixNumericSetValue_CB(Ihandle* ih_matrix, int lin, int col, double new_value)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_matrix, "PLOT");
  int plot_current = iupAttribGetInt(ih_matrix, "_IUP_PLOT_CURRENT");
  int ds = iupAttribGetInt(ih_matrix, "_IUP_DS");
  int sample_index = lin - 1;
  double x, y;

  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);

  if (col == 2 && IupGetInt(ih, "DS_STRXDATA"))
  {
    char* str_x;
    IupPlotGetSampleStr(ih, ds, sample_index, (const char**)&str_x, &y);
    x = sample_index;

    IupPlotSetSampleStr(ih, ds, sample_index, str_x, new_value);
  }
  else
  {
    IupPlotGetSample(ih, ds, sample_index, &x, &y);

    if (col == 1)
      IupPlotSetSample(ih, ds, sample_index, new_value, y);
    else
      IupPlotSetSample(ih, ds, sample_index, x, new_value);
  }

  IFniidd editsample_cb = (IFniidd)IupGetCallback(ih, "EDITSAMPLE_CB");
  if (editsample_cb)
    editsample_cb(ih, ds, sample_index, x, y);

  return IUP_DEFAULT;
}

static int iPlotDataSetValuesMatrixValueEdit_CB(Ihandle* ih_matrix, int lin, int col, char* new_value)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_matrix, "PLOT");
  int plot_current = iupAttribGetInt(ih_matrix, "_IUP_PLOT_CURRENT");
  int ds = iupAttribGetInt(ih_matrix, "_IUP_DS");
  int sample_index = lin - 1;

  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);

  if (col == 1 && IupGetInt(ih, "DS_STRXDATA"))
  {
    char* str_x;
    double x, y;

    IupPlotGetSampleStr(ih, ds, sample_index, (const char**)&str_x, &y);
    x = sample_index;

    IupPlotSetSampleStr(ih, ds, sample_index, new_value, y);

    IFniidd editsample_cb = (IFniidd)IupGetCallback(ih, "EDITSAMPLE_CB");
    if (editsample_cb)
      editsample_cb(ih, ds, sample_index, x, y);
  }

  return IUP_DEFAULT;
}

static int iPlotDataSetValuesMatrixResize_CB(Ihandle *ih, int, int)
{
  IupSetAttribute(ih, "RASTERWIDTH1", NULL);
  IupSetAttribute(ih, "RASTERWIDTH2", NULL);

  IupSetAttribute(ih, "FITTOSIZE", "COLUMNS");

  return IUP_DEFAULT;
}

static int iPlotDataSetValuesButton_CB(Ihandle*)
{
  return IUP_CLOSE;
}

static int iPlotDataSetValues_CB(Ihandle* ih_item)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_item, "PLOT");
  Ihandle* ih_menu = IupGetParent(ih_item);
  int plot_current = iupAttribGetInt(ih_menu, "_IUP_PLOT_CURRENT");
  int ds = iupAttribGetInt(ih_menu, "_IUP_DS");

  Ihandle *matrix = IupCreate("matrixex");  /* IupControlsOpen must have been called somewhere */
  if (!matrix)
    return IUP_DEFAULT;

  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);

  char* ds_name = IupGetAttribute(ih, "DS_NAME");
  Ihandle* label = IupLabel(ds_name);

  Ihandle *button = IupButton("Close", NULL);

  Ihandle *vbox = IupVbox(label, matrix, button, NULL);
  IupSetAttribute(vbox, "ALIGNMENT", "ACENTER");
  IupSetAttribute(vbox, "MARGIN", "10x10");
  IupSetAttribute(vbox, "GAP", "10");

  Ihandle* dlg = IupDialog(vbox);

  Ihandle *parent = IupGetDialog(ih);

  IupSetStrAttribute(dlg, "TITLE", "_@IUP_DATASETVALUESDLG");
  IupSetAttribute(dlg, "MINBOX", "NO");
  IupSetAttribute(dlg, "MAXBOX", "NO");
  IupSetAttribute(dlg, "SHRINK", "YES");
  IupSetAttributeHandle(dlg, "DEFAULTESC", button);
  IupSetAttributeHandle(dlg, "DEFAULTENTER", button);
  IupSetAttributeHandle(dlg, "PARENTDIALOG", parent);

  if (IupGetAttribute(parent, "ICON"))
    IupSetStrAttribute(dlg, "ICON", IupGetAttribute(parent, "ICON"));
  else
    IupSetStrAttribute(dlg, "ICON", IupGetGlobal("ICON"));

  IupSetStrAttribute(button, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));

  IupSetAttribute(matrix, "USETITLESIZE", "YES");
  IupSetAttribute(matrix, "MARKMODE", "CELL");
  IupSetAttribute(matrix, "MARKMULTIPLE", "Yes");
  IupSetAttribute(matrix, "SCROLLBAR", "VERTICAL");

  int count = IupGetInt(ih, "DS_COUNT");
  IupSetAttribute(matrix, "NUMCOL", "2");
  IupSetInt(matrix, "NUMLIN", count);
  IupSetAttribute(matrix, "NUMCOL_VISIBLE", "2");
  if (count > 10)
    IupSetAttribute(matrix, "NUMLIN_VISIBLE", "10");
  else
    IupSetInt(matrix, "NUMLIN_VISIBLE", count);

  if (!IupGetInt(ih, "DS_STRXDATA"))
  {
    IupSetAttribute(matrix, "NUMERICQUANTITY1", "NONE");
    IupSetStrAttribute(matrix, "NUMERICFORMAT1", IupGetAttribute(ih, "AXS_XTICKFORMAT"));
    IupSetAttribute(matrix, "MASK*:1", IUP_MASK_FLOAT);
  }
  IupSetAttribute(matrix, "NUMERICQUANTITY2", "NONE");
  IupSetStrAttribute(matrix, "NUMERICFORMAT2", IupGetAttribute(ih, "AXS_YTICKFORMAT"));
  IupSetAttribute(matrix, "MASK*:2", IUP_MASK_FLOAT);

  IupSetCallback(matrix, "NUMERICGETVALUE_CB", (Icallback)iPlotDataSetValuesMatrixNumericGetValue_CB);
  IupSetCallback(matrix, "RESIZEMATRIX_CB", (Icallback)iPlotDataSetValuesMatrixResize_CB);
  IupSetCallback(matrix, "VALUE_CB", (Icallback)iPlotDataSetValuesMatrixValue_CB);

  if (IupGetInt(ih, "EDITABLEVALUES"))
  {
    IupSetCallback(matrix, "NUMERICSETVALUE_CB", (Icallback)iPlotDataSetValuesMatrixNumericSetValue_CB);
    IupSetCallback(matrix, "VALUE_EDIT_CB", (Icallback)iPlotDataSetValuesMatrixValueEdit_CB);
  }

  IupSetCallback(button, "ACTION", (Icallback)iPlotDataSetValuesButton_CB);

  IupSetAttribute(matrix, "PLOT", (char*)ih);
  iupAttribSetInt(matrix, "_IUP_PLOT_CURRENT", plot_current);
  iupAttribSetInt(matrix, "_IUP_DS", ds);

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  if (IupGetInt(ih, "EDITABLEVALUES"))
    IupSetAttribute(ih, "REDRAW", NULL);

  IupDestroy(dlg);

  return IUP_DEFAULT;
}

static int iPlotKeyPress_CB(Ihandle* ih, int c, int press);
static void iPlotRedrawInteract(Ihandle *ih);

static int iPlotZoomIn_CB(Ihandle* self)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(self, "PLOT");
  return iPlotKeyPress_CB(ih, K_plus, 1);
}

static int iPlotZoomOut_CB(Ihandle* self)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(self, "PLOT");
  return iPlotKeyPress_CB(ih, K_minus, 1);
}

static int iPlotZoomReset_CB(Ihandle* self)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(self, "PLOT");
  return iPlotKeyPress_CB(ih, K_period, 1);
}

static int iPlotShowLegend_CB(Ihandle* self)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(self, "PLOT");
  if (ih->data->current_plot->mLegend.mShow)
    ih->data->current_plot->mLegend.mShow = false;
  else
    ih->data->current_plot->mLegend.mShow = true;

  ih->data->current_plot->mRedraw = true;
  iPlotRedrawInteract(ih);
  return IUP_DEFAULT;
}

static int iPlotShowGrid_CB(Ihandle* self)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(self, "PLOT");
  if (ih->data->current_plot->mGrid.mShowX || ih->data->current_plot->mGrid.mShowY)
  {
    ih->data->current_plot->mGrid.mShowY = false;
    ih->data->current_plot->mGrid.mShowX = false;
  }
  else
  {
    ih->data->current_plot->mGrid.mShowX = true;
    ih->data->current_plot->mGrid.mShowY = true;
  }

  ih->data->current_plot->mRedraw = true;
  iPlotRedrawInteract(ih);
  return IUP_DEFAULT;
}

static int iPlotGetListIndex(const char** list, const char* value)
{
  int i = 0;
  while (list[i])
  {
    if (iupStrEqualNoCase(list[i], value))
      return i;

    i++;
  }
  return 0;
}

struct iPlotAttribParam
{
  const char* name;
  Icallback check;

  const char* label;
  const char* type;
  const char* extra;
  const char* tip;

  const char** list;
};

static const char* iplot_linestyle_list[] = { "CONTINUOUS", "DASHED", "DOTTED", "DASH_DOT", "DASH_DOT_DOT", NULL };
static const char* iplot_fontstyle_list[] = { "", "BOLD", "ITALIC", "BOLDITALIC", NULL };
static const char* iplot_legendpos_list[] = { "TOPRIGHT", "TOPLEFT", "BOTTOMRIGHT", "BOTTOMLEFT", "BOTTOMCENTER", "XY", NULL };
static const char* iplot_grid_list[] = { "NO", "YES", "HORIZONTAL", "VERTICAL", NULL };
static const char* iplot_scale_list[] = { "LIN", "LOG10", "LOG2", "LOGN", NULL };
static const char* iplot_axispos_list[] = { "START", "CROSSORIGIN", "END", NULL };

static const char* iplot_linestyle_extra = { "|_@IUP_CONTINUOUS|_@IUP_DASHED|_@IUP_DOTTED|_@IUP_DASH_DOT|_@IUP_DASH_DOT_DOT|" };
static const char* iplot_fontstyle_extra = { "|_@IUP_PLAIN|_@IUP_BOLD|_@IUP_ITALIC|_@IUP_BOLDITALIC|" };
static const char* iplot_legendpos_extra = { "|_@IUP_TOPRIGHT|_@IUP_TOPLEFT|_@IUP_BOTTOMRIGHT|_@IUP_BOTTOMLEFT|_@IUP_BOTTOMCENTER|_@IUP_XY|" };
static const char* iplot_grid_extra = { "|_@IUP_NO|_@IUP_YES|_@IUP_HORIZONTAL|_@IUP_VERTICAL|" };
static const char* iplot_scale_extra = { "|_@IUP_LINEAR|_@IUP_LOG10|_@IUP_LOG2|_@IUP_LOGN|" };
static const char* iplot_axispos_extra = { "|_@IUP_START|_@IUP_CROSSORIGIN|_@IUP_END|" };

static int iPlotCheckBool(Ihandle* param)
{
  return iupAttribGetBoolean(param, "VALUE");
}

static int iPlotCheckAuto(Ihandle* param)
{
  return !iupAttribGetBoolean(param, "VALUE");
}

static int iPlotCheckAuto2(Ihandle* param)
{
  iupAttribSetInt(param, "CHILDCOUNT", 2);
  return !iupAttribGetBoolean(param, "VALUE");
}

static int iPlotCheckAutoXY(Ihandle* param)  // Simply to be able to distinguish from other iPlotCheckAuto
{
  return !iupAttribGetBoolean(param, "VALUE");
}

static int iPlotCheckLegendXY(Ihandle* param)
{
  int index = iupAttribGetInt(param, "VALUE");
  return iupStrEqualNoCase(iplot_legendpos_list[index], "XY");
}

static iPlotAttribParam iplot_background_attribs[] = {
  { "", NULL, "_@IUP_MARGIN", "t", NULL, NULL, NULL },
  { "MARGINLEFTAUTO", iPlotCheckAuto, "_@IUP_LEFT", "b", "[ ,Auto]", "", NULL },
  { "MARGINLEFT", NULL, "\t_@IUP_VALUE", "i", "", "", NULL },
  { "MARGINRIGHTAUTO", iPlotCheckAuto, "_@IUP_RIGHT", "b", "", "", NULL },
  { "MARGINRIGHT", NULL, "\t_@IUP_VALUE", "i", "", "", NULL },
  { "MARGINTOPAUTO", iPlotCheckAuto, "_@IUP_TOP", "b", "[ ,Auto]", "", NULL },
  { "MARGINTOP", NULL, "\t_@IUP_VALUE", "i", "", "", NULL },
  { "MARGINBOTTOMAUTO", iPlotCheckAuto, "_@IUP_BOTTOM", "b", "[ ,Auto]", "", NULL },
  { "MARGINBOTTOM", NULL, "\t_@IUP_VALUE", "i", "", "", NULL },
  { "", NULL, "", "t", NULL, NULL, NULL },
  { "PADDING", NULL, "_@IUP_PADDING", "s", "[+/-]?/d+[x][+/-]?/d+", "{_@IUP_INTERNALMARGIN}", NULL },
  { "BACKCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_title_attribs[] = {
  { "TITLE", NULL, "_@IUP_TEXT", "s", "", "", NULL },
  { "TITLECOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "TITLEFONTSTYLE", NULL, "_@IUP_FONTSTYLE", "l", iplot_fontstyle_extra, "", iplot_fontstyle_list },
  { "TITLEFONTSIZE", NULL, "_@IUP_FONTSIZE", "i", "[1,,]", "", NULL },
  { "TITLEPOSAUTO", iPlotCheckAutoXY, "_@IUP_POSITION", "b", "[ ,Auto]", "", NULL },
  { "TITLEPOSXY", NULL, "\t_@IUP_POSXY", "s", "[+/-]?/d+[,][+/-]?/d+", "{(pixels)}", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_legend_attribs[] = {
  { "LEGEND", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "LEGENDFONTSTYLE", NULL, "_@IUP_FONTSTYLE", "l", iplot_fontstyle_extra, "", iplot_fontstyle_list },
  { "LEGENDFONTSIZE", NULL, "_@IUP_FONTSIZE", "i", "[1,,]", "", NULL },
  { "LEGENDPOS", iPlotCheckLegendXY, "_@IUP_POSITION", "l", iplot_legendpos_extra, "", iplot_legendpos_list },
  { "LEGENDPOSXY", NULL, "\t_@IUP_POSXY", "s", "[+/-]?/d+[,][+/-]?/d+", "{(pixels)}", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_legendbox_attribs[] = {
  { "LEGENDBOX", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "LEGENDBOXCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "LEGENDBOXBACKCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "LEGENDBOXLINESTYLE", NULL, "_@IUP_LINESTYLE", "l", iplot_linestyle_extra, "", iplot_linestyle_list },
  { "LEGENDBOXLINEWIDTH", NULL, "_@IUP_LINEWIDTH", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_grid_attribs[] = {
  { "GRID", NULL, "_@IUP_SHOW", "l", iplot_grid_extra, "", iplot_grid_list },
  { "GRIDCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "GRIDLINESTYLE", NULL, "_@IUP_LINESTYLE", "l", iplot_linestyle_extra, "", iplot_linestyle_list },
  { "GRIDLINEWIDTH", NULL, "_@IUP_LINEWIDTH", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_gridminor_attribs[] = {
  { "GRIDMINOR", NULL, "_@IUP_SHOW", "l", iplot_grid_extra, "", iplot_grid_list },
  { "GRIDMINORCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "GRIDMINORLINESTYLE", NULL, "_@IUP_LINESTYLE", "l", iplot_linestyle_extra, "", iplot_linestyle_list },
  { "GRIDMINORLINEWIDTH", NULL, "_@IUP_LINEWIDTH", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_box_attribs[] = {
  { "BOX", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "BOXCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "BOXLINESTYLE", NULL, "_@IUP_LINESTYLE", "l", iplot_linestyle_extra, "", iplot_linestyle_list },
  { "BOXLINEWIDTH", NULL, "_@IUP_LINEWIDTH", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisX_attribs[] = {
  { "AXS_X", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "AXS_XARROW", NULL, "_@IUP_SHOWARROW", "b", "", "", NULL },
  { "AXS_XCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "AXS_XLINEWIDTH", NULL, "_@IUP_LINEWIDTH", "i", "[1,,]", "", NULL },
  { "", NULL, "", "t", NULL, NULL, NULL },
  { "AXS_XAUTOMIN", iPlotCheckAuto, "_@IUP_MIN", "b", "[ ,Auto]", "", NULL },
  { "AXS_XMIN", NULL, "\t_@IUP_VALUE", "R", "", "", NULL },
  { "AXS_XAUTOMAX", iPlotCheckAuto, "_@IUP_MAX", "b", "[ ,Auto]", "", NULL },
  { "AXS_XMAX", NULL, "\t_@IUP_VALUE", "R", "", "", NULL },
  { "AXS_XSCALE", NULL, "_@IUP_SCALE", "l", iplot_scale_extra, "", iplot_scale_list },
  { "AXS_XREVERSE", NULL, "_@IUP_REVERSE", "b", "", "", NULL },
  { "AXS_XPOSITION", NULL, "_@IUP_POSITION", "l", iplot_axispos_extra, "", iplot_axispos_list },
  { "AXS_XREVERSETICKSLABEL", NULL, "_@IUP_REVERSETICKSLABEL", "b", "", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisY_attribs[] = {
  { "AXS_Y", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "AXS_YARROW", NULL, "_@IUP_SHOWARROW", "b", "", "", NULL },
  { "AXS_YCOLOR", NULL, "_@IUP_COLOR", "c", "", "", NULL },
  { "AXS_YLINEWIDTH", NULL, "_@IUP_LINEWIDTH", "i", "[1,,]", "", NULL },
  { "", NULL, "", "t", NULL, NULL, NULL },
  { "AXS_YAUTOMIN", iPlotCheckAuto, "_@IUP_MIN", "b", "[ ,Auto]", "", NULL },
  { "AXS_YMIN", NULL, "\t_@IUP_VALUE", "R", "", "", NULL },
  { "AXS_YAUTOMAX", iPlotCheckAuto, "_@IUP_MAX", "b", "[ ,Auto]", "", NULL },
  { "AXS_YMAX", NULL, "\t_@IUP_VALUE", "R", "", "", NULL },
  { "AXS_YSCALE", NULL, "_@IUP_SCALE", "l", iplot_scale_extra, "", iplot_scale_list },
  { "AXS_YREVERSE", NULL, "_@IUP_REVERSE", "b", "", "", NULL },
  { "AXS_YPOSITION", NULL, "_@IUP_POSITION", "l", iplot_axispos_extra, "", iplot_axispos_list },
  { "AXS_YREVERSETICKSLABEL", NULL, "_@IUP_REVERSETICKSLABEL", "b", "", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisXlabel_attribs[] = {
  { "AXS_XLABEL", NULL, "_@IUP_TEXT", "s", "", "", NULL },
  { "AXS_XLABELCENTERED", NULL, "_@IUP_CENTERED", "b", "", "", NULL },
  { "AXS_XLABELSPACING", NULL, "_@IUP_SPACING", "i", "[-1,,]", "", NULL },
  { "AXS_XFONTSTYLE", NULL, "_@IUP_FONTSTYLE", "l", iplot_fontstyle_extra, "", iplot_fontstyle_list },
  { "AXS_XFONTSIZE", NULL, "_@IUP_FONTSIZE", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisYlabel_attribs[] = {
  { "AXS_YLABEL", NULL, "_@IUP_TEXT", "s", "", "", NULL },
  { "AXS_YLABELCENTERED", NULL, "_@IUP_CENTERED", "b", "", "", NULL },
  { "AXS_YLABELSPACING", NULL, "_@IUP_SPACING", "i", "[-1,,]", "", NULL },
  { "AXS_YFONTSTYLE", NULL, "_@IUP_FONTSTYLE", "l", iplot_fontstyle_extra, "", iplot_fontstyle_list },
  { "AXS_YFONTSIZE", NULL, "_@IUP_FONTSIZE", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisXticks_attribs[] = {
  { "AXS_XTICK", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "AXS_XTICKAUTO", iPlotCheckAuto2, "_@IUP_SPACING", "b", "[ ,Auto]", "", NULL },
  { "AXS_XTICKMAJORSPAN", NULL, "\t_@IUP_MAJORSPAN", "R", "", "", NULL },
  { "AXS_XTICKMINORDIVISION", NULL, "\t_@IUP_MINORDIVISION", "i", "[1,,]", "", NULL },
  { "AXS_XTICKSIZEAUTO", iPlotCheckAuto2, "_@IUP_SIZE", "b", "[ ,Auto]", "", NULL },
  { "AXS_XTICKMAJORSIZE", NULL, "\t_@IUP_MAJOR", "i", "[1,,]", "", NULL },
  { "AXS_XTICKMINORSIZE", NULL, "\t_@IUP_MINOR", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisYticks_attribs[] = {
  { "AXS_YTICK", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "AXS_YTICKAUTO", iPlotCheckAuto2, "_@IUP_SPACING", "b", "[ ,Auto]", "", NULL },
  { "AXS_YTICKMAJORSPAN", NULL, "\t_@IUP_MAJORSPAN", "R", "", "", NULL },
  { "AXS_YTICKMINORDIVISION", NULL, "\t_@IUP_MINORDIVISION", "i", "[1,,]", "", NULL },
  { "AXS_YTICKSIZEAUTO", iPlotCheckAuto2, "_@IUP_SIZE", "b", "[ ,Auto]", "", NULL },
  { "AXS_YTICKMAJORSIZE", NULL, "\t_@IUP_MAJOR", "i", "[1,,]", "", NULL },
  { "AXS_YTICKMINORSIZE", NULL, "\t_@IUP_MINOR", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisXticksnumber_attribs[] = {
  { "AXS_XTICKNUMBER", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "AXS_XTICKROTATENUMBER", iPlotCheckBool, "_@IUP_ROTATE", "b", "", "", NULL },
  { "AXS_XTICKROTATENUMBERANGLE", NULL, "\t_@IUP_ANGLE", "A", "", "", NULL },
  { "AXS_XTICKFORMATAUTO", iPlotCheckAuto, "_@IUP_FORMAT", "b", "[ ,Auto]", "", NULL },
  { "AXS_XTICKFORMATPRECISION", NULL, "\t_@IUP_DECIMALS", "i", "[0,,]", "", NULL },
  { "AXS_XTICKFONTSTYLE", NULL, "_@IUP_FONTSTYLE", "l", iplot_fontstyle_extra, "", iplot_fontstyle_list },
  { "AXS_XTICKFONTSIZE", NULL, "_@IUP_FONTSIZE", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static iPlotAttribParam iplot_axisYticksnumber_attribs[] = {
  { "AXS_YTICKNUMBER", NULL, "_@IUP_SHOW", "b", "", "", NULL },
  { "AXS_YTICKROTATENUMBER", iPlotCheckBool, "_@IUP_ROTATE", "b", "", "", NULL },
  { "AXS_YTICKROTATENUMBERANGLE", NULL, "\t_@IUP_ANGLE", "A", "", "", NULL },
  { "AXS_YTICKFORMATAUTO", iPlotCheckAuto, "_@IUP_FORMAT", "b", "[ ,Auto]", "", NULL },
  { "AXS_YTICKFORMATPRECISION", NULL, "\t_@IUP_DECIMALS", "i", "[0,,]", "", NULL },
  { "AXS_YTICKFONTSTYLE", NULL, "_@IUP_FONTSTYLE", "l", iplot_fontstyle_extra, "", iplot_fontstyle_list },
  { "AXS_YTICKFONTSIZE", NULL, "_@IUP_FONTSIZE", "i", "[1,,]", "", NULL },
  { NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

static void iPlotSetParamDouble(Ihandle* control, const char* name, double num)
{
  char value[80];
  char format[30];
  int prec = IupGetInt(NULL, "DEFAULTPRECISION");
  snprintf(format, sizeof(format), "%%.%df", prec);
  iupStrPrintfDoubleLocale(value, format, num, IupGetGlobal("DEFAULTDECIMALSYMBOL"));

  IupStoreAttribute(control, name, value);
}

static void iPlotSetParamValue(Ihandle* param, const char* value)
{
  Ihandle* control = (Ihandle*)IupGetAttribute(param, "CONTROL");
  Ihandle* auxcontrol = (Ihandle*)IupGetAttribute(param, "AUXCONTROL");

  if (value && iupStrEqualNoCase(IupGetAttribute(param, "TYPE"), "LIST"))
  {
    const char** list = (const char**)IupGetAttribute(param, "PLOT_ATTRIBLIST");
    int index = iPlotGetListIndex(list, value);

    IupSetInt(param, "VALUE", index);
    if (control) IupSetInt(control, "VALUE", index + 1);
    // No Aux here
  }
  else
  {
    IupSetStrAttribute(param, "VALUE", value);
    if (control)
    {
      if (iupStrEqualNoCase(iupAttribGet(param, "TYPE"), "REAL"))
      {
        double num = IupGetDouble(param, "VALUE");
        iPlotSetParamDouble(control, "VALUE", num);
      }
      else
        IupSetStrAttribute(control, "VALUE", value);
    }
    if (auxcontrol) IupSetStrAttribute(auxcontrol, "VALUE", value);
  }
}

static const char* iPlotGetParamValue(Ihandle* param)
{
  char* value = IupGetAttribute(param, "VALUE");
  if (!value || value[0] == 0)
    return NULL;  /* reset to default */
  else
  {
    if (iupStrEqualNoCase(IupGetAttribute(param, "TYPE"), "LIST"))
    {
      const char** list = (const char**)IupGetAttribute(param, "PLOT_ATTRIBLIST");
      int index;
      iupStrToInt(value, &index);
      return list[index];
    }

    return value;
  }
}

static void iPlotPropertiesCheckUpdateXY(Ihandle* ih, Ihandle* parambox, Ihandle* param, int param_index)
{
  Icallback check = IupGetCallback(param, "PLOT_ATTRIBCHECK_CB");
  if (check == iPlotCheckAutoXY ||
      check == iPlotCheckLegendXY)
  {
    int active = check(param);
    if (!active)
    {
      /* if not active is automatically calculated every draw, must update param */

      param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", param_index + 1);

      char* name = IupGetAttribute(param, "PLOT_ATTRIB");
      // From Plot
      char* value = IupGetAttribute(ih, name);
      // To Param
      iPlotSetParamValue(param, value);
    }
  }
}

static void iPlotPropertiesCheckParam(Ihandle* parambox, Ihandle* param, int param_index)
{
  Icallback check = IupGetCallback(param, "PLOT_ATTRIBCHECK_CB");
  if (check)
  {
    /* disable or enable the next childcount params */
    int active = check(param);
    int count = iupAttribGetInt(param, "CHILDCOUNT");
    if (count == 0) count = 1;
    while (count)
    {
      param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", param_index + count);

      Ihandle* control = (Ihandle*)IupGetAttribute(param, "CONTROL");
      IupSetInt(IupGetParent(control), "ACTIVE", active);

      count--;
    }
  }
}

static void iPlotPropertiesInit(Ihandle* parambox)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(parambox, "PLOT");
  Ihandle* zbox = IupGetParent(parambox);

  int plot_current = iupAttribGetInt(zbox, "_IUP_PLOT_CURRENT");
  // make sure we are accessing the right plot
  IupSetInt(ih, "PLOT_CURRENT", plot_current);

  int i, count = IupGetInt(parambox, "PARAMCOUNT");
  for (i = 0; i < count; i++)
  {
    Ihandle* param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", i);
    char* name = IupGetAttribute(param, "PLOT_ATTRIB");

    // From Plot
    char* value = IupGetAttribute(ih, name);
    // To Param
    iPlotSetParamValue(param, value);

    iPlotPropertiesCheckParam(parambox, param, i);
  }

  IupSetAttribute(parambox, "PLOT_CHANGED", NULL);
  IupSetAttribute(ih, "REDRAW", NULL);
}

static void iPlotPropertiesResetChanges(Ihandle* parambox)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(parambox, "PLOT");
  Ihandle* zbox = IupGetParent(parambox);

  int plot_current = iupAttribGetInt(zbox, "_IUP_PLOT_CURRENT");
  // make sure we are changing the right plot
  IupSetInt(ih, "PLOT_CURRENT", plot_current);

  int i, count = IupGetInt(parambox, "PARAMCOUNT");
  for (i = count - 1; i >= 0; i--) // backwards to avoid dependencies
  {
    Ihandle* param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", i);
    char* name = IupGetAttribute(param, "PLOT_ATTRIB");

    // From Original Value
    const char* value = IupGetAttribute(param, "RESET_VALUE");
    // To Param
    iPlotSetParamValue(param, value);

    iPlotPropertiesCheckParam(parambox, param, i);

    // From Param
    value = iPlotGetParamValue(param);
    // To Plot
    IupSetStrAttribute(ih, name, value);
  }

  IupSetAttribute(parambox, "PLOT_CHANGED", NULL);
  IupSetAttribute(ih, "REDRAW", NULL);

  Icallback cb = IupGetCallback(ih, "PROPERTIESCHANGED_CB");
  if (cb)
    cb(ih);
}

static void iPlotPropertiesApplyChanges(Ihandle* parambox)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(parambox, "PLOT");
  Ihandle* zbox = IupGetParent(parambox);
  IFnss validate_cb = (IFnss)IupGetCallback(ih, "PROPERTIESVALIDATE_CB");

  int plot_current = iupAttribGetInt(zbox, "_IUP_PLOT_CURRENT");
  // make sure we are changing the right plot
  IupSetInt(ih, "PLOT_CURRENT", plot_current);

  int i, count = IupGetInt(parambox, "PARAMCOUNT");
  for (i = count - 1; i >= 0; i--) // backwards to avoid dependencies
  {
    Ihandle* param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", i);
    char* name = IupGetAttribute(param, "PLOT_ATTRIB");

    // From Param
    const char* value = iPlotGetParamValue(param);
    if (validate_cb && validate_cb(ih, name, (char*)value) == IUP_IGNORE)
      continue;
    else
    {
      // To Plot
      IupSetAttribute(ih, name, value);
    }
  }

  IupSetAttribute(parambox, "PLOT_CHANGED", NULL);
  IupSetAttribute(ih, "REDRAW", NULL);

  for (i = 0; i < count; i++)
  {
    Ihandle* param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", i);
    iPlotPropertiesCheckUpdateXY(ih, parambox, param, i);
  }

  Icallback cb = IupGetCallback(ih, "PROPERTIESCHANGED_CB");
  if (cb)
    cb(ih);
}

static void iPlotPropertiesCheckChanges(Ihandle* parambox)
{
  if (IupGetInt(parambox, "PLOT_CHANGED"))
  {
    Ihandle* dlg = IupMessageDlg();

    IupSetAttributeHandle(dlg, "PARENTDIALOG", IupGetDialog(parambox));

    IupSetAttribute(dlg, "DIALOGTYPE", "WARNING");
    IupSetAttribute(dlg, "BUTTONS", "YESNO");

    IupSetStrAttribute(dlg, "TITLE", "_@IUP_ATTENTION");
    IupSetStrAttribute(dlg, "VALUE", "_@IUP_CHANGESNOTAPPLIEDAPPLY");

    IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

    int ret = IupGetInt(dlg, "BUTTONRESPONSE");
    IupDestroy(dlg);

    if (ret == 1)
      iPlotPropertiesApplyChanges(parambox);
    else
      IupSetAttribute(parambox, "PLOT_CHANGED", NULL);
  }
}

static int iPlotPropertiesTreeSelection_CB(Ihandle *ih_tree, int id, int status)
{
  if (status == 0)
  {
    Ihandle* zbox = IupGetBrother(ih_tree);
    Ihandle* parambox = (Ihandle*)IupGetAttribute(zbox, "VALUE_HANDLE");
    iPlotPropertiesCheckChanges(parambox);
  }
  if (status == 1)
  {
    Ihandle* zbox = IupGetBrother(ih_tree);
    IupSetInt(zbox, "VALUEPOS", id);
    Ihandle* parambox = (Ihandle*)IupGetAttribute(zbox, "VALUE_HANDLE");
    iPlotPropertiesInit(parambox);
  }
  return IUP_DEFAULT;
}

static int iPlotPropertiesParam_CB(Ihandle* parambox, int param_index, void*)
{
  if (param_index == IUP_GETPARAM_BUTTON1)
  {
    iPlotPropertiesApplyChanges(parambox);
    return 0;
  }
  if (param_index == IUP_GETPARAM_BUTTON2)
  {
    iPlotPropertiesResetChanges(parambox);
    return 0;
  }
  if (param_index == IUP_GETPARAM_BUTTON3)
  {
    iPlotPropertiesCheckChanges(parambox);

    IupExitLoop();
    return 0;
  }

  Ihandle* param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", param_index);
  iPlotPropertiesCheckParam(parambox, param, param_index);

  IupSetAttribute(parambox, "PLOT_CHANGED", "1");
  return 1;
}

static int iPlotPropertiesClose_CB(Ihandle* dlg)
{
  Ihandle* zbox = IupGetChild(dlg, 1);
  Ihandle* parambox = (Ihandle*)IupGetAttribute(zbox, "VALUE_HANDLE");
  iPlotPropertiesCheckChanges(parambox);
  return IUP_CLOSE;
}

static void iPlotPropertiesAddParamBox(Ihandle* ih, Ihandle* zbox, iPlotAttribParam* attribs)
{
  Ihandle* params[50];
  int count = 0;
  char format[10240];

  while (attribs[count].name)
  {
    snprintf(format, sizeof(format), "%s%%%s%s%s\n", attribs[count].label, attribs[count].type, attribs[count].extra, attribs[count].tip);
    params[count] = IupParam(format);

    if (attribs[count].name[0] != 0)
    {
      IupSetStrAttribute(params[count], "PLOT_ATTRIB", attribs[count].name);
      IupSetAttribute(params[count], "PLOT_ATTRIBLIST", (char*)(attribs[count].list));
      IupSetCallback(params[count], "PLOT_ATTRIBCHECK_CB", attribs[count].check);

      // From Plot
      char* value = IupGetAttribute(ih, attribs[count].name);
      // To Param
      iPlotSetParamValue(params[count], value);
      IupSetStrAttribute(params[count], "RESET_VALUE", value);
    }

    count++;
  }

  params[count] = IupParam("%u[,,_@IUP_CLOSE]");
  count++;
  params[count] = NULL;

  Ihandle* parambox = IupParamBoxv(params);
  IupSetCallback(parambox, "PARAM_CB", (Icallback)iPlotPropertiesParam_CB);

  IupAppend(zbox, parambox);

  count = IupGetInt(parambox, "PARAMCOUNT");
  for (int i = 0; i < count; i++)
  {
    Ihandle* param = (Ihandle*)IupGetAttributeId(parambox, "PARAM", i);
    iPlotPropertiesCheckParam(parambox, param, i);
  }
}

static int iPlotProperties_CB(Ihandle* ih_item)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_item, "PLOT");
  Ihandle* parent = IupGetDialog(ih);
  Ihandle* ih_menu = IupGetParent(ih_item);
  int plot_current = iupAttribGetInt(ih_menu, "_IUP_PLOT_CURRENT");
  IupSetInt(ih, "PLOT_CURRENT", plot_current);

  Ihandle* tree = IupTree();
  IupSetAttribute(tree, "ADDROOT", "NO");
  IupSetCallback(tree, "SELECTION_CB", (Icallback)iPlotPropertiesTreeSelection_CB);
  IupSetAttribute(tree, "EXPAND", "VERTICAL");
  IupSetAttribute(tree, "SIZE", "100x140");
  IupSetAttribute(tree, "IMAGELEAF", "IMGPAPER");

  Ihandle* zbox = IupZbox(NULL);
  iPlotPropertiesAddParamBox(ih, zbox, iplot_background_attribs);    /* 0 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_title_attribs);         /* 1 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_legend_attribs);        /* 2 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_legendbox_attribs);     /* 3 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_box_attribs);           /* 4 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_grid_attribs);          /* 5 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_gridminor_attribs);     /* 6 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisX_attribs);         /* 7 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisXlabel_attribs);    /* 8 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisXticks_attribs);    /* 9 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisXticksnumber_attribs);  /* 10 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisY_attribs);             /* 11 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisYlabel_attribs);        /* 12 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisYticks_attribs);        /* 13 */
  iPlotPropertiesAddParamBox(ih, zbox, iplot_axisYticksnumber_attribs);  /* 14 */

  IupSetAttribute(zbox, "PLOT", (char*)ih);
  iupAttribSetInt(zbox, "_IUP_PLOT_CURRENT", plot_current);

  Ihandle* dlg = IupDialog(IupHbox(tree, zbox, NULL));
  IupSetAttributeHandle(dlg, "PARENTDIALOG", parent);
  IupSetStrAttribute(dlg, "TITLE", "_@IUP_PROPERTIESDLG");
  IupSetCallback(dlg, "K_ESC", iPlotPropertiesClose_CB);
  IupSetCallback(dlg, "CLOSE_CB", (Icallback)iPlotPropertiesClose_CB);
  IupSetAttribute(dlg, "MINBOX", "NO");
  IupSetAttribute(dlg, "MAXBOX", "NO");

  if (IupGetAttribute(parent, "ICON"))
    IupSetStrAttribute(dlg, "ICON", IupGetAttribute(parent, "ICON"));
  else
    IupSetStrAttribute(dlg, "ICON", IupGetGlobal("ICON"));

  IupMap(dlg);

  IupSetStrAttribute(tree, "ADDLEAF-1", "_@IUP_BACKGROUND");  /* 0 */
  IupSetStrAttribute(tree, "ADDLEAF0", "_@IUP_TITLE");        /* 1 */
  IupSetStrAttribute(tree, "ADDBRANCH1", "_@IUP_LEGEND");     /* 2 */
  IupSetStrAttribute(tree, "ADDLEAF2", "_@IUP_LEGENDBOX");  /* 3 */
  IupSetStrAttribute(tree, "INSERTLEAF2", "_@IUP_BOX");       /* 4 */
  IupSetStrAttribute(tree, "ADDBRANCH4", "_@IUP_GRID");       /* 5 */
  IupSetStrAttribute(tree, "ADDLEAF5", "_@IUP_GRIDMINOR");  /* 6 */
  IupSetStrAttribute(tree, "INSERTBRANCH5", "_@IUP_XAXIS");   /* 7 */
  IupSetStrAttribute(tree, "ADDLEAF7", "_@IUP_AXISLABEL");  /* 8 */
  IupSetStrAttribute(tree, "ADDLEAF8", "_@IUP_AXISTICKS");  /* 9 */
  IupSetStrAttribute(tree, "ADDLEAF9", "_@IUP_AXISTICKSNUMBER");   /* 10 */
  IupSetStrAttribute(tree, "INSERTBRANCH7", "_@IUP_YAXIS");          /* 11 */
  IupSetStrAttribute(tree, "ADDLEAF11", "_@IUP_AXISLABEL");        /* 12 */
  IupSetStrAttribute(tree, "ADDLEAF12", "_@IUP_AXISTICKS");        /* 13 */
  IupSetStrAttribute(tree, "ADDLEAF13", "_@IUP_AXISTICKSNUMBER");  /* 14 */

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);

  IupSetAttribute(ih, "REDRAW", NULL);

  IupDestroy(dlg);

  return IUP_DEFAULT;
}

static int iPlotDataSetPropertiesParam_cb(Ihandle* param_dialog, int param_index, void* user_data)
{
  if (param_index == IUP_GETPARAM_MAP)
  {
    Ihandle* ih = (Ihandle*)user_data;
    IupSetAttributeHandle(param_dialog, "PARENTDIALOG", IupGetDialog(ih));
  }
  else if (param_index == IUP_GETPARAM_BUTTON1)
  {
    Ihandle* ih = (Ihandle*)user_data;
    IFnni cb = (IFnni)IupGetCallback(ih, "DSPROPERTIESVALIDATE_CB");
    int ds = IupGetInt(ih, "_IUP_DS");
    if (cb && cb(ih, param_dialog, ds) == IUP_IGNORE)
      return 0;
  }

  return 1;
}

static int iPlotDataSetProperties_CB(Ihandle* ih_item)
{
  Ihandle* ih = (Ihandle*)IupGetAttribute(ih_item, "PLOT");
  Ihandle* ih_menu = IupGetParent(ih_item);
  int plot_current = iupAttribGetInt(ih_menu, "_IUP_PLOT_CURRENT");
  int ds = iupAttribGetInt(ih_menu, "_IUP_DS");
  char name[100];

  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);
  IupSetInt(ih, "_IUP_DS", ds);

  char* ds_name = IupGetAttribute(ih, "DS_NAME");
  iupStrCopyN(name, sizeof(name), ds_name);

  const char* ds_color = IupGetAttribute(ih, "DS_COLOR");
  char color[30];
  iupStrCopyN(color, sizeof(color), ds_color);

  const char* ds_mode = IupGetAttribute(ih, "DS_MODE");
  const char* mode_list[] = { "LINE", "MARK", "MARKLINE", "AREA", "BAR", "STEM", "MARKSTEM", "HORIZONTALBAR", "MULTIBAR", "STEP", "ERRORBAR", "PIE", NULL };
  int mode = iPlotGetListIndex(mode_list, ds_mode);

  const char* ds_linestyle = IupGetAttribute(ih, "DS_LINESTYLE");
  int linestyle = iPlotGetListIndex(iplot_linestyle_list, ds_linestyle);

  int linewidth = IupGetInt(ih, "DS_LINEWIDTH");

  const char* ds_markstyle = IupGetAttribute(ih, "DS_MARKSTYLE");
  const char* markstyle_list[] = { "PLUS", "STAR", "CIRCLE", "X", "BOX", "DIAMOND", "HOLLOW_CIRCLE", "HOLLOW_BOX", "HOLLOW_DIAMOND", NULL };
  int markstyle = iPlotGetListIndex(markstyle_list, ds_markstyle);

  int marksize = IupGetInt(ih, "DS_MARKSIZE");

  int barOutline = IupGetInt(ih, "DS_BAROUTLINE");

  const char* ds_barOutlineColor = IupGetAttribute(ih, "DS_BAROUTLINECOLOR");
  char barOutlineColor[30];
  iupStrCopyN(barOutlineColor, sizeof(barOutlineColor), ds_barOutlineColor);

  int barSpacing = IupGetInt(ih, "DS_BARSPACING");

  int areaTransparency = IupGetInt(ih, "DS_AREATRANSPARENCY");

  double pieRadius = IupGetDouble(ih, "DS_PIERADIUS");
  double pieStartAngle = IupGetDouble(ih, "DS_PIESTARTANGLE");
  int pieContour = IupGetInt(ih, "DS_PIECONTOUR");
  double pieHole = IupGetDouble(ih, "DS_PIEHOLE");
  const char* pieSliceLabel = IupGetAttribute(ih, "DS_PIESLICELABEL");
  const char* pieSliceLabel_list[] = { "NONE", "X", "Y", "PERCENT", NULL };
  int pieSliceLabel_index = iPlotGetListIndex(pieSliceLabel_list, pieSliceLabel);
  double pieSliceLabelPos = IupGetDouble(ih, "DS_PIESLICELABELPOS");

  char format[1024] =
    "_@IUP_NAME%s\n"
    "_@IUP_COLOR%c\n"
    "_@IUP_MODE%l|_@IUP_LINES|_@IUP_MARKS|_@IUP_MARKSLINES|_@IUP_AREA|_@IUP_BARS|_@IUP_STEMS|_@IUP_MARKSSTEMS|_@IUP_HORIZONTALBARS|_@IUP_MULTIBARS|_@IUP_STEPS|_@IUP_ERRORBARS|_@IUP_PIE|\n"
    "_@IUP_LINESTYLE%l|_@IUP_CONTINUOUS|_@IUP_DASHED|_@IUP_DOTTED|_@IUP_DASH_DOT|_@IUP_DASH_DOT_DOT|\n"
    "_@IUP_LINEWIDTH%i[1,,]\n"
    "_@IUP_MARKSTYLE%l|_@IUP_PLUS|_@IUP_STAR|_@IUP_CIRCLE|_@IUP_X|_@IUP_BOX|_@IUP_DIAMOND|_@IUP_HOLLOW_CIRCLE|_@IUP_HOLLOW_BOX|_@IUP_HOLLOW_DIAMOND|\n"
    "_@IUP_MARKSIZE%i[1,,]\n"
    "_@IUP_BARSPACING%i[0,100]\n"
    "_@IUP_BAROUTLINE%b[false,true]\n"
    "_@IUP_BAROUTLINECOLOR%c\n"
    "_@IUP_AREATRANSPARENCY%i[0,255]\n"
    "_@IUP_PIERADIUS%R[0,,]\n"
    "_@IUP_PIESTARTANGLE%R[0,360,]\n"
    "_@IUP_PIECONTOUR%b[false,true]\n"
    "_@IUP_PIEHOLE%R[0,1,]\n"
    "_@IUP_PIESLICELABEL%l|_@IUP_NONE|X|Y|_@IUP_PERCENT|\n"
    "_@IUP_PIESLICELABELPOS%R[0,1,]\n";

  if (!IupGetParam("_@IUP_DATASETPROPERTIESDLG", iPlotDataSetPropertiesParam_cb, ih, format,
                   name, color, &mode, &linestyle, &linewidth, &markstyle, &marksize,
                   &barSpacing, &barOutline, barOutlineColor,
                   &areaTransparency,
                   &pieRadius, &pieStartAngle, &pieContour, &pieHole, &pieSliceLabel_index, &pieSliceLabelPos,
                   NULL))
    return IUP_DEFAULT;

  // make sure we are changing the right plot
  IupSetInt(ih, "PLOT_CURRENT", plot_current);
  IupSetInt(ih, "CURRENT", ds);
  IupSetAttribute(ih, "_IUP_DS", NULL);

  IupSetStrAttribute(ih, "DS_NAME", name);
  IupSetStrAttribute(ih, "DS_COLOR", color);

  ds_mode = mode_list[mode];
  IupSetStrAttribute(ih, "DS_MODE", ds_mode);

  ds_linestyle = iplot_linestyle_list[linestyle];
  IupSetStrAttribute(ih, "DS_LINESTYLE", ds_linestyle);

  IupSetInt(ih, "DS_LINEWIDTH", linewidth);

  ds_markstyle = markstyle_list[markstyle];
  IupSetStrAttribute(ih, "DS_MARKSTYLE", ds_markstyle);

  IupSetInt(ih, "DS_MARKSIZE", marksize);

  if (barOutline == 1)
    IupSetAttribute(ih, "DS_BAROUTLINE", "Yes");
  else
    IupSetAttribute(ih, "DS_BAROUTLINE", "No");
  IupSetInt(ih, "DS_BARSPACING", barSpacing);
  IupSetStrAttribute(ih, "DS_BAROUTLINECOLOR", barOutlineColor);

  IupSetInt(ih, "DS_AREATRANSPARENCY", areaTransparency);

  IupSetDouble(ih, "DS_PIERADIUS", pieRadius);
  IupSetDouble(ih, "DS_PIESTARTANGLE", pieStartAngle);
  IupSetInt(ih, "DS_PIECONTOUR", pieContour);
  IupSetDouble(ih, "DS_PIEHOLE", pieHole);
  pieSliceLabel = pieSliceLabel_list[pieSliceLabel_index];
  IupSetStrAttribute(ih, "DS_PIESLICELABEL", pieSliceLabel);
  IupSetDouble(ih, "DS_PIESLICELABELPOS", pieSliceLabelPos);

  IupSetAttribute(ih, "REDRAW", NULL);

  IFni cb = (IFni)IupGetCallback(ih, "DSPROPERTIESCHANGED_CB");
  if (cb)
    cb(ih, ds);

  return IUP_DEFAULT;
}

static Ihandle* iPlotCreateMenuContext(Ihandle* ih, int x, int y)
{
  Ihandle* menu = IupMenu(
    IupSetCallbacks(IupMenuItem("_@IUP_ZOOMINAC", NULL), "ACTION", iPlotZoomIn_CB, NULL),
    IupSetCallbacks(IupMenuItem("_@IUP_ZOOMOUTAC", NULL), "ACTION", iPlotZoomOut_CB, NULL),
    IupSetCallbacks(IupMenuItem("_@IUP_RESETZOOMAC", NULL), "ACTION", iPlotZoomReset_CB, NULL),
    IupMenuSeparator(),
    IupSetCallbacks(IupMenuItem("_@IUP_SHOWHIDELEGEND", NULL), "ACTION", iPlotShowLegend_CB, NULL),
    IupSetCallbacks(IupMenuItem("_@IUP_SHOWHIDEGRID", NULL), "ACTION", iPlotShowGrid_CB, NULL),
    NULL);

  if (IupGetInt(ih, "MENUITEMPROPERTIES") || IupGetInt(ih, "MENUITEMVALUES"))
  {
    Ihandle* itemProp = NULL, *itemVal = NULL;
    IupAppend(menu, IupMenuSeparator());
    if (iupRegisterFindClass("matrixex") && !iupStrEqualNoCase(iupAttribGet(ih, "MENUITEMVALUES"), "HIDE"))
      IupAppend(menu, IupSetCallbacks(itemVal = IupMenuItem("_@IUP_DATASETVALUESDLG", NULL), "ACTION", iPlotDataSetValues_CB, NULL));
    if (IupGetInt(ih, "MENUITEMPROPERTIES"))
    {
      IupAppend(menu, IupSetCallbacks(itemProp = IupMenuItem("_@IUP_DATASETPROPERTIESDLG", NULL), "ACTION", iPlotDataSetProperties_CB, NULL));
      IupAppend(menu, IupSetCallbacks(IupMenuItem("_@IUP_PROPERTIESDLG", NULL), "ACTION", iPlotProperties_CB, NULL));
    }

    int ds = IupGetInt(ih, "CURRENT");
    int sample1, sample2;
    double rx1, ry1, rx2, ry2;
    const char* ds_name;
    const char* strX;
    if (ih->data->current_plot->FindDataSetSample((double)x, (double)y, ds, ds_name, sample1, rx1, ry1, strX) ||
        ((ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_CURVE || ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_BOTH) && ih->data->current_plot->FindDataSetSegment((double)x, (double)y, ds, ds_name, sample1, rx1, ry1, sample2, rx2, ry2)))
    {
      // save plot info because it may have changed by the time the callback is called
      iupAttribSetInt(menu, "_IUP_DS", ds);

      if (itemProp) IupSetAttribute(itemProp, "ACTIVE", "YES");
      if (itemVal) IupSetAttribute(itemVal, "ACTIVE", "YES");
    }
    else
    {
      if (itemProp) IupSetAttribute(itemProp, "ACTIVE", "NO");
      if (itemVal) IupSetAttribute(itemVal, "ACTIVE", "NO");
    }

    // save plot info because it may have changed by the time the callback is called
    iupAttribSetInt(menu, "_IUP_PLOT_CURRENT", ih->data->current_plot_index);
  }

  IupSetAttribute(menu, "PLOT", (char*)ih);

  return menu;
}

void iupPlotShowMenuContext(Ihandle* ih, int screen_x, int screen_y, int x, int y)
{
  Ihandle* menu = iPlotCreateMenuContext(ih, x, y);
  IFnnii menucontext_cb;

  menucontext_cb = (IFnnii)IupGetCallback(ih, "MENUCONTEXT_CB");
  if (menucontext_cb)
    menucontext_cb(ih, menu, x, y);

  IupPopup(menu, screen_x, screen_y);

  menucontext_cb = (IFnnii)IupGetCallback(ih, "MENUCONTEXTCLOSE_CB");
  if (menucontext_cb)
    menucontext_cb(ih, menu, x, y);

  IupDestroy(menu);
}

void iupPlotSetPlotCurrent(Ihandle* ih, int p)
{
  ih->data->current_plot_index = p;
  ih->data->current_plot = ih->data->plot_list[ih->data->current_plot_index];
}

void iupPlotRedraw(Ihandle* ih, int flush, int only_current, int reset_redraw)
{
  if (ih->data->sync_view || ih->data->merge_view || ih->data->plot_list_count > 1)
    only_current = 0;

  IupDrawBegin(ih);

  iupPlotDrawContext ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.ih = ih;
  ctx.defaultFontStyle = ih->data->default_font_style;
  ctx.defaultFontSize = ih->data->default_font_size;

  if (only_current)
  {
    if (reset_redraw)
      ih->data->current_plot->mRedraw = true;

    ih->data->current_plot->PrepareRender(&ctx);
    ih->data->current_plot->Render(&ctx);
  }
  else
  {
    int old_current = ih->data->current_plot_index;
    int p;

    for (p = 0; p < ih->data->plot_list_count; p++)
    {
      iupPlotSetPlotCurrent(ih, p);

      if (reset_redraw)
        ih->data->current_plot->mRedraw = true;

      ih->data->current_plot->PrepareRender(&ctx);

      ih->data->current_plot->mBack.mTransparent = false;
    }

    if (ih->data->merge_view)
    {
      iupPlotSetPlotCurrent(ih, 0);
      iupPlot* plot0 = ih->data->current_plot;

      for (p = 1; p < ih->data->plot_list_count; p++)
      {
        iupPlotSetPlotCurrent(ih, p);

        if (ih->data->current_plot->mBack.mMargin.mLeft > plot0->mBack.mMargin.mLeft)
          plot0->mBack.mMargin.mLeft = ih->data->current_plot->mBack.mMargin.mLeft;
        if (ih->data->current_plot->mBack.mMargin.mRight > plot0->mBack.mMargin.mRight)
          plot0->mBack.mMargin.mRight = ih->data->current_plot->mBack.mMargin.mRight;
        if (ih->data->current_plot->mBack.mMargin.mTop > plot0->mBack.mMargin.mTop)
          plot0->mBack.mMargin.mTop = ih->data->current_plot->mBack.mMargin.mTop;
        if (ih->data->current_plot->mBack.mMargin.mBottom > plot0->mBack.mMargin.mBottom)
          plot0->mBack.mMargin.mBottom = ih->data->current_plot->mBack.mMargin.mBottom;

        if (ih->data->current_plot->mRedraw)
          plot0->mRedraw = true;
      }

      for (p = 1; p < ih->data->plot_list_count; p++)
      {
        iupPlotSetPlotCurrent(ih, p);

        ih->data->current_plot->mBack.mMargin.mLeft = plot0->mBack.mMargin.mLeft;
        ih->data->current_plot->mBack.mMargin.mRight = plot0->mBack.mMargin.mRight;
        ih->data->current_plot->mBack.mMargin.mTop = plot0->mBack.mMargin.mTop;
        ih->data->current_plot->mBack.mMargin.mBottom = plot0->mBack.mMargin.mBottom;

        ih->data->current_plot->mBack.mTransparent = true;

        if (plot0->mRedraw)
          ih->data->current_plot->mRedraw = true;
      }
    }

    for (p = 0; p < ih->data->plot_list_count; p++)
    {
      iupPlotSetPlotCurrent(ih, p);

      ih->data->current_plot->Render(&ctx);
    }

    iupPlotSetPlotCurrent(ih, old_current);
  }

  IupDrawEnd(ih);
}

static int iPlotAction_CB(Ihandle* ih)
{
  // in the redraw callback
  int flush = 1,  // always flush
    only_current = 0,  // redraw all plots
    reset_redraw = 0; // render only if necessary

  iupPlotRedraw(ih, flush, only_current, reset_redraw);

  return IUP_DEFAULT;
}

void iupPlotUpdateViewports(Ihandle* ih)
{
  int w, h;
  IupGetIntInt(ih, "DRAWSIZE", &w, &h);

  int numcol = ih->data->numcol;
  if (numcol > ih->data->plot_list_count) numcol = ih->data->plot_list_count;
  int numlin = ih->data->plot_list_count / numcol;

  int pw = w / numcol;
  int ph = h / numlin;

  for (int p = 0; p < ih->data->plot_list_count; p++)
  {
    int lin = p / numcol;
    int col = p % numcol;
    int px = col * pw;
    int py = lin * ph;

    if (ih->data->merge_view)  /* ignore computed values */
    {
      px = 0;
      py = 0;
      pw = w;
      ph = h;
    }

    ih->data->plot_list[p]->SetViewport(px, py, pw, ph);
  }
}

static int iPlotResize_CB(Ihandle* ih, int width, int height)
{
  (void)width;
  (void)height;
  iupPlotUpdateViewports(ih);
  return IUP_DEFAULT;
}

static void iPlotRedrawInteract(Ihandle *ih)
{
  // when interacting
  int flush = 0, // flush if necessary
    only_current = 1,
    reset_redraw = 0;  // render only if necessary

  if (ih->data->current_plot->mRedraw)
    flush = 1;

  iupPlotRedraw(ih, flush, only_current, reset_redraw);
}

void iupPlotResetZoom(Ihandle *ih, int redraw)
{
  ih->data->current_plot->ResetZoom();

  if (ih->data->sync_view)
  {
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
        ih->data->plot_list[p]->ResetZoom();
    }
  }

  if (redraw)
    iPlotRedrawInteract(ih);
}

static void iPlotPanStart(Ihandle *ih)
{
  ih->data->current_plot->PanStart();

  if (ih->data->sync_view)
  {
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
        ih->data->plot_list[p]->PanStart();
    }
  }
}

static void iPlotPan(Ihandle *ih, int x1, int y1, int x2, int y2)
{
  double rx1, ry1, rx2, ry2;
  ih->data->current_plot->TransformBack(x1, y1, rx1, ry1);
  ih->data->current_plot->TransformBack(x2, y2, rx2, ry2);

  double offsetX = rx2 - rx1;
  double offsetY = ry2 - ry1;

  ih->data->current_plot->Pan(offsetX, offsetY);

  if (ih->data->sync_view)
  {
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
      {
        ih->data->plot_list[p]->TransformBack(x1, y1, rx1, ry1);
        ih->data->plot_list[p]->TransformBack(x2, y2, rx2, ry2);

        offsetX = rx2 - rx1;
        offsetY = ry2 - ry1;

        ih->data->plot_list[p]->Pan(offsetX, offsetY);
      }
    }
  }

  iPlotRedrawInteract(ih);
}

static void iPlotZoom(Ihandle *ih, int x, int y, float delta)
{
  double rx, ry;
  ih->data->current_plot->TransformBack(x, y, rx, ry);

  if (delta > 0)
    ih->data->current_plot->ZoomIn(rx, ry);
  else
    ih->data->current_plot->ZoomOut(rx, ry);

  if (ih->data->sync_view)
  {
    for (int p = 0; p<ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
      {
        ih->data->plot_list[p]->TransformBack(x, y, rx, ry);

        if (delta > 0)
          ih->data->plot_list[p]->ZoomIn(rx, ry);
        else
          ih->data->plot_list[p]->ZoomOut(rx, ry);
      }
    }
  }

  iPlotRedrawInteract(ih);
}

void iupPlotSetZoom(Ihandle *ih, int dir)
{
  if (dir > 0)
  {
    int x = ih->data->current_plot->mViewport.mWidth / 2;
    int y = ih->data->current_plot->mViewport.mHeight / 2;
    iPlotZoom(ih, x, y, 1);
  }
  else if (dir < 0)
  {
    int x = ih->data->current_plot->mViewport.mWidth / 2;
    int y = ih->data->current_plot->mViewport.mHeight / 2;
    iPlotZoom(ih, x, y, -1);
  }
  else
    iupPlotResetZoom(ih, 1);
}

static void iPlotZoomTo(Ihandle *ih, int x1, int y1, int x2, int y2)
{
  double rx1, ry1, rx2, ry2;
  ih->data->current_plot->TransformBack(x1, y1, rx1, ry1);
  ih->data->current_plot->TransformBack(x2, y2, rx2, ry2);

  ih->data->current_plot->ZoomTo(rx1, rx2, ry1, ry2);

  if (ih->data->sync_view)
  {
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
      {
        ih->data->plot_list[p]->TransformBack(x1, y1, rx1, ry1);
        ih->data->plot_list[p]->TransformBack(x2, y2, rx2, ry2);

        ih->data->plot_list[p]->ZoomTo(rx1, rx2, ry1, ry2);
      }
    }
  }

  iPlotRedrawInteract(ih);
}

static void iPlotScroll(Ihandle *ih, float delta, bool full_page, bool vertical)
{
  ih->data->current_plot->Scroll(delta, full_page, vertical);

  if (ih->data->sync_view)
  {
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
        ih->data->plot_list[p]->Scroll(delta, full_page, vertical);
    }
  }

  iPlotRedrawInteract(ih);
}

static void iPlotScrollTo(Ihandle *ih, int x, int y)
{
  double rx, ry;
  ih->data->current_plot->TransformBack(x, y, rx, ry);

  ih->data->current_plot->ScrollTo(rx, ry);

  if (ih->data->sync_view)
  {
    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p] != ih->data->current_plot)
      {
        ih->data->plot_list[p]->TransformBack(x, y, rx, ry);
        ih->data->plot_list[p]->ScrollTo(rx, ry);
      }
    }
  }

  iPlotRedrawInteract(ih);
}

static int iPlotFindPlot(Ihandle* ih, int x, int &y, char* status)
{
  int w, h;
  IupGetIntInt(ih, "DRAWSIZE", &w, &h);

  if (ih->data->plot_list_count == 1)
    return 0;

  if (ih->data->merge_view)
  {
    if (ih->data->plot_list_count > 1 && iup_isshift(status))
      return 1;

    if (ih->data->plot_list_count > 2 && iup_iscontrol(status))
      return 2;

    if (ih->data->plot_list_count > 3 && iup_isalt(status))
      return 3;

    return 0;
  }

  int numcol = ih->data->numcol;
  if (numcol > ih->data->plot_list_count) numcol = ih->data->plot_list_count;
  int numlin = ih->data->plot_list_count / numcol;
  int pw = w / numcol;
  int ph = h / numlin;

  int lin = y / ph;
  int col = x / pw;

  int index = lin * numcol + col;
  if (index < ih->data->plot_list_count)
    return index;

  return -1;
}

static int iPlotButton_CB(Ihandle* ih, int button, int press, int x, int y, char* status)
{
  int screen_x = x, screen_y = y;
  int index = iPlotFindPlot(ih, x, y, status);
  if (index < 0)
  {
    iPlotRedrawInteract(ih);
    return IUP_DEFAULT;
  }

  iupPlotSetPlotCurrent(ih, index);

  if (ih->data->current_plot->mDataSetListCount == 0)
    return IUP_DEFAULT;

  x -= ih->data->current_plot->mViewport.mX;
  y = ih->data->current_plot->mViewport.mHeight - 1 - (y - ih->data->current_plot->mViewport.mY);

  IFniidds cb = (IFniidds)IupGetCallback(ih, "PLOTBUTTON_CB");
  if (cb)
  {
    double rx, ry;
    ih->data->current_plot->TransformBack(x, y, rx, ry);
    if (cb(ih, button, press, rx, ry, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  ih->data->last_pos_moving = 0;

  if (press)
  {
    ih->data->last_click_x = x;
    ih->data->last_click_y = y;
    ih->data->last_click_plot = index;

    if (button == IUP_BUTTON1)
    {
      if (!iup_iscontrol(status))
      {
        if (iup_isdouble(status))
          iupPlotResetZoom(ih, 1);
        else
        {
          if (!ih->data->current_plot->mTitle.mAutoPos &&
              ih->data->current_plot->CheckInsideTitle(ih, x, y))
          {
            ih->data->last_pos_x = ih->data->current_plot->mTitle.mPosX;
            ih->data->last_pos_y = ih->data->current_plot->mTitle.mPosY;
            ih->data->last_pos_moving = 1;
          }
          else if (ih->data->current_plot->mLegend.mPosition == IUP_PLOT_XY &&
                   ih->data->current_plot->CheckInsideLegend(x, y))
          {
            ih->data->last_pos_x = ih->data->current_plot->mLegend.mPos.mX;
            ih->data->last_pos_y = ih->data->current_plot->mLegend.mPos.mY;
            ih->data->last_pos_moving = 2;
          }
          else
            iPlotPanStart(ih);
        }
      }
    }
    else if (button == IUP_BUTTON2)
    {
      if (!iup_iscontrol(status))
        iPlotScrollTo(ih, x, y);
    }
    else if (button == IUP_BUTTON3 && !iup_iscontrol(status) && !iup_isshift(status) && IupGetInt(ih, "MENUCONTEXT"))
    {
      int sx, sy;
      IupGetIntInt(ih, "SCREENPOSITION", &sx, &sy);

      screen_x += sx;
      screen_y += sy;

      iupPlotShowMenuContext(ih, screen_x, screen_y, x, y);
    }
  }
  else
  {
    if (iup_iscontrol(status))
    {
      if (ih->data->last_click_x == x && ih->data->last_click_y == y && ih->data->last_click_plot == index)
      {
        float delta = 0;
        if (button == IUP_BUTTON1)
          delta = 1.0;
        else if (button == IUP_BUTTON3)
          delta = -1.0;

        if (delta)
          iPlotZoom(ih, x, y, delta);
      }
      else if (button == IUP_BUTTON1 && ih->data->last_click_x != x && ih->data->last_click_y != y && ih->data->last_click_plot == index)
      {
        iPlotZoomTo(ih, ih->data->last_click_x, ih->data->last_click_y, x, y);
      }
    }
    else if (button == IUP_BUTTON1 && iup_isshift(status))
    {
      double rx1, ry1, rx2, ry2;
      ih->data->current_plot->TransformBack(ih->data->last_click_x, ih->data->last_click_y, rx1, ry1);
      ih->data->current_plot->TransformBack(x, y, rx2, ry2);

      ih->data->current_plot->SelectDataSetSamples(rx1, rx2, ry1, ry2);

      iPlotRedrawInteract(ih);
    }

    if (!iup_iscontrol(status) && ih->data->last_click_x == x && ih->data->last_click_y == y && ih->data->last_click_plot == index)
    {

      int ds, sample1, sample2;
      double rx1, ry1, rx2, ry2;
      const char* ds_name;
      const char* strX;
      IFniiddi clicksample_cb = (IFniiddi)IupGetCallback(ih, "CLICKSAMPLE_CB");
      IFniiddiddi clicksegment_cb = (IFniiddiddi)IupGetCallback(ih, "CLICKSEGMENT_CB");
      if (clicksample_cb && ih->data->current_plot->FindDataSetSample((double)x, (double)y, ds, ds_name, sample1, rx1, ry1, strX))
        clicksample_cb(ih, ds, sample1, rx1, ry1, button);
      else if (clicksegment_cb && ih->data->current_plot->FindDataSetSegment((double)x, (double)y, ds, ds_name, sample1, rx1, ry1, sample2, rx2, ry2))
        clicksegment_cb(ih, ds, sample1, rx1, ry1, sample2, rx2, ry2, button);
    }
  }

  return IUP_DEFAULT;
}

static int iPlotMotion_CB(Ihandle* ih, int x, int y, char *status)
{
  if (iupStrEqualNoCase(IupGetAttribute(ih, "CURSOR"), "HAND"))
    IupSetAttribute(ih, "CURSOR", NULL);

  int index = iPlotFindPlot(ih, x, y, status);
  if (index < 0)
    return IUP_DEFAULT;

  iupPlotSetPlotCurrent(ih, index);

  if (ih->data->current_plot->mDataSetListCount == 0)
    return IUP_DEFAULT;

  x -= ih->data->current_plot->mViewport.mX;
  y = ih->data->current_plot->mViewport.mHeight - 1 - (y - ih->data->current_plot->mViewport.mY);

  //////////// PLOTMOTION_CB

  IFndds cb = (IFndds)IupGetCallback(ih, "PLOTMOTION_CB");
  if (cb)
  {
    double rx, ry;
    ih->data->current_plot->TransformBack(x, y, rx, ry);
    if (cb(ih, rx, ry, status) == IUP_IGNORE)
      return IUP_DEFAULT;
  }

  if (!ih->data->current_plot->mTitle.mAutoPos &&
      ih->data->current_plot->CheckInsideTitle(ih, x, y))
      IupSetAttribute(ih, "CURSOR", "HAND");
  else if (ih->data->current_plot->mLegend.mPosition == IUP_PLOT_XY &&
           ih->data->current_plot->CheckInsideLegend(x, y))
           IupSetAttribute(ih, "CURSOR", "HAND");

  /////////////// SELECTION, TITLE MOVE and LEGEND MOVE

  if (iup_isbutton1(status) && ih->data->last_click_plot == index)
  {
    if (iup_iscontrol(status) || iup_isshift(status))
    {
      ih->data->current_plot->mRedraw = true;
      ih->data->current_plot->mShowSelectionBand = true;
      ih->data->current_plot->mSelectionBand.mX = ih->data->last_click_x < x ? ih->data->last_click_x : x;
      ih->data->current_plot->mSelectionBand.mY = ih->data->last_click_y < y ? ih->data->last_click_y : y;
      ih->data->current_plot->mSelectionBand.mWidth = abs(ih->data->last_click_x - x) + 1;
      ih->data->current_plot->mSelectionBand.mHeight = abs(ih->data->last_click_y - y) + 1;

      iPlotRedrawInteract(ih);

      ih->data->current_plot->mShowSelectionBand = false;
      return IUP_DEFAULT;
    }
    else
    {
      if (ih->data->last_click_x != x || ih->data->last_click_y != y)
      {
        if (!ih->data->current_plot->mTitle.mAutoPos && ih->data->last_pos_moving == 1)
        {
          ih->data->current_plot->mTitle.mPosX = ih->data->last_pos_x + (x - ih->data->last_click_x);
          ih->data->current_plot->mTitle.mPosY = ih->data->last_pos_y + (y - ih->data->last_click_y);
          ih->data->current_plot->mRedraw = true;

          iPlotRedrawInteract(ih);
          return IUP_DEFAULT;
        }

        if (ih->data->current_plot->mLegend.mPosition == IUP_PLOT_XY && ih->data->last_pos_moving == 2)
        {
          ih->data->current_plot->mLegend.mPos.mX = ih->data->last_pos_x + (x - ih->data->last_click_x);
          ih->data->current_plot->mLegend.mPos.mY = ih->data->last_pos_y + (y - ih->data->last_click_y);
          ih->data->current_plot->mRedraw = true;

          iPlotRedrawInteract(ih);
          return IUP_DEFAULT;
        }

        iPlotPan(ih, ih->data->last_click_x, ih->data->last_click_y, x, y);
        return IUP_DEFAULT;
      }
    }
  }

  /////////////// SAMPLE TIPS and HIGHLIGHT

  bool changed = false;
  bool found = false;
  bool redraw = false;
  int ds, sample, sample1, sample2;
  double rx, ry, rx1, ry1, rx2, ry2;
  const char* ds_name;
  const char* strX;
  int prev_cursor_plot = ih->data->last_cursor_plot;
  if (ih->data->current_plot->FindDataSetSample((double)x, (double)y, ds, ds_name, sample, rx, ry, strX))
  {
    found = true;

    if (ih->data->last_cursor_plot != index ||
        ih->data->last_cursor_ds != ds ||
        ih->data->last_cursor_sample != sample)
    {
      char* tipformat = iupAttribGetStr(ih, "TIPFORMAT");

      char str_Y[100];
      iupStrPrintfDoubleLocale(str_Y, ih->data->current_plot->mAxisY.mTipFormatString, ry, IupGetGlobal("DEFAULTDECIMALSYMBOL"));

      if (strX)
        IupSetfAttribute(ih, "TIP", tipformat, ds_name, strX, str_Y);
      else
      {
        char str_X[100];
        iupStrPrintfDoubleLocale(str_X, ih->data->current_plot->mAxisX.mTipFormatString, rx, IupGetGlobal("DEFAULTDECIMALSYMBOL"));
        IupSetfAttribute(ih, "TIP", tipformat, ds_name, str_X, str_Y);
      }

      IupSetAttribute(ih, "TIPVISIBLE", "Yes");

      ih->data->last_cursor_plot = index;
      ih->data->last_cursor_ds = ds;
      ih->data->last_cursor_sample = sample;
      changed = true;
    }
  }
  else
  {
    if (ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_CURVE || ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_BOTH)
    {
      if (ih->data->current_plot->FindDataSetSegment((double)x, (double)y, ds, ds_name, sample1, rx1, ry1, sample2, rx2, ry2))
      {
        found = true;

        if (ih->data->last_cursor_plot != index ||
            ih->data->last_cursor_ds != ds ||
            ih->data->last_cursor_sample != -1)
        {
          ih->data->last_cursor_plot = index;
          ih->data->last_cursor_ds = ds;
          ih->data->last_cursor_sample = -1;
          changed = true;
        }
      }
    }
  }

  if (prev_cursor_plot != -1 &&
      ih->data->last_cursor_ds != -1)
  {
    if ((!found || changed) && ih->data->current_plot->mHighlightMode != IUP_PLOT_HIGHLIGHT_NONE)
    {
      redraw = true;
      ih->data->plot_list[prev_cursor_plot]->ClearHighlight();
    }

    if (!found)
    {
      ih->data->last_cursor_plot = -1;
      ih->data->last_cursor_ds = -1;
      ih->data->last_cursor_sample = -1;

      IupSetAttribute(ih, "TIP", NULL);
      IupSetAttribute(ih, "TIPVISIBLE", "Yes");
    }
    else if (changed)
    {
      if (ih->data->current_plot->mHighlightMode != IUP_PLOT_HIGHLIGHT_NONE)
      {
        redraw = true;

        if (ih->data->last_cursor_sample != -1)
        {
          // priority for sample highlight when sample is found
          if (ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_SAMPLE ||
              ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_BOTH)
              ih->data->current_plot->mDataSetList[ih->data->last_cursor_ds]->mHighlightedSample = ih->data->last_cursor_sample;

          // highlight a curve
          if (ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_CURVE)
            ih->data->current_plot->mDataSetList[ih->data->last_cursor_ds]->mHighlightedCurve = true;
        }
        else
        {
          // highlight a curve
          if (ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_CURVE ||
              ih->data->current_plot->mHighlightMode == IUP_PLOT_HIGHLIGHT_BOTH)
              ih->data->current_plot->mDataSetList[ih->data->last_cursor_ds]->mHighlightedCurve = true;
        }
      }
    }
  }

  if (ih->data->show_cross_hair)
  {
    redraw = true;

    if (ih->data->show_cross_hair == IUP_PLOT_CROSSHAIR_HORIZ)
    {
      ih->data->current_plot->mCrossHairH = true;
      ih->data->current_plot->mCrossHairX = x;
    }
    else
    {
      ih->data->current_plot->mCrossHairV = true;
      ih->data->current_plot->mCrossHairY = y;
    }
  }

  if (redraw)
  {
    ih->data->current_plot->mRedraw = true;
    iPlotRedrawInteract(ih);
  }

  return IUP_DEFAULT;
}

static int iPlotWheel_CB(Ihandle *ih, float delta, int x, int y, char* status)
{
  int index = iPlotFindPlot(ih, x, y, status);
  if (index < 0)
    return IUP_DEFAULT;

  iupPlotSetPlotCurrent(ih, index);

  if (ih->data->current_plot->mDataSetListCount == 0)
    return IUP_DEFAULT;

  x -= ih->data->current_plot->mViewport.mX;
  y = ih->data->current_plot->mViewport.mHeight - 1 - (y - ih->data->current_plot->mViewport.mY);

  if (iup_iscontrol(status))
    iPlotZoom(ih, x, y, delta);
  else
  {
    bool vertical = true;
    if (iup_isshift(status))
      vertical = false;

    iPlotScroll(ih, delta, false, vertical);
  }

  return IUP_DEFAULT;
}

static int iPlotKeyPress_CB(Ihandle* ih, int c, int press)
{
  if (!press)
    return IUP_DEFAULT;

  if (c == K_cH || c == K_cV)
  {
    int new_show_cross_hair = IUP_PLOT_CROSSHAIR_HORIZ;
    if (c == K_cV) new_show_cross_hair = IUP_PLOT_CROSSHAIR_VERT;

    if (ih->data->show_cross_hair == new_show_cross_hair)
      ih->data->show_cross_hair = IUP_PLOT_CROSSHAIR_NONE;
    else
      ih->data->show_cross_hair = new_show_cross_hair;

    for (int p = 0; p < ih->data->plot_list_count; p++)
    {
      if (ih->data->plot_list[p]->mCrossHairH)
      {
        ih->data->plot_list[p]->mRedraw = true;
        ih->data->plot_list[p]->mCrossHairH = false;
      }
      if (ih->data->plot_list[p]->mCrossHairV)
      {
        ih->data->plot_list[p]->mRedraw = true;
        ih->data->plot_list[p]->mCrossHairV = false;
      }
    }

    if (ih->data->show_cross_hair != IUP_PLOT_CROSSHAIR_NONE)  // was shown, leave it there as reference
      iPlotRedrawInteract(ih);

    return IUP_IGNORE;  /* ignore processed keys */
  }

  if (ih->data->current_plot->mDataSetListCount == 0)
    return IUP_DEFAULT;

  if (c == K_plus)
  {
    int x = ih->data->current_plot->mViewport.mWidth / 2;
    int y = ih->data->current_plot->mViewport.mHeight / 2;
    iPlotZoom(ih, x, y, 1);
    return IUP_IGNORE;  /* ignore processed keys */
  }
  else if (c == K_minus)
  {
    int x = ih->data->current_plot->mViewport.mWidth / 2;
    int y = ih->data->current_plot->mViewport.mHeight / 2;
    iPlotZoom(ih, x, y, -1);
    return IUP_IGNORE;  /* ignore processed keys */
  }
  else if (c == K_period)
  {
    iupPlotResetZoom(ih, 1);
    return IUP_IGNORE;  /* ignore processed keys */
  }
  else if (c == K_LEFT || c == K_RIGHT)
  {
    float delta = 1.0f;
    if (c == K_LEFT) delta = -1.0f;
    iPlotScroll(ih, delta, false, false);
    return IUP_IGNORE;  /* ignore processed keys */
  }
  else if (c == K_UP || c == K_DOWN || c == K_PGUP || c == K_PGDN)
  {
    float delta = 1.0f;
    if (c == K_DOWN || c == K_PGDN) delta = -1.0f;

    bool full_page = false;
    if (c == K_PGUP || c == K_PGDN) full_page = true;

    iPlotScroll(ih, delta, full_page, true);
    return IUP_IGNORE;  /* ignore processed keys */
  }
  else if (c == K_DEL)
  {
    if (!ih->data->read_only)
    {
      ih->data->current_plot->DeleteSelectedDataSetSamples();
      iPlotRedrawInteract(ih);
      return IUP_IGNORE;  /* ignore processed keys */
    }
  }
  else if (c == K_ESC)
  {
    ih->data->current_plot->ClearDataSetSelection();
    iPlotRedrawInteract(ih);
    return IUP_IGNORE;  /* ignore processed keys */
  }

  return IUP_DEFAULT;
}

/************************************************************************************/

IUPPLOT_API void IupPlotBegin(Ihandle* ih, int strXdata)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  iupPlotDataSet* theDataSet = (iupPlotDataSet*)iupAttribGet(ih, "_IUP_PLOT_DATASET");
  if (theDataSet)
    delete theDataSet;

  theDataSet = new iupPlotDataSet(strXdata ? true : false);
  iupAttribSet(ih, "_IUP_PLOT_DATASET", (char*)theDataSet);
}

IUPPLOT_API void IupPlotAdd(Ihandle* ih, double x, double y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  iupPlotDataSet* theDataSet = (iupPlotDataSet*)iupAttribGet(ih, "_IUP_PLOT_DATASET");
  theDataSet->AddSample(x, y);
}

IUPPLOT_API void IupPlotAddStr(Ihandle* ih, const char* x, double y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  iupPlotDataSet* theDataSet = (iupPlotDataSet*)iupAttribGet(ih, "_IUP_PLOT_DATASET");
  theDataSet->AddSample(x, y);
}

IUPPLOT_API void IupPlotAddSegment(Ihandle* ih, double x, double y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  iupPlotDataSet* theDataSet = (iupPlotDataSet*)iupAttribGet(ih, "_IUP_PLOT_DATASET");
  theDataSet->AddSampleSegment(x, y, true);
}

IUPPLOT_API int IupPlotEnd(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return -1;

  iupPlotDataSet* theDataSet = (iupPlotDataSet*)iupAttribGet(ih, "_IUP_PLOT_DATASET");
  if (!theDataSet)
    return -1;

  ih->data->current_plot->AddDataSet(theDataSet);

  iupAttribSet(ih, "_IUP_PLOT_DATASET", NULL);

  ih->data->current_plot->mRedraw = true;
  return ih->data->current_plot->mCurrentDataSet;
}

IUPPLOT_API void IupPlotInsert(Ihandle* ih, int inIndex, int inSampleIndex, double inX, double inY)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->InsertSample(inSampleIndex, inX, inY);
}

IUPPLOT_API void IupPlotInsertStr(Ihandle* ih, int inIndex, int inSampleIndex, const char* inX, double inY)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->InsertSample(inSampleIndex, inX, inY);
}

IUPPLOT_API void IupPlotInsertSegment(Ihandle* ih, int inIndex, int inSampleIndex, double inX, double inY)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->InsertSampleSegment(inSampleIndex, inX, inY, true);
}

IUPPLOT_API void IupPlotAddSamples(Ihandle* ih, int inIndex, double *x, double *y, int count)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  for (int i = 0; i < count; i++)
    theDataSet->AddSample(x[i], y[i]);
}

IUPPLOT_API void IupPlotAddStrSamples(Ihandle* ih, int inIndex, const char** x, double* y, int count)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  for (int i = 0; i < count; i++)
    theDataSet->AddSample(x[i], y[i]);
}

IUPPLOT_API void IupPlotInsertStrSamples(Ihandle* ih, int inIndex, int inSampleIndex, const char** inX, double* inY, int count)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];

  for (int i = 0; i < count; i++)
    theDataSet->InsertSample(inSampleIndex + i, inX[i], inY[i]);
}

IUPPLOT_API void IupPlotInsertSamples(Ihandle* ih, int inIndex, int inSampleIndex, double *inX, double *inY, int count)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];

  for (int i = 0; i < count; i++)
    theDataSet->InsertSample(inSampleIndex + i, inX[i], inY[i]);
}

IUPPLOT_API void IupPlotGetSample(Ihandle* ih, int inIndex, int inSampleIndex, double *x, double *y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->GetSample(inSampleIndex, x, y);
}

IUPPLOT_API void IupPlotGetSampleStr(Ihandle* ih, int inIndex, int inSampleIndex, const char* *x, double *y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->GetSample(inSampleIndex, x, y);
}

IUPPLOT_API int IupPlotGetSampleSelection(Ihandle* ih, int inIndex, int inSampleIndex)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return -1;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return -1;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];

  int theCount = theDataSet->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return -1;

  return theDataSet->GetSampleSelection(inSampleIndex);
}

IUPPLOT_API double IupPlotGetSampleExtra(Ihandle* ih, int inIndex, int inSampleIndex)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return -1;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return -1;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];

  int theCount = theDataSet->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return -1;

  return theDataSet->GetSampleExtra(inSampleIndex);
}

IUPPLOT_API void IupPlotSetSample(Ihandle* ih, int inIndex, int inSampleIndex, double x, double y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->SetSample(inSampleIndex, x, y);
}

IUPPLOT_API void IupPlotSetSampleStr(Ihandle* ih, int inIndex, int inSampleIndex, const char* x, double y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];
  theDataSet->SetSample(inSampleIndex, x, y);
}

IUPPLOT_API void IupPlotSetSampleSelection(Ihandle* ih, int inIndex, int inSampleIndex, int inSelected)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];

  int theCount = theDataSet->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  return theDataSet->SetSampleSelection(inSampleIndex, inSelected ? true : false);
}

IUPPLOT_API void IupPlotSetSampleExtra(Ihandle* ih, int inIndex, int inSampleIndex, double inExtra)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (inIndex < 0 || inIndex >= ih->data->current_plot->mDataSetListCount)
    return;

  iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[inIndex];

  int theCount = theDataSet->GetCount();
  if (inSampleIndex < 0 || inSampleIndex >= theCount)
    return;

  return theDataSet->SetSampleExtra(inSampleIndex, inExtra);
}

IUPPLOT_API void IupPlotTransform(Ihandle* ih, double x, double y, double *cnv_x, double *cnv_y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (cnv_x) *cnv_x = ih->data->current_plot->mAxisX.mTrafo->Transform(x);
  if (cnv_y) *cnv_y = ih->data->current_plot->mAxisY.mTrafo->Transform(y);
}

IUPPLOT_API void IupPlotTransformTo(Ihandle* ih, double cnv_x, double cnv_y, double *x, double *y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return;

  if (x) *x = ih->data->current_plot->mAxisX.mTrafo->TransformBack(cnv_x);
  if (y) *y = ih->data->current_plot->mAxisY.mTrafo->TransformBack(cnv_y);
}


static const char* iPlotSkipValue(const char* line_buffer)
{
  // fix next separator
  char ch = *line_buffer;
  while (ch != ' ' && ch != '\t' && ch != ';' && ch != 0)
  {
    line_buffer++;
    ch = *line_buffer;
  }

  // skip separators
  while ((ch == ' ' || ch == '\t' || ch == ';') && ch != 0)
  {
    line_buffer++;
    ch = *line_buffer;
  }

  return line_buffer;
}

static int iPlotCountDataSets(const char* line_buffer)
{
  int ds_count = 0;

  while (*line_buffer != 0)
  {
    line_buffer = iPlotSkipValue(line_buffer);
    ds_count++;
  }

  return ds_count;
}

static int iPlotAddToDataSets(Ihandle* ih, const char* line_buffer, int ds_start, int ds_count)
{
  double x = 0;
  double value;

  for (int ds = 0; ds < ds_count; ds++)
  {
    int ret = sscanf(line_buffer, "%lf", &value);
    if (!ret)
      return 0;

    line_buffer = iPlotSkipValue(line_buffer);

    if (ds == 0)
      x = value;
    else
    {
      double y = value;

      iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[ds_start + ds - 1];
      theDataSet->AddSample(x, y);
    }
  }

  return 1;
}

static int iPlotAddToDataSetsStrX(Ihandle* ih, const char* line_buffer, int ds_start, int ds_count)
{
  char x[100] = "";
  double value = 0;

  for (int ds = 0; ds < ds_count; ds++)
  {
    if (ds == 0)
    {
      int ret = sscanf(line_buffer, "%s", x);
      if (!ret)
        return 0;
    }
    else
    {
      int ret = sscanf(line_buffer, "%lf", &value);
      if (!ret)
        return 0;
    }

    line_buffer = iPlotSkipValue(line_buffer);

    if (ds != 0)
    {
      double y = value;

      iupPlotDataSet* theDataSet = ih->data->current_plot->mDataSetList[ds_start + ds - 1];
      theDataSet->AddSample(x, y);
    }
  }

  return 1;
}

static int iPlotLoadDataFile(Ihandle* ih, IlineFile* line_file, int strXdata)
{
  int first_line = 1;
  int ds_count = 0;
  int ds, ds_start = ih->data->current_plot->mDataSetListCount;

  do
  {
    int line_len = iupLineFileReadLine(line_file);
    if (line_len == -1)
      return 0;

    const char* line_buffer = iupLineFileGetBuffer(line_file);

    int i = 0;
    while (line_buffer[i] == ' ') /* ignore spaces at start */
      i++;

    if (line_buffer[i] == 0) /* skip empty line */
      continue;

    if (line_buffer[i] == '#') /* "#" signifies a comment line when used as the first non-space character on a line */
      continue;

    if (first_line)
    {
      ds_count = iPlotCountDataSets(line_buffer);
      if (ds_count < 2) // must have at least X and Y1, could have Y2, Y3, ...
        return 0;

      for (ds = 0; ds < ds_count - 1; ds++)
      {
        iupPlotDataSet* theDataSet = new iupPlotDataSet(strXdata ? true : false);
        ih->data->current_plot->AddDataSet(theDataSet);
      }

      first_line = 0;
    }

    if (strXdata)
    {
      if (!iPlotAddToDataSetsStrX(ih, line_buffer + i, ds_start, ds_count))
        return 0;
    }
    else
    {
      if (!iPlotAddToDataSets(ih, line_buffer + i, ds_start, ds_count))
        return 0;
    }

  } while (!iupLineFileEOF(line_file));

  return 1;
}

IUPPLOT_API int IupPlotLoadData(Ihandle* ih, const char* filename, int strXdata)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return 0;

  if (!filename)
    return 0;

  IlineFile* line_file = iupLineFileOpen(filename);
  if (!line_file)
    return 0;

  int error = iPlotLoadDataFile(ih, line_file, strXdata);

  iupLineFileClose(line_file);

  return error;
}

IUPPLOT_API int IupPlotFindSample(Ihandle* ih, double cnv_x, double cnv_y, int *ds_index, int *sample_index)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return 0;

  int ds, sample;
  double rx, ry;
  const char* ds_name;
  const char* strX;
  if (ih->data->current_plot->FindDataSetSample(cnv_x, cnv_y, ds, ds_name, sample, rx, ry, strX))
  {
    if (ds_index) *ds_index = ds;
    if (sample_index) *sample_index = sample;
    return 1;
  }

  return 0;
}

IUPPLOT_API int  IupPlotFindSegment(Ihandle* ih, double cnv_x, double cnv_y, int *ds_index, int *sample_index1, int *sample_index2)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return 0;

  if (ih->iclass->nativetype != IUP_TYPECANVAS ||
      !IupClassMatch(ih, "plot"))
      return 0;

  int ds, sample1, sample2;
  double rx1, ry1;
  double rx2, ry2;
  const char* ds_name;
  if (ih->data->current_plot->FindDataSetSegment(cnv_x, cnv_y, ds, ds_name, sample1, rx1, ry1, sample2, rx2, ry2))
  {
    if (ds_index) *ds_index = ds;
    if (sample_index1) *sample_index1 = sample1;
    if (sample_index2) *sample_index2 = sample2;
    return 1;
  }

  return 0;
}

/************************************************************************************/

static int iPlotMapMethod(Ihandle* ih)
{
  for (int p = 0; p < ih->data->plot_list_count; p++)
    ih->data->plot_list[p]->mRedraw = true;

  iupPlotUpdateViewports(ih);

  return IUP_NOERROR;
}

static void iPlotUnMapMethod(Ihandle* ih)
{
  (void)ih;
}

static void iPlotDestroyMethod(Ihandle* ih)
{
  for (int p = 0; p < ih->data->plot_list_count; p++)
    delete ih->data->plot_list[p];

  iupPlotDataSet* theDataSet = (iupPlotDataSet*)iupAttribGet(ih, "_IUP_PLOT_DATASET");
  if (theDataSet)
    delete theDataSet;
}

static int iPlotCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  /* free the data allocated by IupCanvas */
  free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  ih->data->read_only = 1;
  ih->data->plot_list_count = 1;
  ih->data->numcol = 1;
  ih->data->last_cursor_ds = -1;
  ih->data->last_cursor_sample = -1;
  ih->data->last_cursor_plot = -1;
  ih->data->last_click_plot = -1;
  ih->data->plot_list[0] = new iupPlot(ih, 0, 0);   // font style/size will be initialized by font initialization
  ih->data->current_plot = ih->data->plot_list[ih->data->current_plot_index];

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION", (Icallback)iPlotAction_CB);
  IupSetCallback(ih, "RESIZE_CB", (Icallback)iPlotResize_CB);
  IupSetCallback(ih, "BUTTON_CB", (Icallback)iPlotButton_CB);
  IupSetCallback(ih, "MOTION_CB", (Icallback)iPlotMotion_CB);
  IupSetCallback(ih, "WHEEL_CB", (Icallback)iPlotWheel_CB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iPlotKeyPress_CB);

  return IUP_NOERROR;
}

#include "iup_lng_english_plot.h"
#include "iup_lng_portuguese_plot.h"
#include "iup_lng_spanish_plot.h"
#include "iup_lng_czech_plot.h"
#include "iup_lng_russian_plot.h"
#include "iup_lng_german_plot.h"
#include "iup_lng_french_plot.h"
#include "iup_lng_chinese_plot.h"
#include "iup_lng_japanese_plot.h"
#include "iup_lng_italian_plot.h"

static void iPlotSetClassUpdate(Iclass* ic)
{
  Ihandle* lng = NULL;

  (void)ic;

  if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ENGLISH"))
  {
    lng = iup_load_lng_english_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "PORTUGUESE"))
  {
    lng = iup_load_lng_portuguese_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "SPANISH"))
  {
    lng = iup_load_lng_spanish_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "CZECH"))
  {
    lng = iup_load_lng_czech_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "RUSSIAN"))
  {
    lng = iup_load_lng_russian_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "GERMAN"))
  {
    lng = iup_load_lng_german_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "FRENCH"))
  {
    lng = iup_load_lng_french_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "CHINESE"))
  {
    lng = iup_load_lng_chinese_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "JAPANESE"))
  {
    lng = iup_load_lng_japanese_plot();
  }
  else if (iupStrEqualNoCase(IupGetGlobal("LANGUAGE"), "ITALIAN"))
  {
    lng = iup_load_lng_italian_plot();
  }

  if (lng)
  {
    IupSetLanguagePack(lng);
    IupDestroy(lng);
  }
}

static Iclass* iPlotNewClass(void)
{
  Iclass* ic = iupClassNew(iupRegisterFindClass("canvas"));

  ic->name = (char*)"plot";
  ic->format = NULL;  /* none */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;

  /* Class functions */
  ic->New = iPlotNewClass;
  ic->Create = iPlotCreateMethod;
  ic->Destroy = iPlotDestroyMethod;
  ic->Map = iPlotMapMethod;
  ic->UnMap = iPlotUnMapMethod;

  /* IupPlot Callbacks */
  iupClassRegisterCallback(ic, "POSTDRAW_CB", "");
  iupClassRegisterCallback(ic, "PREDRAW_CB", "");
  iupClassRegisterCallback(ic, "CLICKSAMPLE_CB", "iiddi");
  iupClassRegisterCallback(ic, "CLICKSEGMENT_CB", "iiddiddi");
  iupClassRegisterCallback(ic, "DRAWSAMPLE_CB", "iiddi");
  iupClassRegisterCallback(ic, "PLOTMOTION_CB", "dds");
  iupClassRegisterCallback(ic, "PLOTBUTTON_CB", "iidds");
  iupClassRegisterCallback(ic, "EDITSAMPLE_CB", "iidd");
  iupClassRegisterCallback(ic, "DELETE_CB", "iidd");
  iupClassRegisterCallback(ic, "DELETEBEGIN_CB", "");
  iupClassRegisterCallback(ic, "DELETEEND_CB", "");
  iupClassRegisterCallback(ic, "SELECT_CB", "iiddi");
  iupClassRegisterCallback(ic, "SELECTBEGIN_CB", "");
  iupClassRegisterCallback(ic, "SELECTEND_CB", "");
  iupClassRegisterCallback(ic, "MENUCONTEXT_CB", "nii");
  iupClassRegisterCallback(ic, "MENUCONTEXTCLOSE_CB", "nii");
  iupClassRegisterCallback(ic, "PROPERTIESCHANGED_CB", "");
  iupClassRegisterCallback(ic, "PROPERTIESVALIDATE_CB", "ss");
  iupClassRegisterCallback(ic, "DSPROPERTIESCHANGED_CB", "i");
  iupClassRegisterCallback(ic, "DSPROPERTIESVALIDATE_CB", "nni");
  iupClassRegisterCallback(ic, "XTICKFORMATNUMBER_CB", "ssds");
  iupClassRegisterCallback(ic, "YTICKFORMATNUMBER_CB", "ssds");

  iupPlotRegisterAttributes(ic);

  iupClassRegisterAttribute(ic, "CLASSUPDATE", NULL, (IattribSetFunc)iPlotSetClassUpdate, NULL, NULL, IUPAF_WRITEONLY | IUPAF_NO_INHERIT);

  iPlotSetClassUpdate(ic);

  return ic;
}

IUPPLOT_API Ihandle* IupPlot(void)
{
  return IupCreate("plot");
}

IUPPLOT_API void IupPlotOpen(void)
{
  if (!IupIsOpened())
    return;

  if (!IupGetGlobal("_IUP_PLOT_OPEN"))
  {
    iupRegisterClass(iPlotNewClass());
    IupSetGlobal("_IUP_PLOT_OPEN", "1");
  }
}
