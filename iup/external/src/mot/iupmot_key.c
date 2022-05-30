/** \file
 * \brief Motif Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include <Xm/Xm.h>
#include <X11/keysym.h>

#include <stdlib.h>      
#include <stdio.h>      
#include <ctype.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"
#include "iup_str.h"

#include "iup_drv.h"
#include "iupmot_drv.h"


typedef struct Imot2iupkey
{
  KeySym motcode;
  int iupcode;
} Imot2iupkey;

static Imot2iupkey keypad_remap[] = {
  { XK_KP_0,   K_0 },
  { XK_KP_1,   K_1 },
  { XK_KP_2,   K_2 },
  { XK_KP_3,   K_3 },
  { XK_KP_4,   K_4 },
  { XK_KP_5,   K_5 },
  { XK_KP_6,   K_6 },
  { XK_KP_7,   K_7 },
  { XK_KP_8,   K_8 },
  { XK_KP_9,   K_9 },
  { XK_KP_Multiply,  K_asterisk },
  { XK_KP_Add,       K_plus     },
  { XK_KP_Subtract,  K_minus    },
  { XK_KP_Decimal,   K_period   },
  { XK_KP_Divide,    K_slash    },
  { XK_KP_Separator, K_comma    },
  { XK_KP_F1,  K_F1 },
  { XK_KP_F2,  K_F2 },
  { XK_KP_F3,  K_F3 },
  { XK_KP_F4,  K_F4 },
  { XK_KP_Space,     K_SP     },
  { XK_KP_Tab,       K_TAB    },
  { XK_KP_Equal,     K_equal  },
  { XK_KP_Enter,     K_CR     },
  { XK_KP_Home,      K_HOME   },
  { XK_KP_Up,        K_UP     },
  { XK_KP_Page_Up,   K_PGUP   },
  { XK_KP_Left,      K_LEFT   },
  { XK_KP_Begin,     K_MIDDLE },
  { XK_KP_Right,     K_RIGHT  },
  { XK_KP_End,       K_END    },
  { XK_KP_Down,      K_DOWN   },
  { XK_KP_Page_Down, K_PGDN   },
  { XK_KP_Insert,    K_INS    },
  { XK_KP_Delete,    K_DEL    },
};

static Imot2iupkey osfmotkey_remap[] = {
  { 0x1004FF08, XK_BackSpace },  /*osfBackSpace*/ 
  { 0x1004FF0B, XK_Clear     },  /*osfClear    */ 
  { 0x1004FF1B, XK_Escape    },  /*osfEscape   */ 
  { 0X1004FF41, XK_Prior     },  /*osfPageUp   */ 
  { 0x1004FF42, XK_Next      },  /*osfPageDown */ 
  { 0x1004FF44, XK_Return    },  /*osfActivate */ 
  { 0x1004FF45, XK_F10       },  /*osfMenuBar  */ 
  { 0x1004FF51, XK_Left      },  /*osfLeft     */ 
  { 0x1004FF52, XK_Up        },  /*osfUp       */ 
  { 0x1004FF53, XK_Right     },  /*osfRight    */ 
  { 0x1004FF54, XK_Down      },  /*osfDown     */ 
  { 0x1004FF55, XK_Prior     },  /*osfPrior    */ 
  { 0x1004FF56, XK_Next      },  /*osfNext     */ 
  { 0x1004FF57, XK_End       },  /*osfEndLine  */ 
  { 0x1004FF58, XK_Home      },  /*osfBeginLine*/ 
  { 0x1004FF60, XK_Select    },  /*osfSelect   */ 
  { 0x1004FF63, XK_Insert    },  /*osfInsert   */ 
  { 0x1004FF65, XK_Undo      },  /*osfUndo     */ 
  { 0x1004FF67, XK_Menu      },  /*osfMenu     */ 
  { 0x1004FF69, XK_Escape    },  /*osfCancel   */ 
  { 0x1004FF6A, XK_F1        },  /*osfHelp     */ 
  { 0x1004FFFF, XK_Delete    },  /*osfDelete   */ 
};

static Imot2iupkey other_remap[] = {
  { XK_BackSpace, K_BS  },
  { XK_Tab,       K_TAB },
  { XK_Return,    K_CR  },
  { 0xFE03,       K_RALT},
  { XK_dead_tilde,      K_tilde     },
  { XK_dead_acute,      K_acute     },
  { XK_dead_grave,      K_grave     },
  { XK_dead_circumflex, K_circum    },
  { XK_dead_diaeresis,  K_diaeresis },
};

void iupdrvKeyEncode(int code, unsigned int *keycode, unsigned int *state)
{
  KeySym motcode = (KeySym)iup_XkeyBase(code);

  /* Only need to un-remap these */
  if (motcode == K_BS)  motcode = XK_BackSpace;
  else if (motcode == K_TAB) motcode = XK_Tab;
  else if (motcode == K_CR)  motcode = XK_Return;

  *keycode = (unsigned int)XKeysymToKeycode(iupmot_display, motcode);

  *state = 0;
  if (iup_isCtrlXkey(code))
    *state |= ControlMask;

  if (iup_isAltXkey(code))
    *state |= Mod1Mask;

  if (iup_isSysXkey(code))
    *state |= Mod4Mask;

  if (iup_isShiftXkey(code))
    *state |= ShiftMask;
}

static int motKeyMap2Iup(KeySym motcode, unsigned int state)
{
  int code = (int)motcode;

  if (state & (ControlMask|Mod1Mask|Mod5Mask|Mod4Mask))
  {
    /* If it has some of the other modifiers then use upper case version */
    if (motcode >= K_a && motcode <= K_z)
      code = iup_toupper(motcode);
    else if (motcode==K_ccedilla)
      code = K_Ccedilla;
  }

  if (state & ShiftMask) /* Shift */
  {
    /* only add Shift modifiers for non-ASCii codes, except for K_SP and bellow,
       and except when other modifiers are used */
    if ((motcode < K_exclam || motcode > K_tilde) ||
        (state & (ControlMask|Mod1Mask|Mod5Mask|Mod4Mask)))
      code = iup_XkeyShift(code);
  }

  if (state & ControlMask)   /* Ctrl */
    code = iup_XkeyCtrl(code);

  if (state & Mod1Mask || state & Mod5Mask) /* Alt */
    code = iup_XkeyAlt(code);

  if (state & Mod4Mask) /* Apple/Win */
    code = iup_XkeySys(code);

  return code;
}

KeySym iupmotKeycodeToKeysym(XKeyEvent *evt)
{
  int i;
  Modifiers modifiers;
  KeySym motcode;
                   
  /* Other options:
     motcode = XLookupKeysym(evt, 0);     Ignore modifiers
     motcode = XKeycodeToKeysym(iupmot_display, evt->keycode, 0);   Don't use modifiers
     But the Motif version return "osf KeySyms" that we must translate back to defaults. */
  XmTranslateKey(iupmot_display, evt->keycode, evt->state, &modifiers, &motcode);

  if (motcode > 0x1004FF00 && motcode <= 0x1004FFFF)
  {
    /* remap "osf KeySym" */
    int count = sizeof(osfmotkey_remap)/sizeof(osfmotkey_remap[0]);
    for (i = 0; i < count; i++)
    {
      if (osfmotkey_remap[i].motcode == motcode)
      {
        motcode = (KeySym)osfmotkey_remap[i].iupcode;
        break;
      }
    }
  }

  if ((evt->state & Mod2Mask) && /* NumLock */
      (motcode >= XK_KP_Home) &&
      (motcode <= XK_KP_Delete))
  {
    /* remap to numeric keys when NumLock is on */
    KeySym remap_numkey[] = {XK_KP_7, XK_KP_4, XK_KP_8, XK_KP_6, XK_KP_2, XK_KP_9, XK_KP_3, XK_KP_1, XK_KP_5, XK_KP_0, XK_KP_Decimal};
    motcode = remap_numkey[motcode-XK_KP_Home];
  }

  if (motcode >= 0xFF80 && motcode < 0xFFC0)
  {
    /* remap keypad KeySyms */
    int count = sizeof(keypad_remap)/sizeof(keypad_remap[0]);
    for (i = 0; i < count; i++)
    {
      if (keypad_remap[i].motcode == motcode)
      {
        motcode = (KeySym)keypad_remap[i].iupcode;
        break;
      }
    }
  }

  return motcode;
}

int iupmotKeyDecode(XKeyEvent *evt)
{
  KeySym motcode = iupmotKeycodeToKeysym(evt);

  /* Other maps */
  int i, count = sizeof(other_remap)/sizeof(other_remap[0]);
  for (i = 0; i < count; i++)
  {
    if (other_remap[i].motcode == motcode)
    {
      motcode = (KeySym)other_remap[i].iupcode;
      break;
    }
  }

  return motKeyMap2Iup(motcode, evt->state);
}

KeySym iupmotKeyCharToKeySym(char c)
{
  if (c > 0)
    return (KeySym)c;
  return 0;
}

/* Discards keyrepeat by removing the keypress event from the queue.
 * The pair keyrelease/keypress is always put together in the queue,
 * by removing the keypress, we only worry about keyrelease. In case
 * of a keyrelease, we ignore it if the next event is a keypress (which
 * means repetition. Otherwise it is a real keyrelease.
 *
 * Returns 1 if the keypress is found in the queue and 0 otherwise.
 */
static int motKeyDiscardKeypressRepeat(XEvent *evt)
{
  XEvent ahead;
  if (XEventsQueued(iupmot_display, QueuedAfterReading))
  {
    XPeekEvent(iupmot_display, &ahead);
    if (ahead.type == KeyPress && ahead.xkey.window == evt->xkey.window
        && ahead.xkey.keycode == evt->xkey.keycode && ahead.xkey.time == evt->xkey.time)
    {
      /* Pop off the repeated KeyPress and ignore */
      XNextEvent(iupmot_display, evt);
      /* Ignore the auto repeated KeyRelease/KeyPress pair */
      return 1;
    }
  }
  /* No KeyPress found */
  return 0;
}

/* this is called only for canvas */
void iupmotCanvasKeyReleaseEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  if (motKeyDiscardKeypressRepeat(evt))
  {
    /* call key_press because it was removed from the queue */
    iupmotKeyPressEvent(w, ih, evt, cont);
  }
  else
  {
    int result;
    int code = iupmotKeyDecode((XKeyEvent*)evt);
    if (code == 0) 
      return;
    result = iupKeyCallKeyPressCb(ih, code, 0);
    if (result == IUP_CLOSE)
    {
      IupExitLoop();
      return;
    }
    if (result == IUP_IGNORE)
    {
      *cont = False;
      return;
    }
  }
}

void iupmotKeyPressEvent(Widget w, Ihandle *ih, XEvent *evt, Boolean *cont)
{
  int result;
  int code = iupmotKeyDecode((XKeyEvent*)evt);
  if (code == 0) 
      return;

  result = iupKeyCallKeyCb(ih, code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
    return;
  }
  if (result == IUP_IGNORE)
  {
    *cont = False;
    return;
  }

  /* in the previous callback the dialog could be destroyed */
  if (iupObjectCheck(ih))
  {
    /* this is called only for canvas */
    if (ih->iclass->nativetype==IUP_TYPECANVAS) 
    {
      result = iupKeyCallKeyPressCb(ih, code, 1);
      if (result == IUP_CLOSE)
      {
        IupExitLoop();
        return;
      }
      if (result == IUP_IGNORE)
      {
        *cont = False;
        return;
      }
    }

    if ((((XKeyEvent*)evt)->state & Mod1Mask || ((XKeyEvent*)evt)->state & Mod5Mask))   /* Alt + mnemonic */
    {
      KeySym motcode = iupmotKeycodeToKeysym((XKeyEvent*)evt);
      if (motcode < 128)
      {
        if (iupKeyProcessMnemonic(ih, motcode))
        {
          *cont = False;
          return;
        }
      }
    }

    if (iupKeyProcessNavigation(ih, code, ((XKeyEvent*)evt)->state & ShiftMask))
    {
      *cont = False;
      return;
    }
  }

  (void)w;
}

void iupmotButtonKeySetStatus(unsigned int state, unsigned int but, char* status, int doubleclick)
{
  if (state & ShiftMask)
    iupKEY_SETSHIFT(status);

  if (state & ControlMask)
    iupKEY_SETCONTROL(status); 

  if ((state & Button1Mask) || but==Button1)
    iupKEY_SETBUTTON1(status);

  if ((state & Button2Mask) || but==Button2)
    iupKEY_SETBUTTON2(status);

  if ((state & Button3Mask) || but==Button3)
    iupKEY_SETBUTTON3(status);

  if ((state & Button4Mask) || but==Button4)
    iupKEY_SETBUTTON4(status);

  if ((state & Button5Mask) || but==Button5)
    iupKEY_SETBUTTON5(status);

  if (state & Mod1Mask || state & Mod5Mask) /* Alt */
    iupKEY_SETALT(status);

  if (state & Mod4Mask) /* Apple/Win */
    iupKEY_SETSYS(status);

  if (doubleclick)
    iupKEY_SETDOUBLE(status);
}

