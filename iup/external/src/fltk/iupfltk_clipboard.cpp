/** \file
 * \brief Clipboard for the FLTK Driver
 *
 * FLTK paste is async, so a hidden helper widget calls Fl::paste() and pumps
 * events until its FL_PASTE arrives.
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
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


static char* fltk_clipboard_text = nullptr;
static Fl_RGB_Image* fltk_clipboard_image = nullptr;
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
        auto* img = static_cast<Fl_RGB_Image*>(Fl::event_clipboard());
        if (img)
          fltk_clipboard_image = static_cast<Fl_RGB_Image*>(img->copy());
      }
      else
      {
        const char* text = Fl::event_text();
        int len = Fl::event_length();

        if (fltk_clipboard_text)
        {
          free(fltk_clipboard_text);
          fltk_clipboard_text = nullptr;
        }

        if (text && len > 0)
        {
          fltk_clipboard_text = static_cast<char*>(malloc(len + 1));
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

static FltkClipboardReceiver* fltk_clipboard_receiver = nullptr;

static FltkClipboardReceiver* fltkClipboardGetReceiver()
{
  if (!fltk_clipboard_receiver)
    fltk_clipboard_receiver = new FltkClipboardReceiver();
  return fltk_clipboard_receiver;
}

/* Fl::check does not wait, and the X reply needs time to arrive */
static void fltkClipboardWaitPaste()
{
  double remaining = 1.0;

  while (!fltk_clipboard_received && remaining > 0.0)
  {
    Fl::wait(0.05);
    remaining -= 0.05;
  }
}

static int fltkClipboardBuffer(Ihandle* ih)
{
  return iupStrEqualNoCase(iupAttribGetStr(ih, "SELECTION"), "PRIMARY")? 0: 1;
}

static void fltkClipboardRequestText(int buffer)
{
  FltkClipboardReceiver* receiver = fltkClipboardGetReceiver();

  if (fltk_clipboard_text)
  {
    free(fltk_clipboard_text);
    fltk_clipboard_text = nullptr;
  }

  fltk_clipboard_received = 0;
  Fl::paste(*receiver, buffer, Fl::clipboard_plain_text);
  fltkClipboardWaitPaste();
}

static void fltkClipboardRequestImage()
{
  FltkClipboardReceiver* receiver = fltkClipboardGetReceiver();

  if (fltk_clipboard_image)
  {
    delete fltk_clipboard_image;
    fltk_clipboard_image = nullptr;
  }

  fltk_clipboard_received = 0;
  Fl::paste(*receiver, 1, Fl::clipboard_image);
  fltkClipboardWaitPaste();
}

/****************************************************************************
 * TEXT Attribute
 ****************************************************************************/

static int fltkClipboardSetTextAttrib(Ihandle* ih, const char* value)
{
  int buffer = fltkClipboardBuffer(ih);

  if (!value)
  {
    Fl::copy("", 0, buffer);
    return 0;
  }

  Fl::copy(value, static_cast<int>(strlen(value)), buffer);
  return 0;
}

static char* fltkClipboardGetTextAttrib(Ihandle* ih)
{
  fltkClipboardRequestText(fltkClipboardBuffer(ih));

  if (fltk_clipboard_text)
    return iupStrReturnStr(fltk_clipboard_text);

  return nullptr;
}

static char* fltkClipboardGetTextAvailableAttrib(Ihandle* ih)
{
  if (fltkClipboardBuffer(ih) == 0)
  {
    /* Fl::clipboard_contains cannot see the selection, so it has to be fetched */
    fltkClipboardRequestText(0);
    return iupStrReturnBoolean(fltk_clipboard_text && fltk_clipboard_text[0]);
  }

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

  auto* surface = new Fl_Copy_Surface(w, h);
  Fl_Surface_Device::push_current(surface);
  image->draw(0, 0);
  Fl_Surface_Device::pop_current();
  delete surface;
}

static int fltkClipboardSetImageAttrib(Ihandle* ih, const char* value)
{
  if (!value)
    return 0;

  auto* image = static_cast<Fl_Image*>(iupImageGetImage(value, ih, 0, nullptr));
  fltkClipboardCopyImage(image);
  return 0;
}

static int fltkClipboardSetNativeImageAttrib(Ihandle* ih, const char* value)
{
  (void)ih;

  if (!value)
    return 0;

  fltkClipboardCopyImage(reinterpret_cast<Fl_Image*>(const_cast<char*>(value)));
  return 0;
}

static char* fltkClipboardGetNativeImageAttrib(Ihandle* ih)
{
  (void)ih;

  fltkClipboardRequestImage();

  if (fltk_clipboard_image)
    return reinterpret_cast<char*>(fltk_clipboard_image);

  return nullptr;
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
  Iclass* ic = iupClassNew(nullptr);

  ic->name = const_cast<char*>("clipboard");
  ic->format = nullptr;
  ic->nativetype = IUP_TYPEOTHER;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  ic->New = iupClipboardNewClass;

  iupClassRegisterAttribute(ic, "TEXT", fltkClipboardGetTextAttrib, fltkClipboardSetTextAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TEXTAVAILABLE", fltkClipboardGetTextAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NATIVEIMAGE", fltkClipboardGetNativeImageAttrib, fltkClipboardSetNativeImageAttrib, nullptr, nullptr, IUPAF_NO_STRING | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGE", nullptr, fltkClipboardSetImageAttrib, nullptr, nullptr, IUPAF_IHANDLENAME | IUPAF_WRITEONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEAVAILABLE", fltkClipboardGetImageAvailableAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "ADDFORMAT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMAT", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATAVAILABLE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATA", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASTRING", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FORMATDATASIZE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "SELECTION", nullptr, nullptr, "CLIPBOARD", nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  return ic;
}
