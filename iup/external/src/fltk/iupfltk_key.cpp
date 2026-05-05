/** \file
 * \brief FLTK Driver Keyboard Mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Enumerations.H>

extern "C" {
#include "iup.h"
#include "iupkey.h"
#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_drv.h"
}

#include "iupfltk_drv.h"


/****************************************************************************
 * Key Mapping Structures
 ****************************************************************************/

typedef struct _Ifltk2iupkey
{
  int fltkkey;
  int iupcode;
} Ifltk2iupkey;

static Ifltk2iupkey special_remap[] = {
  { FL_BackSpace,   K_BS     },
  { FL_Tab,         K_TAB    },
  { FL_Enter,       K_CR     },
  { FL_Escape,      K_ESC    },
  { FL_Pause,       K_PAUSE  },
  { FL_Print,       K_Print  },
  { FL_Menu,        K_Menu   },
  { FL_Help,        K_HELP   },

  { FL_F + 1,       K_F1     },
  { FL_F + 2,       K_F2     },
  { FL_F + 3,       K_F3     },
  { FL_F + 4,       K_F4     },
  { FL_F + 5,       K_F5     },
  { FL_F + 6,       K_F6     },
  { FL_F + 7,       K_F7     },
  { FL_F + 8,       K_F8     },
  { FL_F + 9,       K_F9     },
  { FL_F + 10,      K_F10    },
  { FL_F + 11,      K_F11    },
  { FL_F + 12,      K_F12    },
  { FL_F + 13,      K_F13    },
  { FL_F + 14,      K_F14    },
  { FL_F + 15,      K_F15    },
  { FL_F + 16,      K_F16    },
  { FL_F + 17,      K_F17    },
  { FL_F + 18,      K_F18    },
  { FL_F + 19,      K_F19    },
  { FL_F + 20,      K_F20    },

  { FL_Home,        K_HOME   },
  { FL_Left,        K_LEFT   },
  { FL_Up,          K_UP     },
  { FL_Right,       K_RIGHT  },
  { FL_Down,        K_DOWN   },
  { FL_Page_Up,     K_PGUP   },
  { FL_Page_Down,   K_PGDN   },
  { FL_End,         K_END    },
  { FL_Insert,      K_INS    },
  { FL_Delete,      K_DEL    },

  { FL_Num_Lock,    K_NUM    },
  { FL_Scroll_Lock, K_SCROLL },
  { FL_Caps_Lock,   K_CAPS   },

  { FL_Shift_L,     K_LSHIFT },
  { FL_Shift_R,     K_RSHIFT },
  { FL_Control_L,   K_LCTRL  },
  { FL_Control_R,   K_RCTRL  },
  { FL_Alt_L,       K_LALT   },
  { FL_Alt_R,       K_RALT   },
  { FL_Meta_L,      K_LALT   },
  { FL_Meta_R,      K_RALT   },

  { FL_KP + '0',    K_0      },
  { FL_KP + '1',    K_1      },
  { FL_KP + '2',    K_2      },
  { FL_KP + '3',    K_3      },
  { FL_KP + '4',    K_4      },
  { FL_KP + '5',    K_5      },
  { FL_KP + '6',    K_6      },
  { FL_KP + '7',    K_7      },
  { FL_KP + '8',    K_8      },
  { FL_KP + '9',    K_9      },
  { FL_KP + '*',    K_asterisk },
  { FL_KP + '+',    K_plus   },
  { FL_KP + '-',    K_minus  },
  { FL_KP + '.',    K_period },
  { FL_KP + '/',    K_slash  },
  { FL_KP_Enter,    K_CR     },
};

/****************************************************************************
 * Key Mapping (FLTK to IUP)
 ****************************************************************************/

static int fltkKeyMap2Iup(int key, int state)
{
  int code = key;

  if (state & (FL_CTRL | FL_ALT | FL_META))
  {
    if (code >= K_a && code <= K_z)
      code = iup_toupper(code);
    else if (code == K_ccedilla)
      code = K_Ccedilla;
  }

  if (state & FL_SHIFT)
  {
    if ((code < K_exclam || code > K_tilde) ||
        (state & (FL_CTRL | FL_ALT | FL_META)))
      code = iup_XkeyShift(code);
  }

  if (state & FL_CTRL)
    code = iup_XkeyCtrl(code);

  if (state & FL_ALT)
    code = iup_XkeyAlt(code);

  if (state & FL_META)
    code = iup_XkeySys(code);

  return code;
}

/****************************************************************************
 * Key Decoding (FLTK event to IUP code)
 ****************************************************************************/

IUP_DRV_API int iupfltkKeyDecode(void)
{
  int key = Fl::event_key();
  int state = Fl::event_state();

  int count = sizeof(special_remap) / sizeof(special_remap[0]);
  for (int i = 0; i < count; i++)
  {
    if (special_remap[i].fltkkey == key)
    {
      key = special_remap[i].iupcode;
      return fltkKeyMap2Iup(key, state);
    }
  }

  /* Printable ASCII characters */
  const char* text = Fl::event_text();
  int text_len = Fl::event_length();
  if (text_len == 1 && !(state & FL_CTRL))
  {
    unsigned char ch = (unsigned char)text[0];
    if (ch >= 32 && ch < 127)
      key = ch;
  }
  else if (key >= 32 && key < 127)
  {
    /* Use the key value directly for printable range */
  }

  return fltkKeyMap2Iup(key, state);
}

/****************************************************************************
 * Key Event Handlers
 ****************************************************************************/

static int iupObjectIsNativeContainer(Ihandle* ih)
{
  if (ih->iclass->childtype != IUP_CHILDNONE &&
      ih->iclass->nativetype != IUP_TYPEVOID)
    return 1;
  else
    return 0;
}

IUP_DRV_API int iupfltkKeyPressEvent(Fl_Widget *widget, Ihandle *ih)
{
  int result;
  int code = iupfltkKeyDecode();
  if (code == 0)
    return 0;

  if (iupObjectIsNativeContainer(ih))
  {
    Fl_Widget* focused = Fl::focus();
    if (focused && focused != widget)
      return 0;
  }

  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return 0;
  }
  if (result == IUP_IGNORE)
    return 1;

  if (iupObjectCheck(ih))
  {
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
    {
      result = iupKeyCallKeyPressCb(ih, code, 1);
      if (result == IUP_CLOSE)
      {
        IupExitLoop();
        return 0;
      }
      if (result == IUP_IGNORE)
        return 1;
    }

    if (Fl::event_state() & FL_ALT)
    {
      int base_code = iup_XkeyBase(code);
      if (base_code < 128 && iupKeyProcessMnemonic(ih, base_code))
        return 1;
    }

    if (iupKeyProcessNavigation(ih, code, (Fl::event_state() & FL_SHIFT) ? 1 : 0))
      return 1;

    if (code == K_F1)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb)
      {
        if (cb(ih) == IUP_CLOSE)
          IupExitLoop();
      }
    }
  }

  return 0;
}

IUP_DRV_API int iupfltkKeyReleaseEvent(Fl_Widget *widget, Ihandle *ih)
{
  int result;
  int code = iupfltkKeyDecode();
  if (code == 0)
    return 0;

  result = iupKeyCallKeyPressCb(ih, code, 0);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return 0;
  }
  if (result == IUP_IGNORE)
    return 1;

  (void)widget;
  return 0;
}

/****************************************************************************
 * Key Encoding (IUP to FLTK)
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  *keyval = (unsigned int)iup_XkeyBase(code);

  if (*keyval == K_BS)  *keyval = FL_BackSpace;
  if (*keyval == K_TAB) *keyval = FL_Tab;
  if (*keyval == K_CR)  *keyval = FL_Enter;
  if (*keyval == K_ESC) *keyval = FL_Escape;

  *state = 0;

  if (iup_isCtrlXkey(code))
    *state |= FL_CTRL;

  if (iup_isAltXkey(code))
    *state |= FL_ALT;

  if (iup_isSysXkey(code))
    *state |= FL_META;

  if (iup_isShiftXkey(code))
    *state |= FL_SHIFT;
}

/****************************************************************************
 * Input Simulation
 ****************************************************************************/

#include <FL/Fl_Window.H>

/* App-local synthesis via Fl::handle: events fire IUP callbacks but the OS cursor is not moved. */

static Fl_Window* fltkFindWindowAt(int sx, int sy)
{
  for (Fl_Window* w = Fl::first_window(); w; w = Fl::next_window(w))
  {
    if (!w->shown() || !w->visible()) continue;
    if (sx >= w->x() && sx < w->x() + w->w() &&
        sy >= w->y() && sy < w->y() + w->h())
      return w;
  }
  return Fl::first_window();
}

extern "C" IUP_SDK_API void iupdrvSendKey(int key, int press)
{
  unsigned int keyval = 0, state = 0;
  iupdrvKeyEncode(key, &keyval, &state);
  if (!keyval) return;

  Fl_Window* win = Fl::focus() ? Fl::focus()->window() : Fl::first_window();
  if (!win) return;

  Fl::e_keysym = (int)keyval;
  Fl::e_state = (int)state;
  Fl::e_text = (char*)"";
  Fl::e_length = 0;

  if (press & 0x01) Fl::handle(FL_KEYDOWN, win);
  if (press & 0x02) Fl::handle(FL_KEYUP, win);
}

extern "C" IUP_SDK_API void iupdrvSendMouse(int x, int y, int bt, int status)
{
  Fl_Window* win = fltkFindWindowAt(x, y);
  if (!win) return;

  Fl::e_x_root = x;
  Fl::e_y_root = y;
  Fl::e_x = x - win->x();
  Fl::e_y = y - win->y();

  if (bt == 'W')
  {
    Fl::e_dy = -status;
    Fl::e_dx = 0;
    Fl::handle(FL_MOUSEWHEEL, win);
    return;
  }

  int fl_btn = 0;
  switch (bt)
  {
    case IUP_BUTTON1: fl_btn = FL_LEFT_MOUSE;   break;
    case IUP_BUTTON2: fl_btn = FL_MIDDLE_MOUSE; break;
    case IUP_BUTTON3: fl_btn = FL_RIGHT_MOUSE;  break;
    default: break;
  }
  Fl::e_keysym = FL_Button + fl_btn;

  if (status == -1)
  {
    Fl::handle(FL_MOVE, win);
  }
  else if (status == 1 || status == 2)
  {
    Fl::e_clicks = (status == 2) ? 1 : 0;
    Fl::handle(FL_PUSH, win);
  }
  else
  {
    Fl::handle(FL_RELEASE, win);
  }
}
