/** \file
 * \brief GTK4 Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <stdlib.h>
#include <stdio.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iup_drv.h"
#include "iupgtk4_drv.h"


typedef struct _Igtk2iupkey
{
  guint keyval;
  int iupcode;
} Igtk2iupkey;

static Igtk2iupkey keypad_remap[] = {
  { GDK_KEY_KP_0,         K_0  },
  { GDK_KEY_KP_1,         K_1  },
  { GDK_KEY_KP_2,         K_2  },
  { GDK_KEY_KP_3,         K_3  },
  { GDK_KEY_KP_4,         K_4  },
  { GDK_KEY_KP_5,         K_5  },
  { GDK_KEY_KP_6,         K_6  },
  { GDK_KEY_KP_7,         K_7  },
  { GDK_KEY_KP_8,         K_8  },
  { GDK_KEY_KP_9,         K_9  },
  { GDK_KEY_KP_Multiply,  K_asterisk },
  { GDK_KEY_KP_Add,       K_plus     },
  { GDK_KEY_KP_Subtract,  K_minus    },
  { GDK_KEY_KP_Decimal,   K_period   },
  { GDK_KEY_KP_Divide,    K_slash    },
  { GDK_KEY_KP_Separator, K_comma    },
  { GDK_KEY_KP_F1,        K_F1   },
  { GDK_KEY_KP_F2,        K_F2   },
  { GDK_KEY_KP_F3,        K_F3   },
  { GDK_KEY_KP_F4,        K_F4   },
  { GDK_KEY_KP_Space,     K_SP   },
  { GDK_KEY_KP_Tab,       K_TAB  },
  { GDK_KEY_KP_Equal,     K_equal},
  { GDK_KEY_KP_Enter,     K_CR   },
  { GDK_KEY_KP_Home,      K_HOME },
  { GDK_KEY_KP_Up,        K_UP   },
  { GDK_KEY_KP_Page_Up,   K_PGUP },
  { GDK_KEY_KP_Left,      K_LEFT },
  { GDK_KEY_KP_Begin,     K_MIDDLE},
  { GDK_KEY_KP_Right,     K_RIGHT},
  { GDK_KEY_KP_End,       K_END  },
  { GDK_KEY_KP_Down,      K_DOWN },
  { GDK_KEY_KP_Page_Down, K_PGDN },
  { GDK_KEY_KP_Insert,    K_INS  },
  { GDK_KEY_KP_Delete,    K_DEL  },
};

static Igtk2iupkey other_remap[] = {
  { GDK_KEY_BackSpace,       K_BS  },
  { GDK_KEY_Tab,             K_TAB },
  { GDK_KEY_Return,          K_CR  },
  { 0xFE03,                  K_RALT},
  { GDK_KEY_dead_tilde,      K_tilde },
  { GDK_KEY_dead_acute,      K_acute },
  { GDK_KEY_dead_grave,      K_grave },
  { GDK_KEY_dead_circumflex, K_circum},
  { GDK_KEY_dead_diaeresis,  K_diaeresis},
};

void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  *keyval = (unsigned int)iup_XkeyBase(code);

  if (*keyval == K_BS)  *keyval = GDK_KEY_BackSpace;
  if (*keyval == K_TAB) *keyval = GDK_KEY_Tab;
  if (*keyval == K_CR)  *keyval = GDK_KEY_Return;

  *state = 0;
  if (iup_isCtrlXkey(code))
    *state |= GDK_CONTROL_MASK;

  if (iup_isAltXkey(code))
    *state |= GDK_ALT_MASK;

  if (iup_isSysXkey(code))
    *state |= GDK_SUPER_MASK;

  if (iup_isShiftXkey(code))
    *state |= GDK_SHIFT_MASK;
}

static int gtk4KeyMap2Iup(guint keyval, GdkModifierType state)
{
  int code = (int)keyval;

  if (state & (GDK_CONTROL_MASK|GDK_ALT_MASK|GDK_SUPER_MASK))
  {
    /* If it has some of the other modifiers then use upper case version */
    if (keyval >= K_a && keyval <= K_z)
      code = iup_toupper(keyval);
    else if (keyval==K_ccedilla)
      code = K_Ccedilla;
  }

  if (state & GDK_SHIFT_MASK)
  {
    /* only add Shift modifiers for non-ASCii codes, except for K_SP and bellow, and except when other modifiers are used */
    if ((keyval < K_exclam || keyval > K_tilde) ||
        (state & (GDK_CONTROL_MASK|GDK_ALT_MASK|GDK_SUPER_MASK)))
      code = iup_XkeyShift(code);
  }

  if (state & GDK_CONTROL_MASK)
    code = iup_XkeyCtrl(code);

  if (state & GDK_ALT_MASK)       /* Alt: consolidated from MOD1/MOD5 */
    code = iup_XkeyAlt(code);

  if (state & GDK_SUPER_MASK)     /* Apple/Win: renamed from MOD4 */
    code = iup_XkeySys(code);

  return code;
}

int iupgtk4KeyDecode(guint keyval, GdkModifierType state)
{
  int i;

  if ((state & GDK_LOCK_MASK) && /* NumLock: GDK_MOD2_MASK → GDK_LOCK_MASK */
      (keyval >= GDK_KEY_KP_Home) &&
      (keyval <= GDK_KEY_KP_Delete))
  {
    /* remap to numeric keys */
    guint remap_numkey[] = {GDK_KEY_KP_7, GDK_KEY_KP_4, GDK_KEY_KP_8, GDK_KEY_KP_6, GDK_KEY_KP_2,
                            GDK_KEY_KP_9, GDK_KEY_KP_3, GDK_KEY_KP_1, GDK_KEY_KP_5, GDK_KEY_KP_0, GDK_KEY_KP_Decimal};
    keyval = remap_numkey[keyval-GDK_KEY_KP_Home];
  }

  if (keyval >= 0xFF80 && keyval < 0xFFC0)
  {
    /* remap keypad KeySyms */
    int count = sizeof(keypad_remap)/sizeof(keypad_remap[0]);
    for (i = 0; i < count; i++)
    {
      if (keypad_remap[i].keyval == keyval)
      {
        keyval = (guint)keypad_remap[i].iupcode;
        break;
      }
    }
  }

  {
    /* Other maps */
    int count = sizeof(other_remap)/sizeof(other_remap[0]);
    for (i = 0; i < count; i++)
    {
      if (other_remap[i].keyval == keyval)
      {
        keyval = (guint)other_remap[i].iupcode;
        break;
      }
    }
  }

  return gtk4KeyMap2Iup(keyval, state);
}

static int iupObjectIsNativeContainer(Ihandle* ih)
{
  if (ih->iclass->childtype != IUP_CHILDNONE &&
      ih->iclass->nativetype != IUP_TYPEVOID)
    return 1;
  else
    return 0;
}

IUP_DRV_API gboolean iupgtk4KeyPressEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  int result;
  int code = iupgtk4KeyDecode(keyval, state);
  GtkWidget *widget;

  (void)keycode;

  if (code == 0)
    return FALSE;

  /* Get the widget that this controller is attached to */
  widget = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

  /* Avoid duplicate calls if a child of a native container contains the focus.
     GTK will call the callback for the child and for the container. Ignore the one sent to the parent. */
  if (iupObjectIsNativeContainer(ih))
  {
    GtkWindow* win = (GtkWindow*)IupGetDialog(ih)->handle;
    GtkWidget *widget_focus = gtk_window_get_focus(win);
    if (widget_focus && widget_focus != widget)
      return FALSE;
  }

  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return FALSE;
  }
  if (result == IUP_IGNORE)
  {
    return TRUE;
  }

  if (iupObjectCheck(ih))
  {
    if (ih->iclass->nativetype == IUP_TYPECANVAS)
    {
      result = iupKeyCallKeyPressCb(ih, code, 1);
      if (result == IUP_CLOSE)
      {
        IupExitLoop();
        return FALSE;
      }
      if (result == IUP_IGNORE)
        return TRUE;
    }

    if (iupKeyProcessNavigation(ih, code, state & GDK_SHIFT_MASK))
      return TRUE;

    /* Compensate the show-help limitation.
     * It is not called on F1, only on Shift+F1 and Ctrl+F1. */
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

  return FALSE;
}

gboolean iupgtk4KeyReleaseEvent(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, Ihandle *ih)
{
  int result;
  int code = iupgtk4KeyDecode(keyval, state);

  (void)controller;
  (void)keycode;

  if (code == 0)
    return FALSE;

  result = iupKeyCallKeyPressCb(ih, code, 0);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return FALSE;
  }
  if (result == IUP_IGNORE)
    return TRUE;

  return FALSE;
}

void iupgtk4ButtonKeySetStatus(GdkModifierType state, unsigned int but, char* status, int doubleclick)
{
  if (state & GDK_SHIFT_MASK)
    iupKEY_SETSHIFT(status);

  if (state & GDK_CONTROL_MASK)
    iupKEY_SETCONTROL(status);

  /* Button masks still exist but deprecated, use button number parameter instead */
  if (but==1)
    iupKEY_SETBUTTON1(status);

  if (but==2)
    iupKEY_SETBUTTON2(status);

  if (but==3)
    iupKEY_SETBUTTON3(status);

  if (but==4)
    iupKEY_SETBUTTON4(status);

  if (but==5)
    iupKEY_SETBUTTON5(status);

  /* If button parameter is 0 (motion events), check button masks in state */
  if (but == 0)
  {
    if (state & GDK_BUTTON1_MASK)
      iupKEY_SETBUTTON1(status);

    if (state & GDK_BUTTON2_MASK)
      iupKEY_SETBUTTON2(status);

    if (state & GDK_BUTTON3_MASK)
      iupKEY_SETBUTTON3(status);

    if (state & GDK_BUTTON4_MASK)
      iupKEY_SETBUTTON4(status);

    if (state & GDK_BUTTON5_MASK)
      iupKEY_SETBUTTON5(status);
  }

  if (state & GDK_ALT_MASK) /* Alt: consolidated */
    iupKEY_SETALT(status);

  if (state & GDK_SUPER_MASK) /* Apple/Win: renamed */
    iupKEY_SETSYS(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);
}
