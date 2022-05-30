/** \file
 * \brief GTK Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <gdk/gdkkeysyms-compat.h>
#endif

#include <stdlib.h>      
#include <stdio.h>      

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iup_drv.h"
#include "iupgtk_drv.h"


typedef struct _Igtk2iupkey
{
  guint keyval;
  int iupcode;
} Igtk2iupkey;

static Igtk2iupkey keypad_remap[] = {
  { GDK_KP_0,   K_0  },
  { GDK_KP_1,   K_1  },
  { GDK_KP_2,   K_2  },
  { GDK_KP_3,   K_3  },
  { GDK_KP_4,   K_4  },
  { GDK_KP_5,   K_5  },
  { GDK_KP_6,   K_6  },
  { GDK_KP_7,   K_7  },
  { GDK_KP_8,   K_8  },
  { GDK_KP_9,   K_9  },
  { GDK_KP_Multiply,  K_asterisk },
  { GDK_KP_Add,       K_plus     },
  { GDK_KP_Subtract,  K_minus    },
  { GDK_KP_Decimal,   K_period   },
  { GDK_KP_Divide,    K_slash    },
  { GDK_KP_Separator, K_comma    },
  { GDK_KP_F1,        K_F1   },
  { GDK_KP_F2,        K_F2   },
  { GDK_KP_F3,        K_F3   },
  { GDK_KP_F4,        K_F4   },
  { GDK_KP_Space,     K_SP   },
  { GDK_KP_Tab,       K_TAB  },
  { GDK_KP_Equal,     K_equal},
  { GDK_KP_Enter,     K_CR   },
  { GDK_KP_Home,      K_HOME },
  { GDK_KP_Up,        K_UP   },
  { GDK_KP_Page_Up,   K_PGUP },
  { GDK_KP_Left,      K_LEFT },
  { GDK_KP_Begin,     K_MIDDLE},
  { GDK_KP_Right,     K_RIGHT},
  { GDK_KP_End,       K_END  },
  { GDK_KP_Down,      K_DOWN },
  { GDK_KP_Page_Down, K_PGDN },
  { GDK_KP_Insert,    K_INS  },
  { GDK_KP_Delete,    K_DEL  },
};

static Igtk2iupkey other_remap[] = {
  { GDK_BackSpace, K_BS  },
  { GDK_Tab,       K_TAB },
  { GDK_Return,    K_CR  },
  { 0xFE03,        K_RALT},
  { GDK_dead_tilde,      K_tilde },
  { GDK_dead_acute,      K_acute },
  { GDK_dead_grave,      K_grave },
  { GDK_dead_circumflex, K_circum},
  { GDK_dead_diaeresis,  K_diaeresis},
};

void iupdrvKeyEncode(int code, unsigned int *keyval, unsigned int *state)
{
  *keyval = (unsigned int)iup_XkeyBase(code);

  /* Only need to un-remap these */
  if (*keyval == K_BS)  *keyval = GDK_BackSpace;
  if (*keyval == K_TAB) *keyval = GDK_Tab;
  if (*keyval == K_CR)  *keyval = GDK_Return;

  *state = 0;
  if (iup_isCtrlXkey(code))
    *state |= GDK_CONTROL_MASK;

  if (iup_isAltXkey(code))
    *state |= GDK_MOD1_MASK;

  if (iup_isSysXkey(code))
    *state |= GDK_MOD4_MASK;

  if (iup_isShiftXkey(code))
    *state |= GDK_SHIFT_MASK;
}

static int gtkKeyMap2Iup(guint keyval, int state)
{
  int code = (int)keyval;

  if (state & (GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD5_MASK|GDK_MOD4_MASK))
  {
    /* If it has some of the other modifiers then use upper case version */
    if (keyval >= K_a && keyval <= K_z)
      code = iup_toupper(keyval);
    else if (keyval==K_ccedilla)
      code = K_Ccedilla;
  }

  if (state & GDK_SHIFT_MASK)  /* Shift */
  {
    /* only add Shift modifiers for non-ASCii codes, except for K_SP and bellow, 
       and except when other modifiers are used */
    if ((keyval < K_exclam || keyval > K_tilde) ||
        (state & (GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD5_MASK|GDK_MOD4_MASK)))
      code = iup_XkeyShift(code);  
  }

  if (state & GDK_CONTROL_MASK)   /* Ctrl */
    code = iup_XkeyCtrl(code);

  if (state & GDK_MOD1_MASK ||
      state & GDK_MOD5_MASK)      /* Alt */
    code = iup_XkeyAlt(code);

  if (state & GDK_MOD4_MASK)      /* Apple/Win */
    code = iup_XkeySys(code);

  return code;
}

int iupgtkKeyDecode(GdkEventKey *evt)
{
  int i;
  guint keyval = evt->keyval;

  if ((evt->state & GDK_MOD2_MASK) && /* NumLock */
      (keyval >= GDK_KP_Home) &&
      (keyval <= GDK_KP_Delete))
  {
    /* remap to numeric keys */
                        /*  GDK_KP_Home,GDK_KP_Left,GDK_KP_Up,GDK_KP_Right,GDK_KP_Down,GDK_KP_Page_Up,GDK_KP_Page_Down,GDK_KP_End,GDK_KP_Begin,GDK_KP_Insert,GDK_KP_Delete */
    guint remap_numkey[] = {GDK_KP_7,   GDK_KP_4,   GDK_KP_8, GDK_KP_6,    GDK_KP_2,   GDK_KP_9,      GDK_KP_3,        GDK_KP_1,  GDK_KP_5,    GDK_KP_0,     GDK_KP_Decimal};
    keyval = remap_numkey[keyval-GDK_KP_Home];
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

  return gtkKeyMap2Iup(keyval, evt->state);
}

static int iupObjectIsNativeContainer(Ihandle* ih)
{
  if (ih->iclass->childtype != IUP_CHILDNONE && 
      ih->iclass->nativetype != IUP_TYPEVOID)
    return 1;
  else
    return 0;
}

IUP_DRV_API gboolean iupgtkKeyPressEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  int result;
  int code = iupgtkKeyDecode(evt);
  if (code == 0) 
    return FALSE;

  /* Avoid duplicate calls if a child of a native container contains the focus.
     GTK will call the callback for the child and for the container.
     Ignore the one sent to the parent. For now only IupDialog and IupTabs
     have keyboard signals intercepted.
     */
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
    return TRUE;

  /* in the previous callback the dialog could be destroyed */
  if (iupObjectCheck(ih))
  {
    /* this is called only for canvas */
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

    if (iupKeyProcessNavigation(ih, code, evt->state & GDK_SHIFT_MASK))
      return TRUE;

    /* compensate the show-help limitation. 
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

  (void)widget;
  return FALSE;
}

gboolean iupgtkKeyReleaseEvent(GtkWidget *widget, GdkEventKey *evt, Ihandle *ih)
{
  /* this is called only for canvas */
  int result;
  int code = iupgtkKeyDecode(evt);
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

  (void)widget;
  return FALSE;
}

void iupgtkButtonKeySetStatus(guint state, unsigned int but, char* status, int doubleclick)
{
  if (state & GDK_SHIFT_MASK)
    iupKEY_SETSHIFT(status);

  if (state & GDK_CONTROL_MASK)
    iupKEY_SETCONTROL(status); 

  if ((state & GDK_BUTTON1_MASK) || but==1)
    iupKEY_SETBUTTON1(status);

  if ((state & GDK_BUTTON2_MASK) || but==2)
    iupKEY_SETBUTTON2(status);

  if ((state & GDK_BUTTON3_MASK) || but==3)
    iupKEY_SETBUTTON3(status);

  if ((state & GDK_BUTTON4_MASK) || but==4)
    iupKEY_SETBUTTON4(status);

  if ((state & GDK_BUTTON5_MASK) || but==5)
    iupKEY_SETBUTTON5(status);

  if (state & GDK_MOD1_MASK || state & GDK_MOD5_MASK) /* Alt */
    iupKEY_SETALT(status);

  if (state & GDK_MOD4_MASK) /* Apple/Win */
    iupKEY_SETSYS(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);
}

