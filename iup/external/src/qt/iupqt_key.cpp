/** \file
 * \brief Qt Driver Keyboard Mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <Qt>
#include <QKeyEvent>
#include <QApplication>

#include <cstdlib>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"
#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_drv.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Key Mapping Structures
 ****************************************************************************/

typedef struct _Iqt2iupkey
{
  int qtkey;
  int iupcode;
} Iqt2iupkey;

static Iqt2iupkey keypad_remap[] = {
  { Qt::Key_0,        K_0  },
  { Qt::Key_1,        K_1  },
  { Qt::Key_2,        K_2  },
  { Qt::Key_3,        K_3  },
  { Qt::Key_4,        K_4  },
  { Qt::Key_5,        K_5  },
  { Qt::Key_6,        K_6  },
  { Qt::Key_7,        K_7  },
  { Qt::Key_8,        K_8  },
  { Qt::Key_9,        K_9  },
  { Qt::Key_Asterisk, K_asterisk },
  { Qt::Key_Plus,     K_plus     },
  { Qt::Key_Minus,    K_minus    },
  { Qt::Key_Period,   K_period   },
  { Qt::Key_Slash,    K_slash    },
  { Qt::Key_Comma,    K_comma    },
  { Qt::Key_F1,       K_F1   },
  { Qt::Key_F2,       K_F2   },
  { Qt::Key_F3,       K_F3   },
  { Qt::Key_F4,       K_F4   },
  { Qt::Key_Space,    K_SP   },
  { Qt::Key_Tab,      K_TAB  },
  { Qt::Key_Equal,    K_equal},
  { Qt::Key_Enter,    K_CR   },
  { Qt::Key_Return,   K_CR   },
  { Qt::Key_Home,     K_HOME },
  { Qt::Key_Up,       K_UP   },
  { Qt::Key_PageUp,   K_PGUP },
  { Qt::Key_Left,     K_LEFT },
  { Qt::Key_Clear,    K_MIDDLE},
  { Qt::Key_Right,    K_RIGHT},
  { Qt::Key_End,      K_END  },
  { Qt::Key_Down,     K_DOWN },
  { Qt::Key_PageDown, K_PGDN },
  { Qt::Key_Insert,   K_INS  },
  { Qt::Key_Delete,   K_DEL  },
};

static Iqt2iupkey other_remap[] = {
  { Qt::Key_Backspace,  K_BS  },
  { Qt::Key_Tab,        K_TAB },
  { Qt::Key_Return,     K_CR  },
  { Qt::Key_Enter,      K_CR  },
  { Qt::Key_Escape,     K_ESC },
  { Qt::Key_Pause,      K_PAUSE },
  { Qt::Key_Print,      K_Print },
  { Qt::Key_Menu,       K_Menu },
  { Qt::Key_Help,       K_HELP },

  /* Dead keys (compose keys) */
  { Qt::Key_Dead_Tilde,      K_tilde },
  { Qt::Key_Dead_Acute,      K_acute },
  { Qt::Key_Dead_Grave,      K_grave },
  { Qt::Key_Dead_Circumflex, K_circum },
  { Qt::Key_Dead_Diaeresis,  K_diaeresis },

  /* Function keys */
  { Qt::Key_F1,  K_F1  },
  { Qt::Key_F2,  K_F2  },
  { Qt::Key_F3,  K_F3  },
  { Qt::Key_F4,  K_F4  },
  { Qt::Key_F5,  K_F5  },
  { Qt::Key_F6,  K_F6  },
  { Qt::Key_F7,  K_F7  },
  { Qt::Key_F8,  K_F8  },
  { Qt::Key_F9,  K_F9  },
  { Qt::Key_F10, K_F10 },
  { Qt::Key_F11, K_F11 },
  { Qt::Key_F12, K_F12 },
  { Qt::Key_F13, K_F13 },
  { Qt::Key_F14, K_F14 },
  { Qt::Key_F15, K_F15 },
  { Qt::Key_F16, K_F16 },
  { Qt::Key_F17, K_F17 },
  { Qt::Key_F18, K_F18 },
  { Qt::Key_F19, K_F19 },
  { Qt::Key_F20, K_F20 },

  /* Navigation */
  { Qt::Key_Home,     K_HOME },
  { Qt::Key_Left,     K_LEFT },
  { Qt::Key_Up,       K_UP   },
  { Qt::Key_Right,    K_RIGHT},
  { Qt::Key_Down,     K_DOWN },
  { Qt::Key_PageUp,   K_PGUP },
  { Qt::Key_PageDown, K_PGDN },
  { Qt::Key_End,      K_END  },
  { Qt::Key_Insert,   K_INS  },
  { Qt::Key_Delete,   K_DEL  },
  { Qt::Key_Clear,    K_MIDDLE },

  /* Lock keys */
  { Qt::Key_NumLock,    K_NUM    },
  { Qt::Key_ScrollLock, K_SCROLL },
  { Qt::Key_CapsLock,   K_CAPS   },

  /* Modifier keys */
  { Qt::Key_Shift,   K_LSHIFT },
  { Qt::Key_Control, K_LCTRL  },
  { Qt::Key_Alt,     K_LALT   },
  { Qt::Key_Meta,    K_LALT   }, /* Some systems */
  { Qt::Key_AltGr,   K_RALT   },
};

/****************************************************************************
 * Key Encoding (IUP to Qt)
 ****************************************************************************/

extern "C" void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  *keyval = (unsigned int)iup_XkeyBase(code);

  if (*keyval == K_BS)  *keyval = Qt::Key_Backspace;
  if (*keyval == K_TAB) *keyval = Qt::Key_Tab;
  if (*keyval == K_CR)  *keyval = Qt::Key_Return;
  if (*keyval == K_ESC) *keyval = Qt::Key_Escape;

  *state = 0;

  if (iup_isCtrlXkey(code))
    *state |= Qt::ControlModifier;

  if (iup_isAltXkey(code))
    *state |= Qt::AltModifier;

  if (iup_isSysXkey(code))
    *state |= Qt::MetaModifier;

  if (iup_isShiftXkey(code))
    *state |= Qt::ShiftModifier;
}


/****************************************************************************
 * Key Mapping (Qt to IUP)
 ****************************************************************************/

static int qtKeyMap2Iup(int keyval, Qt::KeyboardModifiers modifiers)
{
  int code = keyval;

  /* If it has modifiers (except Shift alone), convert to uppercase */
  if (modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
  {
    if (keyval >= K_a && keyval <= K_z)
      code = iup_toupper(keyval);
    else if (keyval == K_ccedilla)
      code = K_Ccedilla;
  }

  if (modifiers & Qt::ShiftModifier)
  {
    if ((keyval < K_exclam || keyval > K_tilde) ||
        (modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)))
      code = iup_XkeyShift(code);
  }

  if (modifiers & Qt::ControlModifier)
    code = iup_XkeyCtrl(code);

  if (modifiers & Qt::AltModifier)
    code = iup_XkeyAlt(code);

  /* Add Meta/Super/Cmd modifier */
  if (modifiers & Qt::MetaModifier)
    code = iup_XkeySys(code);

  return code;
}

/****************************************************************************
 * Key Decoding (Qt event to IUP code)
 ****************************************************************************/

static int qtKeyDecode(QKeyEvent *evt)
{
  int key = evt->key();
  Qt::KeyboardModifiers modifiers = evt->modifiers();

  if ((modifiers & Qt::KeypadModifier) && !(modifiers & Qt::ShiftModifier))
  {
    /* Remap keypad navigation to numeric keys when NumLock is on */
    switch (key)
    {
      case Qt::Key_Home:     key = Qt::Key_7; break;
      case Qt::Key_Left:     key = Qt::Key_4; break;
      case Qt::Key_Up:       key = Qt::Key_8; break;
      case Qt::Key_Right:    key = Qt::Key_6; break;
      case Qt::Key_Down:     key = Qt::Key_2; break;
      case Qt::Key_PageUp:   key = Qt::Key_9; break;
      case Qt::Key_PageDown: key = Qt::Key_3; break;
      case Qt::Key_End:      key = Qt::Key_1; break;
      case Qt::Key_Clear:    key = Qt::Key_5; break;
      case Qt::Key_Insert:   key = Qt::Key_0; break;
      case Qt::Key_Delete:   key = Qt::Key_Period; break;
    }
  }

  if (modifiers & Qt::KeypadModifier)
  {
    int count = sizeof(keypad_remap) / sizeof(keypad_remap[0]);
    for (int i = 0; i < count; i++)
    {
      if (keypad_remap[i].qtkey == key)
      {
        key = keypad_remap[i].iupcode;
        break;
      }
    }
  }
  else
  {
    int count = sizeof(other_remap) / sizeof(other_remap[0]);
    for (int i = 0; i < count; i++)
    {
      if (other_remap[i].qtkey == key)
      {
        key = other_remap[i].iupcode;
        break;
      }
    }
  }

  /* If it's a printable ASCII character from text(), use that instead */
  QString text = evt->text();
  if (text.length() == 1 && !(modifiers & Qt::ControlModifier))
  {
    QChar ch = text[0];
    if (ch.isPrint() && ch.unicode() < 128)
      key = ch.unicode();
  }

  /* Map to IUP code with modifiers */
  return qtKeyMap2Iup(key, modifiers);
}

/****************************************************************************
 * Key Event Handlers (exported to C)
 ****************************************************************************/

static int iupObjectIsNativeContainer(Ihandle* ih)
{
  if (ih->iclass->childtype != IUP_CHILDNONE &&
      ih->iclass->nativetype != IUP_TYPEVOID)
    return 1;
  else
    return 0;
}

extern "C" int iupqtKeyPressEvent(QWidget *widget, QKeyEvent *evt, Ihandle *ih)
{
  int result;
  int code = qtKeyDecode(evt);
  if (code == 0)
    return 0;

  /* Avoid duplicate calls if a child of a native container has focus */
  if (iupObjectIsNativeContainer(ih))
  {
    /* For Qt, get the focused widget from the window */
    QWidget* focused = QApplication::focusWidget();
    if (focused && focused != widget)
      return 0;
  }

  /* Call K_ANY and K_<key> callbacks */
  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return 0;
  }
  if (result == IUP_IGNORE)
    return 1;

  /* In the previous callback the dialog could be destroyed */
  if (iupObjectCheck(ih))
  {
    /* This is called only for canvas */
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

    /* Handle Alt+key for mnemonics (similar to Windows implementation) */
    if (evt->modifiers() & Qt::AltModifier)
    {
      int base_code = iup_XkeyBase(code);
      if (base_code < 128 && iupKeyProcessMnemonic(ih, base_code))
        return 1;
    }

    /* Process navigation keys (Tab, arrows, etc.) */
    if (iupKeyProcessNavigation(ih, code, evt->modifiers() & Qt::ShiftModifier))
      return 1;

    /* Handle F1 for help - Qt doesn't have special help key handling like GTK */
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

extern "C" int iupqtKeyReleaseEvent(QWidget *widget, QKeyEvent *evt, Ihandle *ih)
{
  /* This is called only for canvas */
  int result;
  int code = qtKeyDecode(evt);
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
 * Button/Key Status String
 ****************************************************************************/

extern "C" void iupqtButtonKeySetStatus(Qt::KeyboardModifiers modifiers, Qt::MouseButtons buttons,
                                        int button, char* status, int doubleclick)
{
  if (modifiers & Qt::ShiftModifier)
    iupKEY_SETSHIFT(status);

  if (modifiers & Qt::ControlModifier)
    iupKEY_SETCONTROL(status);

  if ((buttons & Qt::LeftButton) || button == 1)
    iupKEY_SETBUTTON1(status);

  if ((buttons & Qt::MiddleButton) || button == 2)
    iupKEY_SETBUTTON2(status);

  if ((buttons & Qt::RightButton) || button == 3)
    iupKEY_SETBUTTON3(status);

  if ((buttons & Qt::XButton1) || button == 4)
    iupKEY_SETBUTTON4(status);

  if ((buttons & Qt::XButton2) || button == 5)
    iupKEY_SETBUTTON5(status);

  if (modifiers & Qt::AltModifier)
    iupKEY_SETALT(status);

  if (modifiers & Qt::MetaModifier)
    iupKEY_SETSYS(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);
}
