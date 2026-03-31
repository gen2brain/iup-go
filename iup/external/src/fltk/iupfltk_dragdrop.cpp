/** \file
 * \brief Drag and Drop Support - FLTK Implementation
 *
 * FLTK DnD model:
 *   Drag: Fl::copy(data, len, 0) + Fl::dnd() (blocks until drop)
 *   Drop: FL_DND_ENTER/DRAG/LEAVE/RELEASE events + FL_PASTE for data
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_key.h"
}

#include "iupfltk_drv.h"


static Ihandle* fltk_drag_source_ih = NULL;
static int fltk_drag_start_x = 0;
static int fltk_drag_start_y = 0;
static int fltk_drag_tracking = 0;

static int fltk_drop_x = 0;
static int fltk_drop_y = 0;

IUP_DRV_API int iupfltkDragDropHandleEvent(Fl_Widget* widget, Ihandle* ih, int event)
{
  if (!ih || !widget)
    return 0;

  switch (event)
  {
    case FL_PUSH:
    {
      if (iupAttribGetBoolean(ih, "DRAGSOURCE") && Fl::event_button() == FL_LEFT_MOUSE)
      {
        fltk_drag_source_ih = ih;
        fltk_drag_start_x = Fl::event_x();
        fltk_drag_start_y = Fl::event_y();
        fltk_drag_tracking = 1;
      }
      return 0;
    }

    case FL_DRAG:
    {
      if (fltk_drag_tracking && fltk_drag_source_ih == ih)
      {
        int dx = Fl::event_x() - fltk_drag_start_x;
        int dy = Fl::event_y() - fltk_drag_start_y;

        if (dx * dx + dy * dy >= 25)
        {
          fltk_drag_tracking = 0;

          int wx = Fl::event_x() - widget->x();
          int wy = Fl::event_y() - widget->y();

          IFnii dragbegin_cb = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
          if (dragbegin_cb && dragbegin_cb(ih, wx, wy) == IUP_IGNORE)
          {
            fltk_drag_source_ih = NULL;
            return 1;
          }

          char* type = iupAttribGet(ih, "DRAGTYPES");
          if (!type) type = (char*)"text/plain";

          IFns datasize_cb = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
          int data_size = 0;
          if (datasize_cb)
            data_size = datasize_cb(ih, type);

          if (data_size > 0)
          {
            char* data = (char*)calloc(1, data_size + 1);

            IFnsVi dragdata_cb = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");
            if (dragdata_cb)
              dragdata_cb(ih, type, data, data_size);

            Fl::copy(data, data_size, 0);
            free(data);

            Fl::dnd();

            IFni dragend_cb = (IFni)IupGetCallback(ih, "DRAGEND_CB");
            if (dragend_cb)
            {
              int action = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE") ? 1 : 0;
              dragend_cb(ih, action);
            }
          }

          fltk_drag_source_ih = NULL;
          return 1;
        }
      }
      return 0;
    }

    case FL_RELEASE:
    {
      if (fltk_drag_tracking && fltk_drag_source_ih == ih)
      {
        fltk_drag_tracking = 0;
        fltk_drag_source_ih = NULL;
      }
      return 0;
    }

    case FL_DND_ENTER:
    case FL_DND_DRAG:
    {
      if (iupAttribGetBoolean(ih, "DROPTARGET"))
      {
        fltk_drop_x = Fl::event_x() - widget->x();
        fltk_drop_y = Fl::event_y() - widget->y();

        if (event == FL_DND_DRAG)
        {
          IFniis dropmotion_cb = (IFniis)IupGetCallback(ih, "DROPMOTION_CB");
          if (dropmotion_cb)
          {
            char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
            iupfltkButtonKeySetStatus(Fl::event_state(), 0, status, 0);
            dropmotion_cb(ih, fltk_drop_x, fltk_drop_y, status);
          }

          int pos = IupConvertXYToPos(ih, fltk_drop_x, fltk_drop_y);
          if (pos >= 0)
          {
            iupAttribSetInt(ih, "_IUPFLTK_DND_TARGET_LINE", pos);
            widget->redraw();
          }
        }

        return 1;
      }
      return 0;
    }

    case FL_DND_LEAVE:
      if (iupAttribGetBoolean(ih, "DROPTARGET"))
      {
        iupAttribSet(ih, "_IUPFLTK_DND_TARGET_LINE", NULL);
        widget->redraw();
        return 1;
      }
      return 0;

    case FL_DND_RELEASE:
      if (iupAttribGetBoolean(ih, "DROPTARGET"))
      {
        fltk_drop_x = Fl::event_x() - widget->x();
        fltk_drop_y = Fl::event_y() - widget->y();
        iupAttribSet(ih, "_IUPFLTK_DND_TARGET_LINE", NULL);
        widget->redraw();
        return 1;
      }
      return 0;

    case FL_PASTE:
    {
      iupAttribSet(ih, "_IUPFLTK_DND_TARGET_LINE", NULL);
      widget->redraw();

      if (iupAttribGetBoolean(ih, "DROPTARGET"))
      {
        const char* text = Fl::event_text();
        int len = Fl::event_length();

        if (text && len > 0)
        {
          if (strstr(text, "file://"))
          {
            iupfltkHandleDropFiles(ih);
            return 1;
          }

          char* type = iupAttribGet(ih, "DROPTYPES");
          if (!type) type = (char*)"text/plain";

          IFnsViii dropdata_cb = (IFnsViii)IupGetCallback(ih, "DROPDATA_CB");
          if (dropdata_cb)
          {
            dropdata_cb(ih, type, (void*)text, len, fltk_drop_x, fltk_drop_y);
            return 1;
          }
        }
      }

      if (IupGetCallback(IupGetDialog(ih), "DROPFILES_CB"))
      {
        if (Fl::event_text() && strstr(Fl::event_text(), "file://"))
        {
          iupfltkHandleDropFiles(ih);
          return 1;
        }
      }

      return 0;
    }
  }

  return 0;
}


extern "C" IUP_SDK_API void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
}
