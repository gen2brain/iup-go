/** \file
 * \brief Clipboard for the FLTK Driver
 *
 * FLTK clipboard is async (paste triggers FL_PASTE event on a widget).
 * We use a hidden helper widget to receive paste events synchronously
 * by calling Fl::paste() then pumping events until FL_PASTE arrives.
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Copy_Surface.H>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_image.h"
}

#include "iupfltk_drv.h"


static char* fltk_clipboard_text = NULL;
static Fl_RGB_Image* fltk_clipboard_image = NULL;
static int fltk_clipboard_received = 0;

class FltkClipboardReceiver : public Fl_Widget
{
public:
  FltkClipboardReceiver() : Fl_Widget(0, 0, 0, 0) {}

  int handle(int event) override
  {
    if (event == FL_PASTE)
    {
      fltk_clipboard_received = 1;

      if (Fl::event_clipboard_type() == Fl::clipboard_image)
      {
        Fl_RGB_Image* img = (Fl_RGB_Image*)Fl::event_clipboard();
        if (img)
          fltk_clipboard_image = (Fl_RGB_Image*)img->copy();
      }
      else
      {
        const char* text = Fl::event_text();
        int len = Fl::event_length();

        if (fltk_clipboard_text)
        {
          free(fltk_clipboard_text);
          fltk_clipboard_text = NULL;
        }

        if (text && len > 0)
        {
          fltk_clipboard_text = (char*)malloc(len + 1);
          memcpy(fltk_clipboard_text, text, len);
          fltk_clipboard_text[len] = 0;
        }
      }

      return 1;
    }
    return 0;
  }

  void draw() override {}
};

static FltkClipboardReceiver* fltk_clipboard_receiver = NULL;

static FltkClipboardReceiver* fltkClipboardGetReceiver(void)
{
  if (!fltk_clipboard_receiver)
    fltk_clipboard_receiver = new FltkClipboardReceiver();
  return fltk_clipboard_receiver;
}

static void fltkClipboardRequestText(void)
{
  FltkClipboardReceiver* receiver = fltkClipboardGetReceiver();

  if (fltk_clipboard_text)
  {
    free(fltk_clipboard_text);
    fltk_clipboard_text = NULL;
  }

  fltk_clipboard_received = 0;
  Fl::paste(*receiver, 1, Fl::clipboard_plain_text);

  int timeout = 50;
  while (!fltk_clipboard_received && timeout > 0)
  {
    Fl::check();
    timeout--;
  }
}

static void fltkClipboardRequestImage(void)
{
  FltkClipboardReceiver* receiver = fltkClipboardGetReceiver();

  if (fltk_clipboard_image)
  {
    delete fltk_clipboard_image;
    fltk_clipboard_image = NULL;
  }

  fltk_clipboard_received = 0;
  Fl::paste(*receiver, 1, Fl::clipboard_image);

  int timeout = 50;
  while (!fltk_clipboard_received && timeout > 0)
  {
    Fl::check();
    timeout--;
  }
}

/****************************************************************************
 * TEXT Attribute
 ****************************************************************************/

static int fltkClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  (void)ih;

  if (!value)
  {
    Fl::copy("", 0, 1);
    return 0;
  }

  Fl::copy(value, (int)strlen(value), 1);
  return 0;
}

static char* fltkClipboardGetTextAttrib(Ihandle* ih)
{
  (void)ih;

  fltkClipboardRequestText();

  if (fltk_clipboard_text)
    return iupStrReturnStr(fltk_clipboard_text);

  return NULL;
}

static char* fltkClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(Fl::clipboard_contains(Fl::clipboard_plain_text));
}

/****************************************************************************
 * IMAGE Attributes
 ****************************************************************************/

static void fltkClipboardCopyImage(Fl_Image* image)
{
  if (!image) return;

  int w = image->data_w();
  int h = image->data_h();

  Fl_Copy_Surface* surface = new Fl_Copy_Surface(w, h);
  Fl_Surface_Device::push_current(surface);
  image->draw(0, 0);
  Fl_Surface_Device::pop_current();
  delete surface;
}

static int fltkClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  Fl_Image* image = (Fl_Image*)iupImageGetImage(value, ih, 0, NULL);
  fltkClipboardCopyImage(image);
  return 0;
}

static int fltkClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
  (void)ih;

  if (!value)
    return 0;

  fltkClipboardCopyImage((Fl_Image*)value);
  return 0;
}

static char* fltkClipboardGetNativeImageAttrib(Ihandle* ih)
{
  (void)ih;

  fltkClipboardRequestImage();

  if (fltk_clipboard_image)
    return (char*)fltk_clipboard_image;

  return NULL;
}

static char* fltkClipboardGetImageAvailableAttrib(Ihandle* ih)
{
  (void)ih;
  return iupStrReturnBoolean(Fl::clipboard_contains(Fl::clipboard_image));
}

/****************************************************************************
 * Class
 ****************************************************************************/

extern "C" IUP_API Ihandle* IupClipboard(void)
{
  return IupCreate("clipboard");
}

extern "C" Iclass* iupClipboardNewClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = (char*)"clipboard";
  ic->format = NULL;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", fltkClipboardGetTextAttrib, fltkClipboardSetTextAttrib, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", fltkClipboardGetTextAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", fltkClipboardGetNativeImageAttrib, fltkClipboardSetNativeImageAttrib, NULL, NULL, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", NULL, fltkClipboardSetImageAttrib, NULL, NULL, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", fltkClipboardGetImageAvailableAttrib, NULL, NULL, NULL, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  return ic;
}
