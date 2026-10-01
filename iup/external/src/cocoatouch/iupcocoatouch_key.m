/** \file
 * \brief iOS UIKit Keyboard mapping
 *
 * See Copyright Notice in "iup.h"
 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <stdbool.h>

#include "iup.h"
#include "iupkey.h"
#include "iup_attrib.h"

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

	{ UIKeyboardHIDUsageKeypad0,        K_KP_0 },
	{ UIKeyboardHIDUsageKeypad1,        K_KP_1 },
	{ UIKeyboardHIDUsageKeypad2,        K_KP_2 },
	{ UIKeyboardHIDUsageKeypad3,        K_KP_3 },
	{ UIKeyboardHIDUsageKeypad4,        K_KP_4 },
	{ UIKeyboardHIDUsageKeypad5,        K_KP_5 },
	{ UIKeyboardHIDUsageKeypad6,        K_KP_6 },
	{ UIKeyboardHIDUsageKeypad7,        K_KP_7 },
	{ UIKeyboardHIDUsageKeypad8,        K_KP_8 },
	{ UIKeyboardHIDUsageKeypad9,        K_KP_9 },
	{ UIKeyboardHIDUsageKeypadAsterisk, K_KP_MULT },
	{ UIKeyboardHIDUsageKeypadPlus,     K_KP_PLUS },
	{ UIKeyboardHIDUsageKeypadHyphen,   K_KP_MINUS },
	{ UIKeyboardHIDUsageKeypadPeriod,   K_KP_DECIMAL },
	{ UIKeyboardHIDUsageKeypadSlash,    K_KP_DIV },
	{ UIKeyboardHIDUsageKeypadEqualSign,K_KP_EQUAL },
	{ UIKeyboardHIDUsageKeypadEnter,    K_KP_CR },
	{ UIKeyboardHIDUsageKeypadNumLock,  K_NUM },

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

	if ((base >= K_LSHIFT && base <= K_RCTRL) || base == K_LALT || base == K_RALT || base == K_CAPS)
	{
		return base;
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

static const void* IUPCOCOATOUCH_KEY_PHASE = &IUPCOCOATOUCH_KEY_PHASE;

static UIResponder* s_first_responder = nil;

@interface UIResponder (IupCocoaTouchKey)
- (void)iupCocoaTouchCaptureFirstResponder:(id)sender;
@end

@implementation UIResponder (IupCocoaTouchKey)
- (void)iupCocoaTouchCaptureFirstResponder:(id)sender
{
	(void)sender;
	s_first_responder = self;
}
@end

IUP_DRV_API UIResponder* iupCocoaTouchKeyFirstResponder(void)
{
	s_first_responder = nil;
	[[UIApplication sharedApplication] sendAction:@selector(iupCocoaTouchCaptureFirstResponder:) to:nil from:nil forEvent:nil];
	return s_first_responder;
}

/* a press no view claims reaches no one, so the topmost dialog or popover takes it */
IUP_DRV_API void iupCocoaTouchKeyUpdateResponder(void)
{
	UIViewController* top = iupCocoaTouchFindTopPresentedViewController();
	if ([top isKindOfClass:[UINavigationController class]])
		top = [(UINavigationController*)top topViewController];
	if (!top || ![top respondsToSelector:@selector(ihandle)] || ![top canBecomeFirstResponder])
		return;

	UIResponder* responder = iupCocoaTouchKeyFirstResponder();
	if (responder == top)
		return;
	if ([responder isKindOfClass:[UIView class]] && [(UIView*)responder isDescendantOfView:top.view])
		return;

	[top becomeFirstResponder];
}

IUP_DRV_API bool iupCocoaTouchKeyPresses(Ihandle* ih, NSSet<UIPress*>* presses, bool is_pressed)
{
	NSString* phase = is_pressed ? @"down" : @"up";
	bool handled = false;
	for (UIPress* press in presses)
	{
		if ([objc_getAssociatedObject(press, IUPCOCOATOUCH_KEY_PHASE) isEqualToString:phase])
			continue;
		if (iupCocoaTouchKeyEvent(ih, press, is_pressed))
			handled = true;
	}
	return handled;
}

IUP_DRV_API bool iupCocoaTouchKeyEvent(Ihandle* ih, UIPress* press, bool is_pressed)
{
	if (!ih || !press)
	{
		return false;
	}
	objc_setAssociatedObject(press, IUPCOCOATOUCH_KEY_PHASE, is_pressed ? @"down" : @"up", OBJC_ASSOCIATION_RETAIN_NONATOMIC);
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

	iupAttribSet(ih, "_IUPCOCOATOUCH_KEYPAD", iup_isKeyPadXkey(code)? "1": NULL);
	if (is_pressed)
		iupAttribSetInt(ih, "_IUPCOCOATOUCH_KEYPRESS", code);
	else
		iupAttribSet(ih, "_IUPCOCOATOUCH_KEYPRESS", NULL);

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



IUP_DRV_API NSArray<UIKeyCommand*>* iupCocoaTouchKeyCommands(void)
{
	static NSArray<UIKeyCommand*>* commands = nil;
	if (!commands)
	{
		UIKeyCommand* tab = [UIKeyCommand keyCommandWithInput:@"\t" modifierFlags:0 action:@selector(iupCocoaTouchKeyCommand:)];
		UIKeyCommand* back_tab = [UIKeyCommand keyCommandWithInput:@"\t" modifierFlags:UIKeyModifierShift action:@selector(iupCocoaTouchKeyCommand:)];
		UIKeyCommand* esc = [UIKeyCommand keyCommandWithInput:UIKeyInputEscape modifierFlags:0 action:@selector(iupCocoaTouchKeyCommand:)];
		tab.wantsPriorityOverSystemBehavior = YES;
		back_tab.wantsPriorityOverSystemBehavior = YES;
		esc.wantsPriorityOverSystemBehavior = YES;
		commands = [[NSArray alloc] initWithObjects:tab, back_tab, esc, nil];
	}
	return commands;
}

/* an IME composition keeps Esc and Tab */
IUP_DRV_API bool iupCocoaTouchKeyCommandAllowed(UIResponder* responder)
{
	if ([responder conformsToProtocol:@protocol(UITextInput)] && [(id<UITextInput>)responder markedTextRange])
		return false;
	return true;
}

IUP_DRV_API void iupCocoaTouchKeyCommandEvent(Ihandle* ih, UIKeyCommand* command, UIResponder* responder)
{
	if (!ih)
	{
		return;
	}

	int shift = ([command modifierFlags] & UIKeyModifierShift) ? 1 : 0;
	int code = [[command input] isEqualToString:UIKeyInputEscape] ? K_ESC : (shift ? K_sTAB : K_TAB);

	int result = iupKeyCallKeyCb(ih, code);
	if (result == IUP_CLOSE)
	{
		IupExitLoop();
		return;
	}
	if (result == IUP_IGNORE || !iupObjectCheck(ih))
	{
		return;
	}

	if (ih->iclass->nativetype == IUP_TYPECANVAS)
	{
		result = iupKeyCallKeyPressCb(ih, code, 1);
		if (result == IUP_CLOSE)
		{
			IupExitLoop();
			return;
		}
		if (!iupObjectCheck(ih))
		{
			return;
		}
		if (iupKeyCallKeyPressCb(ih, code, 0) == IUP_CLOSE)
		{
			IupExitLoop();
			return;
		}
		if (result == IUP_IGNORE || !iupObjectCheck(ih))
		{
			return;
		}
	}

	if (iupKeyProcessNavigation(ih, code, shift))
	{
		return;
	}

	if (code == K_TAB && iupAttribGetInt(ih, "_IUP_MULTILINE_TEXT") && [responder conformsToProtocol:@protocol(UIKeyInput)])
	{
		[(id<UIKeyInput>)responder insertText:@"\t"];
	}
}

IUP_DRV_API int iupCocoaTouchKeyTextCode(NSString* text)
{
	if ([text length] != 1)
		return 0;

	unichar ch = [text characterAtIndex:0];
	if ((ch >= K_SP && ch <= K_tilde) || (ch >= 0xA0 && ch <= 0xFF))
		return (int)ch;
	return 0;
}

/* true when K_ANY or KEYPRESS_CB consumed the key; a press that already reported it is skipped */
IUP_DRV_API bool iupCocoaTouchKeyText(Ihandle* ih, UIResponder* responder, int code)
{
	if (!ih || code == 0)
		return false;
	if (iupAttribGetInt(ih, "_IUPCOCOATOUCH_KEYPRESS") == code)
	{
		iupAttribSet(ih, "_IUPCOCOATOUCH_KEYPRESS", NULL);
		return false;
	}
	if ([responder conformsToProtocol:@protocol(UITextInput)] && [(id<UITextInput>)responder markedTextRange])
		return false;

	int result = iupKeyCallKeyCb(ih, code);
	if (result == IUP_CLOSE)
	{
		IupExitLoop();
		return true;
	}
	if (result == IUP_IGNORE || !iupObjectCheck(ih))
		return true;

	if (ih->iclass->nativetype == IUP_TYPECANVAS)
	{
		result = iupKeyCallKeyPressCb(ih, code, 1);
		if (result == IUP_CLOSE)
		{
			IupExitLoop();
			return true;
		}
		if (!iupObjectCheck(ih))
			return true;
		if (iupKeyCallKeyPressCb(ih, code, 0) == IUP_CLOSE)
		{
			IupExitLoop();
			return true;
		}
		if (result == IUP_IGNORE || !iupObjectCheck(ih))
			return true;
	}
	return false;
}

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
