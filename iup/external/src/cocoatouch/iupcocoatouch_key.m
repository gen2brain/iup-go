/** \file
 * \brief iOS UIKit Keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>

#include <stdbool.h>

#include "iup.h"
#include "iupkey.h"

#include "iup_object.h"
#include "iup_class.h"
#include "iup_str.h"
#include "iup_key.h"

#include "iupcocoatouch_drv.h"


typedef struct _IupCocoaTouchKey
{
	UIKeyboardHIDUsage hid;
	int iup_code;
} IupCocoaTouchKey;

static const IupCocoaTouchKey s_keyMap[] = {
	/* Letters */
	{ UIKeyboardHIDUsageKeyboardA, K_a },
	{ UIKeyboardHIDUsageKeyboardB, K_b },
	{ UIKeyboardHIDUsageKeyboardC, K_c },
	{ UIKeyboardHIDUsageKeyboardD, K_d },
	{ UIKeyboardHIDUsageKeyboardE, K_e },
	{ UIKeyboardHIDUsageKeyboardF, K_f },
	{ UIKeyboardHIDUsageKeyboardG, K_g },
	{ UIKeyboardHIDUsageKeyboardH, K_h },
	{ UIKeyboardHIDUsageKeyboardI, K_i },
	{ UIKeyboardHIDUsageKeyboardJ, K_j },
	{ UIKeyboardHIDUsageKeyboardK, K_k },
	{ UIKeyboardHIDUsageKeyboardL, K_l },
	{ UIKeyboardHIDUsageKeyboardM, K_m },
	{ UIKeyboardHIDUsageKeyboardN, K_n },
	{ UIKeyboardHIDUsageKeyboardO, K_o },
	{ UIKeyboardHIDUsageKeyboardP, K_p },
	{ UIKeyboardHIDUsageKeyboardQ, K_q },
	{ UIKeyboardHIDUsageKeyboardR, K_r },
	{ UIKeyboardHIDUsageKeyboardS, K_s },
	{ UIKeyboardHIDUsageKeyboardT, K_t },
	{ UIKeyboardHIDUsageKeyboardU, K_u },
	{ UIKeyboardHIDUsageKeyboardV, K_v },
	{ UIKeyboardHIDUsageKeyboardW, K_w },
	{ UIKeyboardHIDUsageKeyboardX, K_x },
	{ UIKeyboardHIDUsageKeyboardY, K_y },
	{ UIKeyboardHIDUsageKeyboardZ, K_z },

	/* Digit row */
	{ UIKeyboardHIDUsageKeyboard1, K_1 },
	{ UIKeyboardHIDUsageKeyboard2, K_2 },
	{ UIKeyboardHIDUsageKeyboard3, K_3 },
	{ UIKeyboardHIDUsageKeyboard4, K_4 },
	{ UIKeyboardHIDUsageKeyboard5, K_5 },
	{ UIKeyboardHIDUsageKeyboard6, K_6 },
	{ UIKeyboardHIDUsageKeyboard7, K_7 },
	{ UIKeyboardHIDUsageKeyboard8, K_8 },
	{ UIKeyboardHIDUsageKeyboard9, K_9 },
	{ UIKeyboardHIDUsageKeyboard0, K_0 },

	/* Whitespace, editing, navigation */
	{ UIKeyboardHIDUsageKeyboardReturnOrEnter, K_CR },
	{ UIKeyboardHIDUsageKeyboardEscape,        K_ESC },
	{ UIKeyboardHIDUsageKeyboardDeleteOrBackspace, K_BS },
	{ UIKeyboardHIDUsageKeyboardTab,           K_TAB },
	{ UIKeyboardHIDUsageKeyboardSpacebar,      K_SP },
	{ UIKeyboardHIDUsageKeyboardDeleteForward, K_DEL },
	{ UIKeyboardHIDUsageKeyboardHome,          K_HOME },
	{ UIKeyboardHIDUsageKeyboardEnd,           K_END },
	{ UIKeyboardHIDUsageKeyboardPageUp,        K_PGUP },
	{ UIKeyboardHIDUsageKeyboardPageDown,      K_PGDN },
	{ UIKeyboardHIDUsageKeyboardLeftArrow,     K_LEFT },
	{ UIKeyboardHIDUsageKeyboardRightArrow,    K_RIGHT },
	{ UIKeyboardHIDUsageKeyboardUpArrow,       K_UP },
	{ UIKeyboardHIDUsageKeyboardDownArrow,     K_DOWN },
	{ UIKeyboardHIDUsageKeyboardInsert,        K_INS },
	{ UIKeyboardHIDUsageKeyboardPause,         K_PAUSE },
	{ UIKeyboardHIDUsageKeyboardPrintScreen,   K_Print },
	{ UIKeyboardHIDUsageKeyboardMenu,          K_Menu },
	{ UIKeyboardHIDUsageKeyboardHelp,          K_HELP },
	{ UIKeyboardHIDUsageKeyboardScrollLock,    K_SCROLL },

	/* punctuation: base ASCII; modifiers via cocoaTouchKeyApplyModifiers */
	{ UIKeyboardHIDUsageKeyboardHyphen,      K_minus },
	{ UIKeyboardHIDUsageKeyboardEqualSign,   K_equal },
	{ UIKeyboardHIDUsageKeyboardOpenBracket, K_bracketleft },
	{ UIKeyboardHIDUsageKeyboardCloseBracket,K_bracketright },
	{ UIKeyboardHIDUsageKeyboardBackslash,   K_backslash },
	{ UIKeyboardHIDUsageKeyboardSemicolon,   K_semicolon },
	{ UIKeyboardHIDUsageKeyboardQuote,       K_apostrophe },
	{ UIKeyboardHIDUsageKeyboardGraveAccentAndTilde, K_grave },
	{ UIKeyboardHIDUsageKeyboardComma,       K_comma },
	{ UIKeyboardHIDUsageKeyboardPeriod,      K_period },
	{ UIKeyboardHIDUsageKeyboardSlash,       K_slash },

	/* Function keys */
	{ UIKeyboardHIDUsageKeyboardF1,  K_F1 },
	{ UIKeyboardHIDUsageKeyboardF2,  K_F2 },
	{ UIKeyboardHIDUsageKeyboardF3,  K_F3 },
	{ UIKeyboardHIDUsageKeyboardF4,  K_F4 },
	{ UIKeyboardHIDUsageKeyboardF5,  K_F5 },
	{ UIKeyboardHIDUsageKeyboardF6,  K_F6 },
	{ UIKeyboardHIDUsageKeyboardF7,  K_F7 },
	{ UIKeyboardHIDUsageKeyboardF8,  K_F8 },
	{ UIKeyboardHIDUsageKeyboardF9,  K_F9 },
	{ UIKeyboardHIDUsageKeyboardF10, K_F10 },
	{ UIKeyboardHIDUsageKeyboardF11, K_F11 },
	{ UIKeyboardHIDUsageKeyboardF12, K_F12 },
	{ UIKeyboardHIDUsageKeyboardF13, K_F13 },
	{ UIKeyboardHIDUsageKeyboardF14, K_F14 },
	{ UIKeyboardHIDUsageKeyboardF15, K_F15 },
	{ UIKeyboardHIDUsageKeyboardF16, K_F16 },
	{ UIKeyboardHIDUsageKeyboardF17, K_F17 },
	{ UIKeyboardHIDUsageKeyboardF18, K_F18 },
	{ UIKeyboardHIDUsageKeyboardF19, K_F19 },
	{ UIKeyboardHIDUsageKeyboardF20, K_F20 },

	/* Numeric keypad */
	{ UIKeyboardHIDUsageKeypad0,        K_0 },
	{ UIKeyboardHIDUsageKeypad1,        K_1 },
	{ UIKeyboardHIDUsageKeypad2,        K_2 },
	{ UIKeyboardHIDUsageKeypad3,        K_3 },
	{ UIKeyboardHIDUsageKeypad4,        K_4 },
	{ UIKeyboardHIDUsageKeypad5,        K_5 },
	{ UIKeyboardHIDUsageKeypad6,        K_6 },
	{ UIKeyboardHIDUsageKeypad7,        K_7 },
	{ UIKeyboardHIDUsageKeypad8,        K_8 },
	{ UIKeyboardHIDUsageKeypad9,        K_9 },
	{ UIKeyboardHIDUsageKeypadAsterisk, K_asterisk },
	{ UIKeyboardHIDUsageKeypadPlus,     K_plus },
	{ UIKeyboardHIDUsageKeypadHyphen,   K_minus },
	{ UIKeyboardHIDUsageKeypadPeriod,   K_period },
	{ UIKeyboardHIDUsageKeypadSlash,    K_slash },
	{ UIKeyboardHIDUsageKeypadEqualSign,K_equal },
	{ UIKeyboardHIDUsageKeypadEnter,    K_CR },
	{ UIKeyboardHIDUsageKeypadNumLock,  K_NUM },

	/* modifiers: IUP exposes both L and R */
	{ UIKeyboardHIDUsageKeyboardLeftShift,    K_LSHIFT },
	{ UIKeyboardHIDUsageKeyboardRightShift,   K_RSHIFT },
	{ UIKeyboardHIDUsageKeyboardLeftControl,  K_LCTRL },
	{ UIKeyboardHIDUsageKeyboardRightControl, K_RCTRL },
	{ UIKeyboardHIDUsageKeyboardLeftAlt,      K_LALT },
	{ UIKeyboardHIDUsageKeyboardRightAlt,     K_RALT },
	{ UIKeyboardHIDUsageKeyboardCapsLock,     K_CAPS },
};



static int cocoaTouchKeyLookup(UIKeyboardHIDUsage hid)
{
	for (size_t i = 0; i < sizeof(s_keyMap)/sizeof(s_keyMap[0]); i++)
	{
		if (s_keyMap[i].hid == hid)
		{
			return s_keyMap[i].iup_code;
		}
	}
	return 0;
}

static int cocoaTouchKeyApplyModifiers(int iup_key, UIKeyModifierFlags flags)
{
	/* Command key -> IUP "sys" modifier */
	int has_shift = (flags & UIKeyModifierShift)      != 0;
	int has_ctrl  = (flags & UIKeyModifierControl)    != 0;
	int has_alt   = (flags & UIKeyModifierAlternate)  != 0;
	int has_sys   = (flags & UIKeyModifierCommand)    != 0;
	int has_caps  = (flags & UIKeyModifierAlphaShift) != 0;

	int is_letter = (iup_key >= K_a && iup_key <= K_z);
	if (has_caps && is_letter)
	{
		has_shift = !has_shift;
	}

	if (has_ctrl || has_alt || has_sys)
	{
		if (iup_key >= K_a && iup_key <= K_z)
		{
			iup_key = iup_toupper(iup_key);
		}
		else if (iup_key == K_ccedilla)
		{
			iup_key = K_Ccedilla;
		}
	}

	if (has_shift &&
	    ((iup_key < K_exclam || iup_key > K_tilde) ||
	     (has_ctrl || has_alt || has_sys)))
	{
		iup_key = iup_XkeyShift(iup_key);
	}
	if (has_ctrl) iup_key = iup_XkeyCtrl(iup_key);
	if (has_alt)  iup_key = iup_XkeyAlt(iup_key);
	if (has_sys)  iup_key = iup_XkeySys(iup_key);

	return iup_key;
}

static int cocoaTouchKeyDecode(UIKey* key)
{
	if (!key) return 0;

	UIKeyboardHIDUsage hid = [key keyCode];
	UIKeyModifierFlags flags = [key modifierFlags];

	int base = cocoaTouchKeyLookup(hid);
	if (base == 0)
	{
		return 0;
	}

	int has_ctrl = (flags & UIKeyModifierControl)   != 0;
	int has_alt  = (flags & UIKeyModifierAlternate) != 0;
	int has_sys  = (flags & UIKeyModifierCommand)   != 0;

	/* unmodified printable: use typed char (honors layout + Shift + CapsLock) */
	if ((base >= K_exclam && base <= K_tilde) && !(has_ctrl || has_alt || has_sys))
	{
		NSString* chars = [key characters];
		if ([chars length] > 0)
		{
			unichar ch = [chars characterAtIndex:0];
			if (ch >= K_SP && ch <= K_tilde)
			{
				return (int)ch;
			}
		}
	}

	return cocoaTouchKeyApplyModifiers(base, flags);
}

IUP_DRV_API bool iupCocoaTouchKeyEvent(Ihandle* ih, UIPress* press, bool is_pressed)
{
	if (!ih || !press || !ih->iclass->is_interactive)
	{
		return false;
	}
	UIKey* key = [press key];
	if (!key)
	{
		return false;
	}
	int code = cocoaTouchKeyDecode(key);
	if (code == 0)
	{
		return false;
	}

	if (is_pressed)
	{
		int result = iupKeyCallKeyCb(ih, code);
		if (result == IUP_CLOSE)
		{
			IupExitLoop();
			return true;
		}
		if (result == IUP_IGNORE)
		{
			return true;
		}

		if (!iupObjectCheck(ih))
		{
			return false;
		}

		if (ih->iclass->nativetype == IUP_TYPECANVAS)
		{
			result = iupKeyCallKeyPressCb(ih, code, 1);
			if (result == IUP_CLOSE)
			{
				IupExitLoop();
				return true;
			}
			if (result == IUP_IGNORE)
			{
				return true;
			}
		}

		if ([key modifierFlags] & UIKeyModifierAlternate)
		{
			int base_code = iup_XkeyBase(code);
			if (base_code < 128 && iupKeyProcessMnemonic(ih, base_code))
			{
				return true;
			}
		}

		int has_shift = ([key modifierFlags] & UIKeyModifierShift) ? 1 : 0;
		if (iupKeyProcessNavigation(ih, code, has_shift))
		{
			return true;
		}

		if (code == K_F1)
		{
			Icallback cb = IupGetCallback(ih, "HELP_CB");
			if (cb && cb(ih) == IUP_CLOSE)
			{
				IupExitLoop();
			}
		}

		return false;
	}

	/* release: canvas-only KEYPRESS_CB(press=0) */
	if (ih->iclass->nativetype == IUP_TYPECANVAS)
	{
		int result = iupKeyCallKeyPressCb(ih, code, 0);
		if (result == IUP_CLOSE)
		{
			IupExitLoop();
			return true;
		}
		if (result == IUP_IGNORE)
		{
			return true;
		}
	}
	return false;
}



/* SDK stub; iOS has no synthesized-key API so iupdrvSendKey is a no-op. */
IUP_SDK_API void iupdrvKeyEncode(int code, unsigned int* keyval, unsigned int* state)
{
	if (keyval) *keyval = (unsigned int)iup_XkeyBase(code);
	if (state)
	{
		unsigned int s = 0;
		if (code & iup_XkeyShift(0))  s |= UIKeyModifierShift;
		if (code & iup_XkeyCtrl(0))   s |= UIKeyModifierControl;
		if (code & iup_XkeyAlt(0))    s |= UIKeyModifierAlternate;
		if (code & iup_XkeySys(0))    s |= UIKeyModifierCommand;
		*state = s;
	}
}

IUP_DRV_API void iupCocoaTouchButtonKeySetStatus(UIEvent* event, UIKeyModifierFlags modifier_flags, int pressed_button, int doubleclick, char* out_status)
{
	(void)event;
	if (!out_status) return;
	memcpy(out_status, IUPKEY_STATUS_INIT, IUPKEY_STATUS_SIZE);

	if (modifier_flags & UIKeyModifierShift)      iupKEY_SETSHIFT(out_status);
	if (modifier_flags & UIKeyModifierControl)    iupKEY_SETCONTROL(out_status);
	if (modifier_flags & UIKeyModifierAlternate)  iupKEY_SETALT(out_status);
	if (modifier_flags & UIKeyModifierCommand)    iupKEY_SETSYS(out_status);

	if (pressed_button >= 1 && pressed_button <= 5)
	{
		switch (pressed_button)
		{
			case 1: iupKEY_SETBUTTON1(out_status); break;
			case 2: iupKEY_SETBUTTON2(out_status); break;
			case 3: iupKEY_SETBUTTON3(out_status); break;
			case 4: iupKEY_SETBUTTON4(out_status); break;
			case 5: iupKEY_SETBUTTON5(out_status); break;
		}
	}

	if (doubleclick)
	{
		iupKEY_SETDOUBLE(out_status);
	}
}
