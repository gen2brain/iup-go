/** \file
 * \brief Label Control
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdarg.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_attrib.h"
#include "iup_dialog.h"
#include "iup_str.h"
#include "iup_label.h"
#include "iup_drv.h"
#include "iup_image.h"

#include "iupmot_drv.h"


void iupdrvLabelAddExtraPadding(Ihandle* ih, int *x, int *y)
{
  (void)ih;
  (void)x;
  (void)y;
}

static int motLabelSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_TEXT)
  {
    iupmotSetMnemonicTitle(ih, NULL, 0, value);
    return 1;
  }

  return 0;
}

static int motLabelSetBgColorAttrib(Ihandle* ih, const char* value)
{
  /* ignore given value, must use only from parent */
  value = iupBaseNativeParentGetBgColor(ih);

  if (iupdrvBaseSetBgColorAttrib(ih, value))
    return 1;
  return 0; 
}

static int motLabelSetBackgroundAttrib(Ihandle* ih, const char* value)
{
  /* ignore given value, must use only from parent */
  value = iupAttribGetInheritNativeParent(ih, "BACKGROUND");

  if (iupdrvBaseSetBgColorAttrib(ih, value))
    return 1;
  else
  {
    Pixmap pixmap = (Pixmap)iupImageGetImage(value, ih, 0, NULL);
    if (pixmap)
    {
      XtVaSetValues(ih->handle, XmNbackgroundPixmap, pixmap, NULL);
      return 1;
    }
  }
  return 0; 
}

static int motLabelSetAlignmentAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    unsigned char align;
    char value1[30], value2[30];

    iupStrToStrStr(value, value1, value2, ':');   /* value2 is ignored, NOT supported in Motif */

    if (iupStrEqualNoCase(value1, "ARIGHT"))
      align = XmALIGNMENT_END;
    else if (iupStrEqualNoCase(value1, "ACENTER"))
      align = XmALIGNMENT_CENTER;
    else /* "ALEFT" (default) */
      align = XmALIGNMENT_BEGINNING;

    XtVaSetValues(ih->handle, XmNalignment, align, NULL);
    return 1;
  }
  else
    return 0;
}

static int motLabelSetImageAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelPixmap, 0);

    if (!iupdrvIsActive(ih))
    {
      if (!iupAttribGet(ih, "IMINACTIVE"))
      {
        /* if not active and IMINACTIVE is not defined 
           then automatically create one based on IMAGE */
        iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 1); /* make_inactive */
      }
    }
    return 1;
  }
  else
    return 0;
}

static int motLabelSetImInactiveAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->type == IUP_LABEL_IMAGE)
  {
    iupmotSetPixmap(ih, value, XmNlabelInsensitivePixmap, 0);
    return 1;
  }
  else
    return 0;
}

static int motLabelSetActiveAttrib(Ihandle* ih, const char* value)
{
  /* update the inactive image if necessary */
  if (ih->data->type == IUP_LABEL_IMAGE && !iupStrBoolean(value))
  {
    if (!iupAttribGet(ih, "IMINACTIVE"))
    {
      /* if not defined then automatically create one based on IMAGE */
      char* name = iupAttribGet(ih, "IMAGE");
      iupmotSetPixmap(ih, name, XmNlabelInsensitivePixmap, 1); /* make_inactive */
    }
  }

  return iupBaseSetActiveAttrib(ih, value);
}

static int motLabelSetPaddingAttrib(Ihandle* ih, const char* value)
{
  iupStrToIntInt(value, &ih->data->horiz_padding, &ih->data->vert_padding, 'x');
  if (ih->handle && ih->data->type != IUP_LABEL_SEP_HORIZ && ih->data->type != IUP_LABEL_SEP_VERT)
  {
    XtVaSetValues(ih->handle, XmNmarginHeight, ih->data->vert_padding,
                              XmNmarginWidth, ih->data->horiz_padding, NULL);
    return 0;
  }
  else
    return 1; /* store until not mapped, when mapped will be set again */
}

static int motLabelMapMethod(Ihandle* ih)
{
  char* value;
  int num_args = 0;
  Arg args[20];
  WidgetClass widget_class;

  value = iupAttribGet(ih, "SEPARATOR");
  if (value)
  {
    widget_class = xmSeparatorWidgetClass;
    if (iupStrEqualNoCase(value, "HORIZONTAL"))
    {
      ih->data->type = IUP_LABEL_SEP_HORIZ;
      iupMOT_SETARG(args, num_args, XmNorientation, XmHORIZONTAL);
    }
    else /* "VERTICAL" */
    {
      ih->data->type = IUP_LABEL_SEP_VERT;
      iupMOT_SETARG(args, num_args, XmNorientation, XmVERTICAL);
    }
  }
  else
  {
    value = iupAttribGet(ih, "IMAGE");
    widget_class = xmLabelWidgetClass;
    if (value)
    {
      ih->data->type = IUP_LABEL_IMAGE;
      iupMOT_SETARG(args, num_args, XmNlabelType, XmPIXMAP); 
    }
    else
    {
      ih->data->type = IUP_LABEL_TEXT;
      iupMOT_SETARG(args, num_args, XmNlabelType, XmSTRING); 
    }
  }

  /* Core */
  iupMOT_SETARG(args, num_args, XmNmappedWhenManaged, False);  /* not visible when managed */
  iupMOT_SETARG(args, num_args, XmNx, 0);  /* x-position */
  iupMOT_SETARG(args, num_args, XmNy, 0);  /* y-position */
  iupMOT_SETARG(args, num_args, XmNwidth, 10);  /* default width to avoid 0 */
  iupMOT_SETARG(args, num_args, XmNheight, 10); /* default height to avoid 0 */
  /* Primitive */
  iupMOT_SETARG(args, num_args, XmNtraversalOn, False);
  iupMOT_SETARG(args, num_args, XmNhighlightThickness, 0);
  /* Label */
  iupMOT_SETARG(args, num_args, XmNrecomputeSize, False);  /* no automatic resize from text */
  iupMOT_SETARG(args, num_args, XmNmarginHeight, 0);  /* default padding */
  iupMOT_SETARG(args, num_args, XmNmarginWidth, 0);
  iupMOT_SETARG(args, num_args, XmNmarginTop, 0);     /* no extra margins */
  iupMOT_SETARG(args, num_args, XmNmarginLeft, 0);
  iupMOT_SETARG(args, num_args, XmNmarginBottom, 0);
  iupMOT_SETARG(args, num_args, XmNmarginRight, 0);
  
  ih->handle = XtCreateManagedWidget(
    iupDialogGetChildIdStr(ih),  /* child identifier */
    widget_class,                /* widget class */
    iupChildTreeGetNativeParentHandle(ih), /* widget parent */
    args, num_args);

  if (!ih->handle)
    return IUP_ERROR;

  ih->serial = iupDialogGetChildId(ih); /* must be after using the string */

  XtAddEventHandler(ih->handle, EnterWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, LeaveWindowMask, False, (XtEventHandler)iupmotEnterLeaveWindowEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, ButtonPressMask|ButtonReleaseMask, False, (XtEventHandler)iupmotButtonPressReleaseEvent, (XtPointer)ih);
  XtAddEventHandler(ih->handle, PointerMotionMask, False, (XtEventHandler)iupmotPointerMotionEvent, (XtPointer)ih);

  /* Drag Source is enabled by default in label */
  iupmotDisableDragSource(ih->handle);

  /* initialize the widget */
  XtRealizeWidget(ih->handle);

  if (ih->data->type == IUP_LABEL_TEXT)
    iupmotSetXmString(ih->handle, XmNlabelString, "");

  return IUP_NOERROR;
}

void iupdrvLabelInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = motLabelMapMethod;

  /* Driver Dependent Attribute functions */

  /* Overwrite Visual */
  iupClassRegisterAttribute(ic, "ACTIVE", iupBaseGetActiveAttrib, motLabelSetActiveAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "BGCOLOR", iupBaseNativeParentGetBgColorAttrib, motLabelSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_NO_SAVE);
  iupClassRegisterAttribute(ic, "BACKGROUND", NULL, motLabelSetBackgroundAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);

  /* Special */
  iupClassRegisterAttribute(ic, "FGCOLOR", NULL, iupdrvBaseSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "TITLE", NULL, motLabelSetTitleAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* IupLabel only */
  iupClassRegisterAttribute(ic, "ALIGNMENT", NULL, motLabelSetAlignmentAttrib, "ALEFT:ACENTER", NULL, IUPAF_NO_INHERIT);  /* force new default value */
  iupClassRegisterAttribute(ic, "IMAGE", NULL, motLabelSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "PADDING", iupLabelGetPaddingAttrib, motLabelSetPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED);

  /* IupLabel GTK and Motif only */
  iupClassRegisterAttribute(ic, "IMINACTIVE", NULL, motLabelSetImInactiveAttrib, NULL, NULL, IUPAF_IHANDLENAME|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);

  /* Not Supported */
  iupClassRegisterAttribute(ic, "WORDWRAP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ELLIPSIS", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKUP", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED|IUPAF_NO_INHERIT);
}
