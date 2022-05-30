/** \file
 * \brief Mac Driver keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#import <Cocoa/Cocoa.h>
// Original code called this, but Xcode fails to find.
//#import <Carbon/HIToolbox/Events.h>
// This works instead. We only need constants and don't need to link Carbon, so we are okay for modern Mac.
//#import <Carbon/Carbon.h>
// I've copied the relevant constants out of 
// Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/Headers/Events.h
// to iupmac_keycodes.h
// so we don't have to worry about the Carbon headers disappearing in the future.
#include "iupmac_keycodes.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "iup.h"
#include "iupcbs.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_key.h"

#include "iupcocoa_drv.h"


typedef struct _Imac2iupkey
{
  int maccode; // the native [nsevent keyCode], as documented in <Carbon/HIToolbox/Events.h>
  int iupcode; // the IUP equivalent base key
  int s_iupcode; // the key with shift (now unused)
  int c_iupcode; // the key with control (now unused)
  int m_iupcode; // the key with alt/option (now unused)
  int y_iupcode; // the key with command (now unused)
} Imac2iupkey;

// EW: I think this table is somewhat obsolete due to IUP changes in 3.9.
// 3.9 purports to support handling keys with multiple modifiers.
// So mapping to just one specific key may not make sense as in s, c, m, y.
// So we will probably no longer use c, m, y. 
// s might possibly be useful still, but we might use [nsevent characters] to get this value a different way.
static Imac2iupkey s_macKeyMap[] = {

{ kVK_Escape,    K_ESC,   K_sESC,    K_cESC,   K_mESC   ,K_yESC   },
//{ kVK_Pause,     K_PAUSE, K_sPAUSE,  K_cPAUSE, K_mPAUSE ,K_yPAUSE },
//{ kVK_Print,     K_Print, K_sPrint,  K_cPrint, K_mPrint ,K_yPrint },
//{ kVK_Menu,      K_Menu,  K_sMenu,   K_cMenu,  K_mMenu  ,K_yMenu  },
                                                       
{ kVK_Home,      K_HOME,  K_sHOME,   K_cHOME,  K_mHOME  ,K_yHOME  },
{ kVK_UpArrow,        K_UP,    K_sUP,     K_cUP,    K_mUP    ,K_yUP    },
{ kVK_PageUp,     K_PGUP,  K_sPGUP,   K_cPGUP,  K_mPGUP  ,K_yPGUP  },
{ kVK_LeftArrow,      K_LEFT,  K_sLEFT,   K_cLEFT,  K_mLEFT  ,K_yLEFT  },
//{ kVK_Begin,     K_MIDDLE,K_sMIDDLE, K_cMIDDLE,K_mMIDDLE,K_yMIDDLE},
{ kVK_RightArrow,     K_RIGHT, K_sRIGHT,  K_cRIGHT, K_mRIGHT ,K_yRIGHT },
{ kVK_End,       K_END,   K_sEND,    K_cEND,   K_mEND   ,K_yEND   },
{ kVK_DownArrow,      K_DOWN,  K_sDOWN,   K_cDOWN,  K_mDOWN  ,K_yDOWN  },
{ kVK_PageDown,      K_PGDN,  K_sPGDN,   K_cPGDN,  K_mPGDN  ,K_yPGDN  },
//{ kVK_Insert,    K_INS,   K_sINS,    K_cINS,   K_mINS   ,K_yINS   },
{ kVK_ForwardDelete,    K_DEL,   K_sDEL,    K_cDEL,   K_mDEL   ,K_yDEL   },
{ kVK_Space,     K_SP,    K_sSP,     K_cSP,    K_mSP    ,K_ySP    },
{ kVK_Tab,       K_TAB,   K_sTAB,    K_cTAB,   K_mTAB   ,K_yTAB   },
{ kVK_Return,    K_CR,    K_sCR,     K_cCR,    K_mCR    ,K_yCR    },
{ kVK_Delete, K_BS,    K_sBS,     K_cBS,    K_mBS    ,K_yBS    },


//{ kVK_Command, ,    0,     0,    0,    0     },
{ kVK_Shift, K_LSHIFT,    0,     0,    0,    0    },
{ kVK_CapsLock, K_CAPS,    0,     0,    0,    0    },
{ kVK_Option, K_LALT,    0,     0,    0,    0    },
{ kVK_Control, K_LCTRL,    0,     0,    0,    0    },
//{ kVK_RightCommand, ,    0,     0,    0,    0    },
{ kVK_RightShift, K_RSHIFT,    0,     0,    0,    0    },
{ kVK_RightOption, K_RALT,    0,     0,    0,    0    },
{ kVK_RightControl, K_RCTRL,    0,     0,    0,    0    },
//{ kVK_Function, ,    0,     0,    0,    0    },

// Not sure what do about Help vs. Insert. But typically Insert is the keypad 0, and I think you may have needed to hold Fn to trigger it. (Doesn't work for me.)
// So I will pick HELP since we technically need it for the HELP_CB (even though I don't think anybody can trigger it today).
{ kVK_Help, K_HELP, K_HELP, K_HELP, K_HELP },


{ kVK_ANSI_1,   K_1, K_exclam,      K_c1, K_m1, K_y1 },
{ kVK_ANSI_2,   K_2, K_at,          K_c2, K_m2, K_y2 },
{ kVK_ANSI_3,   K_3, K_numbersign,  K_c3, K_m3, K_y3 },
{ kVK_ANSI_4,   K_4, K_dollar,      K_c4, K_m4, K_y4 },
{ kVK_ANSI_5,   K_5, K_percent,     K_c5, K_m5, K_y5 },
{ kVK_ANSI_6,   K_6, K_circum,      K_c6, K_m6, K_y6 },
{ kVK_ANSI_7,   K_7, K_ampersand,   K_c7, K_m7, K_y7 },
{ kVK_ANSI_8,   K_8, K_asterisk,    K_c8, K_m8, K_y8 },
{ kVK_ANSI_9,   K_9, K_parentleft,  K_c9, K_m9, K_y9 },
{ kVK_ANSI_0,   K_0, K_parentright, K_c0, K_m0, K_y0 },


{ kVK_ANSI_A,   K_a, K_A, K_cA, K_mA, K_yA },
{ kVK_ANSI_B,   K_b, K_B, K_cB, K_mB, K_yB },
{ kVK_ANSI_C,   K_c, K_C, K_cC, K_mC, K_yC },
{ kVK_ANSI_D,   K_d, K_D, K_cD, K_mD, K_yD },
{ kVK_ANSI_E,   K_e, K_E, K_cE, K_mE, K_yE },
{ kVK_ANSI_F,   K_f, K_F, K_cF, K_mF, K_yF },
{ kVK_ANSI_G,   K_g, K_G, K_cG, K_mG, K_yG },
{ kVK_ANSI_H,   K_h, K_H, K_cH, K_mH, K_yH },
{ kVK_ANSI_I,   K_i, K_I, K_cI, K_mI, K_yI },
{ kVK_ANSI_J,   K_j, K_J, K_cJ, K_mJ, K_yJ },
{ kVK_ANSI_K,   K_k, K_K, K_cK, K_mK, K_yK },
{ kVK_ANSI_L,   K_l, K_L, K_cL, K_mL, K_yL },
{ kVK_ANSI_M,   K_m, K_M, K_cM, K_mM, K_yM },
{ kVK_ANSI_N,   K_n, K_N, K_cN, K_mN, K_yN },
{ kVK_ANSI_O,   K_o, K_O, K_cO, K_mO, K_yO },
{ kVK_ANSI_P,   K_p, K_P, K_cP, K_mP, K_yP },
{ kVK_ANSI_Q,   K_q, K_Q, K_cQ, K_mQ, K_yQ },
{ kVK_ANSI_R,   K_r, K_R, K_cR, K_mR, K_yR },
{ kVK_ANSI_S,   K_s, K_S, K_cS, K_mS, K_yS },
{ kVK_ANSI_T,   K_t, K_T, K_cT, K_mT, K_yT },
{ kVK_ANSI_U,   K_u, K_U, K_cU, K_mU, K_yU },
{ kVK_ANSI_V,   K_v, K_V, K_cV, K_mV, K_yV },
{ kVK_ANSI_W,   K_w, K_W, K_cW, K_mW, K_yW },
{ kVK_ANSI_X,   K_x, K_X, K_cX, K_mX, K_yX },
{ kVK_ANSI_Y,   K_y, K_Y, K_cY, K_mY, K_yY },
{ kVK_ANSI_Z,   K_z, K_Z, K_cZ, K_mZ, K_yZ },


{ kVK_F1,  K_F1,  K_sF1,  K_cF1,  K_mF1,  K_yF1 },
{ kVK_F2,  K_F2,  K_sF2,  K_cF2,  K_mF2,  K_yF2 },
{ kVK_F3,  K_F3,  K_sF3,  K_cF3,  K_mF3,  K_yF3 },
{ kVK_F4,  K_F4,  K_sF4,  K_cF4,  K_mF4,  K_yF4 },
{ kVK_F5,  K_F5,  K_sF5,  K_cF5,  K_mF5,  K_yF5 },
{ kVK_F6,  K_F6,  K_sF6,  K_cF6,  K_mF6,  K_yF6 },
{ kVK_F7,  K_F7,  K_sF7,  K_cF7,  K_mF7,  K_yF7 },
{ kVK_F8,  K_F8,  K_sF8,  K_cF8,  K_mF8,  K_yF8 },
{ kVK_F9,  K_F9,  K_sF9,  K_cF9,  K_mF9,  K_yF9 },
{ kVK_F10, K_F10, K_sF10, K_cF10, K_mF10, K_yF10 },
{ kVK_F11, K_F11, K_sF11, K_cF11, K_mF11, K_yF11 },
{ kVK_F12, K_F12, K_sF12, K_cF12, K_mF12, K_yF12 },
{ kVK_F13, K_F13, K_sF13, K_cF13, K_mF13, K_yF13 },
{ kVK_F14, K_F14, K_sF14, K_cF14, K_mF14, K_yF14 },
{ kVK_F16, K_F16, K_sF16, K_cF16, K_mF16, K_yF16 },
{ kVK_F17, K_F17, K_sF17, K_cF17, K_mF17, K_yF17 },
{ kVK_F18, K_F18, K_sF18, K_cF18, K_mF18, K_yF18 },
{ kVK_F19, K_F19, K_sF19, K_cF19, K_mF19, K_yF19 },
{ kVK_F20, K_F20, K_sF20, K_cF20, K_mF20, K_yF20 },

{ kVK_ANSI_Semicolon,   K_semicolon,    K_colon,      K_cSemicolon, K_mSemicolon, K_ySemicolon },
{ kVK_ANSI_Equal,       K_equal,        K_plus,       K_cEqual,     K_mEqual,  K_yEqual },
{ kVK_ANSI_Comma,       K_comma,        K_less,       K_cComma,     K_mComma,  K_yComma },
{ kVK_ANSI_Minus,       K_minus,        K_underscore, K_cMinus,     K_mMinus,  K_yMinus },
{ kVK_ANSI_Period,      K_period,       K_greater,    K_cPeriod,    K_mPeriod, K_yPeriod },
{ kVK_ANSI_Slash,       K_slash,        K_question,   K_cSlash,     K_mSlash,  K_ySlash },
{ kVK_ANSI_Grave,       K_grave,        K_tilde,      0, 0, 0 },
{ kVK_ANSI_LeftBracket, K_bracketleft,  K_braceleft,  K_cBracketleft, K_mBracketleft, K_yBracketleft },
{ kVK_ANSI_Backslash,   K_backslash,    K_bar,        K_cBackslash,   K_mBackslash,   K_yBackslash },
{ kVK_ANSI_RightBracket,K_bracketright, K_braceright, K_cBracketright,K_mBracketright,K_yBracketright },
{ kVK_ANSI_Quote,  K_apostrophe,   K_quotedbl,   0, 0, 0 },

{ kVK_ANSI_Keypad0,   K_0, K_0,  K_c0, K_m0, K_y0 },
{ kVK_ANSI_Keypad1,   K_1, K_1,  K_c1, K_m1, K_y1 },
{ kVK_ANSI_Keypad2,   K_2, K_2,  K_c2, K_m2, K_y2 },
{ kVK_ANSI_Keypad3,   K_3, K_3,  K_c3, K_m3, K_y3 },
{ kVK_ANSI_Keypad4,   K_4, K_4,  K_c4, K_m4, K_y4 },
{ kVK_ANSI_Keypad5,   K_5, K_5,  K_c5, K_m5, K_y5 },
{ kVK_ANSI_Keypad6,   K_6, K_6,  K_c6, K_m6, K_y6 },
{ kVK_ANSI_Keypad7,   K_7, K_7,  K_c7, K_m7, K_y7 },
{ kVK_ANSI_Keypad8,   K_8, K_8,  K_c8, K_m8, K_y8 },
{ kVK_ANSI_Keypad9,   K_9, K_9,  K_c9, K_m9, K_y9 },
{ kVK_ANSI_KeypadMultiply,  K_asterisk, K_sAsterisk, K_cAsterisk, K_mAsterisk, K_yAsterisk },
{ kVK_ANSI_KeypadPlus,       K_plus,     K_sPlus,     K_cPlus,     K_mPlus,     K_yPlus },
{ kVK_ANSI_KeypadMinus,  K_minus,    K_sMinus,    K_cMinus,    K_mMinus,    K_yMinus },
{ kVK_ANSI_KeypadDecimal,   K_period,   K_sPeriod,   K_cPeriod,   K_mPeriod,   K_yPeriod },
{ kVK_ANSI_KeypadDivide,    K_slash,    K_sSlash,    K_cSlash,    K_mSlash,    K_ySlash },

/*

{ kVK_ccedilla,     K_ccedilla, K_Ccedilla, K_cCcedilla, K_mCcedilla, K_yCcedilla },
{ kVK_Ccedilla,     K_ccedilla, K_Ccedilla, K_cCcedilla, K_mCcedilla, K_yCcedilla },

{ kVK_dead_tilde,      K_tilde, K_circum, 0, 0, 0 },
{ kVK_dead_acute,      K_acute, K_grave,  0, 0, 0 },
{ kVK_dead_grave,      K_grave, K_tilde,  0, 0, 0 },
{ kVK_dead_circumflex, K_tilde, K_circum, 0, 0, 0 },

{ kVK_KP_F1,        K_F1,    K_sF1,    K_cF1,    K_mF1,    K_yF1 },
{ kVK_KP_F2,        K_F2,    K_sF2,    K_cF2,    K_mF2,    K_yF2 },
{ kVK_KP_F3,        K_F3,    K_sF3,    K_cF3,    K_mF3,    K_yF3 },
{ kVK_KP_F4,        K_F4,    K_sF4,    K_cF4,    K_mF4,    K_yF4 },
{ kVK_KP_Space,     K_SP,    K_sSP,    K_cSP,    K_mSP    ,K_ySP    },
{ kVK_KP_Tab,       K_TAB,   K_sTAB,   K_cTAB,   K_mTAB   ,K_yTAB   },
*/

{ kVK_ANSI_KeypadClear, K_CLEAR,    K_sCLEAR,    K_cCLEAR,    K_mCLEAR,    K_yCLEAR },
{ kVK_ANSI_KeypadEquals,     K_equal, 0,        K_cEqual, K_mEqual, K_yEqual },
{ kVK_ANSI_KeypadEnter,     K_CR,    K_sCR,     K_cCR,    K_mCR,    K_yCR    },
/*
{ kVK_KP_Home,      K_HOME,  K_sHOME,   K_cHOME,  K_mHOME,  K_yHOME  },
{ kVK_KP_Up,        K_UP,    K_sUP,     K_cUP,    K_mUP,    K_yUP    },
{ kVK_KP_Page_Up,   K_PGUP,  K_sPGUP,   K_cPGUP,  K_mPGUP,  K_yPGUP  },
{ kVK_KP_Left,      K_LEFT,  K_sLEFT,   K_cLEFT,  K_mLEFT,  K_yLEFT  },
{ kVK_KP_Begin,     K_MIDDLE,K_sMIDDLE, K_cMIDDLE,K_mMIDDLE,K_yMIDDLE},
{ kVK_KP_Right,     K_RIGHT, K_sRIGHT,  K_cRIGHT, K_mRIGHT, K_yRIGHT },
{ kVK_KP_End,       K_END,   K_sEND,    K_cEND,   K_mEND,   K_yEND   },
{ kVK_KP_Down,      K_DOWN,  K_sDOWN,   K_cDOWN,  K_mDOWN,  K_yDOWN  },
{ kVK_KP_Page_Down, K_PGDN,  K_sPGDN,   K_cPGDN,  K_mPGDN,  K_yPGDN  },
{ kVK_KP_Insert,    K_INS,   K_sINS,    K_cINS,   K_mINS,   K_yINS   },
{ kVK_KP_Delete,    K_DEL,   K_sDEL,    K_cDEL,   K_mDEL,   K_yDEL   }
*/



/* JIS keyboards only*/
{ kVK_JIS_KeypadComma, K_comma,    K_sComma,    K_cComma,    K_mComma,    K_yComma }

};

#if 0
// original code used guint. Not sure what those are; changing to NSUInteger
void iupmacKeyEncode(int key, NSUInteger *keyval, NSUInteger *state)
{
  int i, iupcode = key & 0xFF; /* 0-255 interval */
  int count = sizeof(s_macKeyMap)/sizeof(s_macKeyMap[0]);
  for (i = 0; i < count; i++)
  {
    Imac2iupkey* key_map = &(s_macKeyMap[i]);
    if (key_map->iupcode == iupcode)
    {
      *keyval = key_map->maccode;
      *state = 0;

      if (iupcode != key)
      {
        if (key_map->c_iupcode == key)
          *state = kVK_Control;
        else if (key_map->m_iupcode == key)
          *state = kVK_Option;
        else if (key_map->y_iupcode == key)
          *state = kVK_Command;
        else if (key_map->s_iupcode == key)
          *state = kVK_Shift;
      }
      return;
    }
    else if (key_map->s_iupcode == key)   /* There are Shift keys bellow 256 */
    {
      *keyval = key_map->maccode;
      *state = kVK_Shift;
      return;
    }
  }
}
#endif
#if 0
static int cocoaKeyMap2Iup(NSEvent* ns_event, int mac_key_code)
{
  int code = 0;
  if (state & kVK_Control)   /* Ctrl */
    code = s_macKeyMap[i].c_iupcode;
  else if (state & kVK_Option ||
           state & kVK_RightOption) /* Alt */
    code = s_macKeyMap[i].m_iupcode;
  else if (state & kVK_Command) /* Apple/Win */
    code = s_macKeyMap[i].y_iupcode;
  else if (state & kVK_CapsLock) /* CapsLock */
  {
    if ((state & kVK_Shift) || !iupKeyCanCaps(s_macKeyMap[i].iupcode))
      return s_macKeyMap[i].iupcode;
    else
      code = s_macKeyMap[i].s_iupcode;
  }
  else if (state & kVK_Shift) /* Shift */
    code = s_macKeyMap[i].s_iupcode;
  else
    return s_macKeyMap[i].iupcode;

  if (!code)
    code = s_macKeyMap[i].iupcode;

  return code;
}
#endif

// This is a copy & modification of iupKeyCallKeyCb because I don't think iupKeyCallKeyCb is correct for this platform.
// First, we need to distinguish between if the callback exists and returned IUP_DEFAULT, vs. the callback doesn't exist.
// The reason is that in the later, we need to call super to pass the event up the responder chain.
// Otherwise, various subtle things won't work correctly, from context menus to system beeps.
// Second, it seems that the original implementation loops through the ih->parents manually?
// I think this is problematic because Cocoa will normally go up the responder chain normally, so we may get callbacks triggered twice.
// I think circumventing the responder chain is only going to break things.
#define IUPCOCOA_NO_CALLBACK -15
static int iupCocoaKeyCallKeyCb(Ihandle *ih, int code)
{
  char* name = iupKeyCodeToName(code);
//  for (; ih; ih = ih->parent) // disabled. rely on the responder chain
  {
    IFni cb = NULL;
    if (name)
      cb = (IFni)IupGetCallback(ih, name);
    if (!cb)
      cb = (IFni)IupGetCallback(ih, "K_ANY");

    if (cb)
    {
      int ret = cb(ih, code);
      return ret;
    }
	else
	{
      return IUPCOCOA_NO_CALLBACK;
	}
	  
  }
}

// This is a copy & modification of iupKeyCallKeyPressCb because I don't think iupKeyCallKeyCb is correct for this platform.
// First, we need to distinguish between if the callback exists and returned IUP_DEFAULT, vs. the callback doesn't exist.
// The reason is that in the later, we need to call super to pass the event up the responder chain.
static int iupCocoaKeyCallKeyPressCb(Ihandle *ih, int code, int press)
{
  IFnii cb = (IFnii)IupGetCallback(ih, "KEYPRESS_CB");
  if (cb)
  {
    return cb(ih, code, press);
  }
  else
  {
    return IUPCOCOA_NO_CALLBACK;
  }
}

/*
IUP has some behaviors that surprise me, and we need to special case.
- Pressing modifier keys also triggers a key event. Cocoa doesn't normally do this, be we can support using flagsChanged:
- If the character pressed is an ASCII key that has different representations if the shift key is pressed (e.g. a vs. A, or 1 vs. !),
we are supposed to throw away the shift modifier and report the evaluated character (e.g. A). This means shift-A is not distinguishable from caps-lock A.
I think the GTK code filters between K_exclam (!) through K_tilde (~) (in ASCII codes).
- It seems that IUP seems to (| 0xFF80) to a lot of its key constant values. (e.g. K_DEL is 0xFFFF instead of the ASCII 0x7F). I don't understand this.


Additional gotcha:
Apple does key equivalents. For certain key combinations (e.g. shift-option-e yields ´). International keyboards may also do something similar.
This is intended to help type useful characters that might not have a direct key mapping. Text (as opposed to games) is the intent for this feature.
It is not clear what IUP intends with its API, since it is a native GUI kit and it treats shift/caps-lock as different keys than the non-shift/non-capslock (which implies the combos should be respected),
but Canvas is one of the main examples, and a Canvas is often more game-like. Also, the API fires for every keypress, even modifiers.
So I'm stuck between two different ideas, and also two different implementations.
- [the_event characters]; will get us the combo string. But IUP only really defines ASCII values so a lot of these will get lost.
However, if we convert this to ASCII and take the first character, (assuming we don't get nil from a lossy conversion), this does also solve how to get the shift/caps-lock evaluated key instead of the raw keycode.
- Otherwise we could raw keycodes (defined in Carbon headers) and try to map them manually to IUP keys. But we will lose the combo characters.

*/
static int cocoaKeyDecode(NSEvent* ns_event, int mac_key_code)
{
	const size_t array_length = sizeof(s_macKeyMap)/sizeof(s_macKeyMap[0]);
	int iup_base_key = 0;
	int iup_result_key = 0;
	size_t i;
	// Iterate through the whole mac/iup keycode array and find the iup base key for the current mac key code.
	// TODO: We could reorganize the data structure so it is ordered by the Native Mac Key index. Assuming no holes (only an initial offset), we could make this O(1) instead of O(N).
	for(i = 0; i < array_length; i++)
	{
		if(s_macKeyMap[i].maccode == mac_key_code)
		{
			// Get the base key. We'll sort out shift and capslock further down.
			iup_base_key = s_macKeyMap[i].iupcode;
			break;
		}
	}

	// should we give up since we don't have an IUP key equivalent. 
	// (Or should we try [nsevent characters] first?)
	if(0 == iup_base_key)
	{
		return 0;
	}
	// At this point, we should have an iup_base_key and also i points to the array index in the s_macKeyMap if we need it.
	
	// For ASCII value keys that respond to shift/capslock, we are supposed to return the transformed values, not the base key.
	// Also, in this case, we are supposed to throw away (not report) the shift modifier being down.
	// The GTK code seems to filter the ASCII codes between K_exclam (!) through K_tilde (~) to do this.
	// Since we need character equivalents, instead of doing the manual checks for shift and capslock 
	// (and figuring out how things are supposed to behave in edge cases like when both are on)
	// we can use [nsevent characters] to get us what Apple interprets the keys as.
	// Remember that Iup keys are in ASCII values, so we can do this range check following the ASCII table.
	// BUG: [NSEvent characters] throws an exception for non-keyUp/keyDown events. I'm calling this for modifier changes (flagChanged).
	// So I need avoid this block on NSEventTypeFlagsChanged, and only allow key events
    if( (iup_base_key >= K_exclam) && (iup_base_key <= K_tilde)
    	&& ( (NSEventTypeKeyDown == [ns_event type]) || (NSEventTypeKeyUp == [ns_event type]) )
	)
	{
		// turn event info into NSString
		NSString* character_string = [ns_event characters];
//		NSLog(@"keydown string: %@", character_string);

		// forces NSString to convert to ASCII-equivalent, if possible (lossy conversion)
		NSData* convert_to_data = [character_string dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
		// get ascii string as NSString to use NSString methods on it
		NSString* ascii_string = [[[NSString alloc] initWithData:convert_to_data encoding:NSASCIIStringEncoding] autorelease];
//		NSLog(@"ascii string: %@", ascii_string);


		if([ascii_string length] > 0)
		{
			unichar first_char = [ascii_string characterAtIndex:0];
			char ascii_char = (char)first_char;
			//int cb_val = 0xFF80 | ascii_char; // Many of Iup's constants seem to have leading bits set. This hack will add those bits. I don't seem to need it for the K_exclam to K_tilde range.
			//cb_result = iupKeyCallKeyCb(ih, cb_val);
			iup_result_key = (int)ascii_char;
		}
		// Not sure about this case. If the user did some funny combo, we might get something outside the ascii range
		else if([character_string length] > 0)
		{
			// FIXME: figure out what we should do here
			// are we going to change the spec?
			// or formalize and figure out what an official solution should be?


			// Should we abort or give this to the user?
#if 0
			unichar fallback_char = [character_string characterAtIndex:0];
			iup_result_key = (int)fallback_char;
#else
			return 0;
#endif
			


		}
		else
		{
			// Should we abort or give this to the user?
			return 0;

		}



	}
	else
	{
		iup_result_key = iup_base_key;
		// Outside this range, we need to report the shift key if it is pressed.
		if([ns_event modifierFlags] & NSEventModifierFlagShift)
		{
			iup_result_key |= iup_XkeyShift(iup_result_key);  
		}	
	}
	
	// Report the other modifiers

	if([ns_event modifierFlags] & NSEventModifierFlagControl)
	{
		iup_result_key |= iup_XkeyCtrl(iup_result_key);  
	}
	if([ns_event modifierFlags] & NSEventModifierFlagOption)
	{
		iup_result_key |= iup_XkeyAlt(iup_result_key);  
	}
	if([ns_event modifierFlags] & NSEventModifierFlagCommand)
	{
		iup_result_key |= iup_XkeySys(iup_result_key);  
	}

	return iup_result_key;
}


// All keyDown: and keyUp: overrides can call this method to handle K_ANY, the Canvas Keypress, and HELP_CB.
// I'm trying to match the iupgtk semantics:
// GTK doc's: "Returning TRUE indicates that the event has been handled, and that it should not propagate further."
// IUP seemas to follow this.
// So Returns true if Iup handled the key event. False if there was no callback or it return IUP_CONTINUE.
// The caller should usually invoke super if this returns false to make sure the event is passed up the responder chain.
// IUP_IGNORE means do not handle and do not pass up the responder chain, thus returns true.
bool iupCocoaKeyDownEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code)
{
  int result;
  int iup_key_code;
//  bool caller_should_not_propagate = false;
  bool caller_should_propagate = true;


  // This was here before, but I did not see this in th GTK implementation. Not sure about this.
  if (!ih->iclass->is_interactive)
  {
	  return false;
  }	

  iup_key_code = cocoaKeyDecode(ns_event, mac_key_code);
  if(0 == iup_key_code)
  { 
    return false;
  }

  result = iupCocoaKeyCallKeyCb(ih, iup_key_code);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
	caller_should_propagate = false;
	return !caller_should_propagate;
  }
  else if (result == IUP_IGNORE)
  {
	caller_should_propagate = false;
	return !caller_should_propagate;
  }
  else if (result == IUP_CONTINUE)
  {
    caller_should_propagate = true;
  }
  else if (result == IUPCOCOA_NO_CALLBACK)
  {
    caller_should_propagate = true;
  }
  else
  {
    caller_should_propagate = false;
  }

  /* in the previous callback the dialog could be destroyed */
  if (iupObjectCheck(ih))
  {
    /* this is called only for canvas */
    if (ih->iclass->nativetype == IUP_TYPECANVAS) 
    {
      result = iupCocoaKeyCallKeyPressCb(ih, iup_key_code, true);
      if (result == IUP_CLOSE)
      {
        IupExitLoop();
		caller_should_propagate = false;
		return !caller_should_propagate;
      }
	  else if (result == IUP_IGNORE)
      {
		caller_should_propagate = false;
		return !caller_should_propagate;
      }
	  else if (result == IUP_CONTINUE)
	  {
	  	// BUG: This is a bug if the k_any callback did not use IUP_CONTINUE
		caller_should_propagate = true;
	  }
      else if (result == IUPCOCOA_NO_CALLBACK)
      {
        caller_should_propagate = true;
      }
  	  else
      {
        caller_should_propagate = false;
      }

    }

	// NOTE: I'm finding this function problematic and worry we may need to rethink this part of the design.
	// It tries to manage things like arrow keys and tab keys, ESC, Return, and function keys.
	// But their are assumptions that rub up against normal Mac behaviors.
	// For example:
	// TAB: is supposed to trigger the next/previous key loop/focus widget. While this is the correct key mapping, IUP doesn't invoke the native next/prev mechanism.
	// Additionally, this only works if we have some how gotten the event through Responder chain to come here. This works for our custom subclasses, but is less clear how to get the native unsubclassed widgets to come here.
	// And generally speaking, the better Cocoa thing to do is not handle these at all, and pass the event up the responder chain to let the OS deal with them correct.
	// Arrows: Currently when I test on IupOutlineView, I think this returns false and we get to do the correct thing with the responder chain assuming the user didn't eat the event. However
	// Function Keys: Hard coding F12 or any other function key is a bad idea.
	// For now this is enabled, but I may need to remove this.
	// UPDATE: This is incorrectly setting focus to widgets that shouldn't have focus (all the accept responder rules) under the "text boxes and lists only" preference in System Preferences.
	// Also, it seems to keep native widgets out of the chain???
	// I'm diabling for now.
#if 0
    if (iupKeyProcessNavigation(ih, iup_key_code, [ns_event modifierFlags] & NSEventModifierFlagShift))
      return true;
#endif
	// Mac uses the Help menu for help, not keyboard shortcuts. F1 should not be used.
	// Function keys are typically remapped by users nowadays to do custom system shortcuts since they are never used as standard keys. We don't want to break users.
	// Apple used to have a dedicated Help button on the keyboard, but we don't have an IUP keycode for that.
	// And that button hasn't been around in many years.
	// This button seems to be same as Insert (also not around in a long time).
	// Also notice the constant: NSEventModifierFlagHelp 
	// I just added a new IUP keycode: K_HELP
	// I'm ignoring the result value because I don't want it to override the callbacks above. Chances are nobody is going to trigger this.
    if (iup_key_code == K_HELP)
    {
      Icallback cb = IupGetCallback(ih, "HELP_CB");
      if (cb)
      {
		  result = cb(ih);
		  if (result == IUP_CLOSE)
		  {
			  IupExitLoop();
              caller_should_propagate = false;
              return !caller_should_propagate;
		  }
		  else if (result == IUP_IGNORE)
          {
              caller_should_propagate = false;
              return !caller_should_propagate;
		  }
		  else if (result == IUP_CONTINUE)
		  {
			// BUG: This is a bug if the k_any callback did not use IUP_CONTINUE
//			caller_should_propagate = true;
		  }
          else if (result == IUPCOCOA_NO_CALLBACK)
          {
//            caller_should_propagate = true;
          }
          else
          {
 //           caller_should_propagate = false;
          }
      }
    }
  }

  return !caller_should_propagate;
}

// I'm trying to match the iupgtk semantics:
// GTK doc's: "Returning TRUE indicates that the event has been handled, and that it should not propagate further."
// IUP seemas to follow this.
// So Returns true if Iup handled the key event. False if there was no callback or it return IUP_CONTINUE.
// The caller should usually invoke super if this returns false to make sure the event is passed up the responder chain.
// IUP_IGNORE means do not handle and do not pass up the responder chain, thus returns true.
bool iupCocoaKeyUpEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code)
{
  int result;
  int iup_key_code;
//  bool caller_should_not_propagate = false;
  bool caller_should_propagate = true;


  // This was here before, but I did not see this in th GTK implementation. Not sure about this.
  if (!ih->iclass->is_interactive)
  {
	  return false;
  }

  iup_key_code = cocoaKeyDecode(ns_event, mac_key_code);
  if(0 == iup_key_code)
  {
    return false;
  }

  result = iupCocoaKeyCallKeyPressCb(ih, iup_key_code, false);
  if (result == IUP_CLOSE)
  {
    IupExitLoop();
	caller_should_propagate = false;
	return !caller_should_propagate;
  }
  else if (result == IUP_IGNORE)
  {
	caller_should_propagate = false;
	return !caller_should_propagate;
  }
  else if (result == IUP_CONTINUE)
  {
    caller_should_propagate = true;
  }
  else if (result == IUPCOCOA_NO_CALLBACK)
  {
    caller_should_propagate = true;
  }
  else
  {
    caller_should_propagate = false;
  }
	
  return !caller_should_propagate;
}


// I learned the hard way that the IUP code for up and down is not symmetrical.
// For example, the NextKeyView triggers twice accidentally (once for key down, again for key up) if we use the same routine for both.
// So this function splits the events appropriately.
bool iupCocoaKeyEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code, bool is_pressed)
{
	if(is_pressed)
	{
		return iupCocoaKeyDownEvent(ih, ns_event, mac_key_code);
	}
	else
	{
		return iupCocoaKeyUpEvent(ih, ns_event, mac_key_code);
	}
}

// IUP triggers key callbacks for *most* modifiers. Cocoa does not do generally do this.
// But we can implement it with flagsChanged:
// But flagsChanged does not give us information about whether the key was pressed or released.
// So we can't call iupCocoaKeyEvent() directly.
// But we can figure out the pressed state by jumping through some hoops.
// This function does that and then calls iupCocoaKeyEvent().
// All flagsChanged: overrides can call this function.
// WARNING: IUP does not currently provide a "SYS" keycode. This means no platform triggers a key event for Command/Win/Meta
bool iupCocoaModifierEvent(Ihandle *ih, NSEvent* ns_event, int mac_key_code)
{
	bool is_pressed = false;
	/*
    NSEventModifierFlagCapsLock           = 1 << 16, // Set if Caps Lock key is pressed.
    NSEventModifierFlagShift              = 1 << 17, // Set if Shift key is pressed.
    NSEventModifierFlagControl            = 1 << 18, // Set if Control key is pressed.
    NSEventModifierFlagOption             = 1 << 19, // Set if Option or Alternate key is pressed.
    NSEventModifierFlagCommand            = 1 << 20, // Set if Command key is pressed.
    NSEventModifierFlagNumericPad         = 1 << 21, // Set if any key in the numeric keypad is pressed.
    NSEventModifierFlagHelp               = 1 << 22, // Set if the Help key is pressed.
    NSEventModifierFlagFunction           = 1 << 23, // Set if any function key is pressed.
*/

	// The NSEvent doesn't directly tell us if the key was pressed or released.
	// We can attempt to figure this out by looking at the mac_key_code, and then seeing if the modifier mask is set or not.
	// e.g. If we get LSHIFT, is the left shift in the mask?
	// Problem: Apple doesn't seem to give us the masks for left-vs-right modifierFlags.
	// However, I can run these manually and record the values.
	
	/*
		Left-shift: 	0x20002
		Right-shift: 	0x20004
		Left-ctrl:		0x40001
		Right-ctrl: 	0x42000
		Left-option:	0x80020
		Right-option: 	0x80040
		Left-command:	0x100008
		Right-command: 	0x100010

	*/
#define EventModifierFlagShiftLeft 			0x20002
#define EventModifierFlagShiftRight 		0x20004
#define EventModifierFlagControlLeft 		0x40001
#define EventModifierFlagControlRight 		0x42000
#define EventModifierFlagOptionLeft 		0x80020
#define EventModifierFlagOptionRight 		0x80040
#define EventModifierFlagCommandLeft 		0x100008
#define EventModifierFlagCommandRight 		0x100010

//	NSLog(@"iupCocoaModifierEvent: modifiers 0x%X", [ns_event modifierFlags]);
//	NSLog(@"iupCocoaModifierEvent: left&modifiers 0x%X", [ns_event modifierFlags] & EventModifierFlagShiftLeft);
//	NSLog(@"iupCocoaModifierEvent: right&modifiers 0x%X", [ns_event modifierFlags] & EventModifierFlagShiftLeft);

	if(kVK_Shift == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagShiftLeft) == EventModifierFlagShiftLeft)
		{
			is_pressed = true;
		}
	}
	else if(kVK_RightShift == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagShiftRight) == EventModifierFlagShiftRight)
		{
			is_pressed = true;
		}
	}
	else if(kVK_Control == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagControlLeft) == EventModifierFlagControlLeft)
		{
			is_pressed = true;
		}
	}
	else if(kVK_RightControl == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagControlRight) == EventModifierFlagControlRight)
		{
			is_pressed = true;
		}
	}
	else if(kVK_Option == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagOptionLeft) == EventModifierFlagOptionLeft)
		{
			is_pressed = true;
		}
	}
	else if(kVK_RightOption == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagOptionRight) == EventModifierFlagOptionRight)
		{
			is_pressed = true;
		}
	}
	// WARNING: IUP does not currently provide a "SYS" keycode. This means no platform triggers a key event for Command/Win/Meta
	else if(kVK_Command == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagCommandLeft) == EventModifierFlagCommandLeft)
		{
			is_pressed = true;
		}
	}
	// WARNING: IUP does not currently provide a "SYS" keycode. This means no platform triggers a key event for Command/Win/Meta
	else if(kVK_RightCommand == mac_key_code)
	{
		if(([ns_event modifierFlags] & EventModifierFlagCommandRight) == EventModifierFlagCommandRight)
		{
			is_pressed = true;
		}
	}
	
	else if(kVK_CapsLock == mac_key_code)
	{
		if(([ns_event modifierFlags] & NSEventModifierFlagCapsLock) == NSEventModifierFlagCapsLock)
		{
			is_pressed = true;
		}
	}
	else if(kVK_Function == mac_key_code)
	{
		if(([ns_event modifierFlags] & NSEventModifierFlagFunction) == NSEventModifierFlagFunction)
		{
			is_pressed = true;
		}
	}
	// There is no numlock keycode. Clear might be the key, but I think that should be handled in the normal keyDown/keyUp case. Same with the Help key?
	
	
	
	// NSLog(@"iupCocoaModifierEvent: is_pressed %d", is_pressed);

	return iupCocoaKeyEvent(ih, ns_event, mac_key_code, is_pressed);
}


// CGEventSourceFlagsState worries me about App Sandbox because it sounds like this might be able to read events that don't belong to our app.
#if 0
void iupmacButtonKeySetStatus(int keys, int but, char* status, int doubleclick)
{
	CGEventFlags flags =CGEventSourceFlagsState(kCGEventSourceStateCombinedSessionState);
	if (flags & kCGEventFlagMaskShift)
		iupKEY_SETSHIFT(status);
	
	if (flags & kCGEventFlagMaskControl)
		iupKEY_SETCONTROL(status);
	
	if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState,kCGMouseButtonLeft))
		iupKEY_SETBUTTON1(status);
	
	if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState,kCGMouseButtonLeft))
		iupKEY_SETBUTTON2(status);
	
	if (CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState,kCGMouseButtonRight))
		iupKEY_SETBUTTON3(status);
	
	if (doubleclick)
		iupKEY_SETDOUBLE(status);
	
	if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState,kVK_Option))
		iupKEY_SETALT(status);
	
	if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState,kVK_Command))
		iupKEY_SETSYS(status);
}

#else

void iupcocoaButtonKeySetStatus(NSEvent* ns_event, char* out_status)
{
	if([ns_event modifierFlags] & NSEventModifierFlagShift)
	{
		iupKEY_SETSHIFT(out_status);
	}
	if([ns_event modifierFlags] & NSEventModifierFlagControl)
	{
		iupKEY_SETCONTROL(out_status);
	}
	if([ns_event modifierFlags] & NSEventModifierFlagOption)
	{
		iupKEY_SETALT(out_status);
	}
	if([ns_event modifierFlags] & NSEventModifierFlagCommand)
	{
		iupKEY_SETSYS(out_status);
	}
	
	// TODO: Triple-click is also useful to know on Mac. How can we extend the API to support this?
	if([ns_event clickCount] == 2)
	{
		iupKEY_SETDOUBLE(out_status);
	}
	
	
	// Button 0 is left
	// Button 1 is right
	// Button 2 is middle
	// Button 3 keeps going
	switch([ns_event buttonNumber])
	{
		case 0:
		{
		    iupKEY_SETBUTTON1(out_status);
			break;
		}
		case 1:
		{
		    iupKEY_SETBUTTON3(out_status);
			break;
		}
		case 2:
		{
		    iupKEY_SETBUTTON2(out_status);
			break;
		}
		case 3:
		{
		    iupKEY_SETBUTTON4(out_status);
			break;
		}
		case 4:
		{
		    iupKEY_SETBUTTON5(out_status);
			break;
		}
		default:
		{
			break;
		}
	}
}

#endif
