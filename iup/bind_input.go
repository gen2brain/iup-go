package iup

// From 32 to 126, all character sets are equal,
// the key code is the same as the ASCII character code.
const (
	K_SP           = int(' ')  /* 32 (0x20) */
	K_exclam       = int('!')  /* 33 */
	K_quotedbl     = int('"')  /* 34 */
	K_numbersign   = int('#')  /* 35 */
	K_dollar       = int('$')  /* 36 */
	K_percent      = int('%')  /* 37 */
	K_ampersand    = int('&')  /* 38 */
	K_apostrophe   = int('\'') /* 39 */
	K_parentleft   = int('(')  /* 40 */
	K_parentright  = int(')')  /* 41 */
	K_asterisk     = int('*')  /* 42 */
	K_plus         = int('+')  /* 43 */
	K_comma        = int(',')  /* 44 */
	K_minus        = int('-')  /* 45 */
	K_period       = int('.')  /* 46 */
	K_slash        = int('/')  /* 47 */
	K_0            = int('0')  /* 48 (0x30) */
	K_1            = int('1')  /* 49 */
	K_2            = int('2')  /* 50 */
	K_3            = int('3')  /* 51 */
	K_4            = int('4')  /* 52 */
	K_5            = int('5')  /* 53 */
	K_6            = int('6')  /* 54 */
	K_7            = int('7')  /* 55 */
	K_8            = int('8')  /* 56 */
	K_9            = int('9')  /* 57 */
	K_colon        = int(':')  /* 58 */
	K_semicolon    = int(';')  /* 59 */
	K_less         = int('<')  /* 60 */
	K_equal        = int('=')  /* 61 */
	K_greater      = int('>')  /* 62 */
	K_question     = int('?')  /* 63 */
	K_at           = int('@')  /* 64 */
	K_A            = int('A')  /* 65 (0x41) */
	K_B            = int('B')  /* 66 */
	K_C            = int('C')  /* 67 */
	K_D            = int('D')  /* 68 */
	K_E            = int('E')  /* 69 */
	K_F            = int('F')  /* 70 */
	K_G            = int('G')  /* 71 */
	K_H            = int('H')  /* 72 */
	K_I            = int('I')  /* 73 */
	K_J            = int('J')  /* 74 */
	K_K            = int('K')  /* 75 */
	K_L            = int('L')  /* 76 */
	K_M            = int('M')  /* 77 */
	K_N            = int('N')  /* 78 */
	K_O            = int('O')  /* 79 */
	K_P            = int('P')  /* 80 */
	K_Q            = int('Q')  /* 81 */
	K_R            = int('R')  /* 82 */
	K_S            = int('S')  /* 83 */
	K_T            = int('T')  /* 84 */
	K_U            = int('U')  /* 85 */
	K_V            = int('V')  /* 86 */
	K_W            = int('W')  /* 87 */
	K_X            = int('X')  /* 88 */
	K_Y            = int('Y')  /* 89 */
	K_Z            = int('Z')  /* 90 */
	K_bracketleft  = int('[')  /* 91 */
	K_backslash    = int('\\') /* 92 */
	K_bracketright = int(']')  /* 93 */
	K_circum       = int('^')  /* 94 */
	K_underscore   = int('_')  /* 95 */
	K_grave        = int('`')  /* 96 */
	K_a            = int('a')  /* 97 (0x61) */
	K_b            = int('b')  /* 98 */
	K_c            = int('c')  /* 99 */
	K_d            = int('d')  /* 100 */
	K_e            = int('e')  /* 101 */
	K_f            = int('f')  /* 102 */
	K_g            = int('g')  /* 103 */
	K_h            = int('h')  /* 104 */
	K_i            = int('i')  /* 105 */
	K_j            = int('j')  /* 106 */
	K_k            = int('k')  /* 107 */
	K_l            = int('l')  /* 108 */
	K_m            = int('m')  /* 109 */
	K_n            = int('n')  /* 110 */
	K_o            = int('o')  /* 111 */
	K_p            = int('p')  /* 112 */
	K_q            = int('q')  /* 113 */
	K_r            = int('r')  /* 114 */
	K_s            = int('s')  /* 115 */
	K_t            = int('t')  /* 116 */
	K_u            = int('u')  /* 117 */
	K_v            = int('v')  /* 118 */
	K_w            = int('w')  /* 119 */
	K_x            = int('x')  /* 120 */
	K_y            = int('y')  /* 121 */
	K_z            = int('z')  /* 122 */
	K_braceleft    = int('{')  /* 123 */
	K_bar          = int('|')  /* 124 */
	K_braceright   = int('}')  /* 125 */
	K_tilde        = int('~')  /* 126 (0x7E) */
)

// Also define the escape sequences that have keys associated
const (
	K_BS  = int('\b') /* 8 */
	K_TAB = int('\t') /* 9 */
	K_LF  = int('\n') /* 10 (0x0A) not a real key, is a combination of CR with a modifier, just to document */
	K_CR  = int('\r') /* 13 (0x0D) */
)

// Backward compatible definitions
const (
	K_quoteleft  = K_grave
	K_quoteright = K_apostrophe
)

// These use the same definition as X11 and GDK.
// This also means that any X11 or GDK definition can also be used.
const (
	K_PAUSE  = 0xFF13
	K_ESC    = 0xFF1B
	K_HOME   = 0xFF50
	K_LEFT   = 0xFF51
	K_UP     = 0xFF52
	K_RIGHT  = 0xFF53
	K_DOWN   = 0xFF54
	K_PGUP   = 0xFF55
	K_PGDN   = 0xFF56
	K_END    = 0xFF57
	K_MIDDLE = 0xFF0B
	K_Print  = 0xFF61
	K_INS    = 0xFF63
	K_Menu   = 0xFF67
	K_DEL    = 0xFFFF
	K_F1     = 0xFFBE
	K_F2     = 0xFFBF
	K_F3     = 0xFFC0
	K_F4     = 0xFFC1
	K_F5     = 0xFFC2
	K_F6     = 0xFFC3
	K_F7     = 0xFFC4
	K_F8     = 0xFFC5
	K_F9     = 0xFFC6
	K_F10    = 0xFFC7
	K_F11    = 0xFFC8
	K_F12    = 0xFFC9
)

// No Shift/Ctrl/Alt
const (
	K_LSHIFT = 0xFFE1
	K_RSHIFT = 0xFFE2
	K_LCTRL  = 0xFFE3
	K_RCTRL  = 0xFFE4
	K_LALT   = 0xFFE9
	K_RALT   = 0xFFEA

	K_NUM    = 0xFF7F
	K_SCROLL = 0xFF14
	K_CAPS   = 0xFFE5
)

// Also, these are the same as the Latin-1 definition
const (
	K_ccedilla  = 0x00E7
	K_Ccedilla  = 0x00C7
	K_acute     = 0x00B4 /* no Shift/Ctrl/Alt */
	K_diaeresis = 0x00A8
)

// IsPrint informs if a key can be directly used as a printable character.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func IsPrint(c int) bool {
	return c > 31 && c < 127
}

// IsXKey informs if a given key is an extended code.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func IsXKey(c int) bool {
	return (c >= 128)
}

// IsShiftXKey informs if a given key is an extended code using the Shift modifier.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func IsShiftXKey(c int) bool {
	return (uint(c) & 0x10000000) != 0
}

// IsCtrlXKey informs if a given key is an extended code using the Ctrl modifier.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func IsCtrlXKey(c int) bool {
	return (uint(c) & 0x20000000) != 0
}

// IsAltXKey informs if a given key is an extended code using the Alt modifier.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func IsAltXKey(c int) bool {
	return (uint(c) & 0x40000000) != 0
}

// IsSysXKey informs if a given key is an extended code using the Sys modifier.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func IsSysXKey(c int) bool {
	return (uint(c) & 0x80000000) != 0
}

// XKeyBase obtains a key code for a generic combination.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func XKeyBase(c int) int {
	return int(uint(c) & 0x0FFFFFFF)
}

// XKeyShift obtains a key code for a generic combination.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func XKeyShift(c int) int {
	return int(uint(c) | 0x10000000)
}

// XKeyCtrl obtains a key code for a generic combination.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func XKeyCtrl(c int) int {
	return int(uint(c) | 0x20000000)
}

// XKeyAlt obtains a key code for a generic combination.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func XKeyAlt(c int) int {
	return int(uint(c) | 0x40000000)
}

// XKeySys obtains a key code for a generic combination.
//
// https://www.tecgraf.puc-rio.br/iup/en/attrib/key.html
func XKeySys(c int) int {
	return int(uint(c) | 0x80000000)
}

// IsShift mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsShift(s string) bool {
	return s[0] == 'S'
}

// IsControl mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsControl(s string) bool {
	return s[1] == 'C'
}

// IsButton1 mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsButton1(s string) bool {
	return s[2] == '1'
}

// IsButton2 mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsButton2(s string) bool {
	return s[3] == '2'
}

// IsButton3 mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsButton3(s string) bool {
	return s[4] == '3'
}

// IsDouble mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsDouble(s string) bool {
	return s[5] == 'D'
}

// IsAlt mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsAlt(s string) bool {
	return s[6] == 'A'
}

// IsSys mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsSys(s string) bool {
	return s[7] == 'Y'
}

// IsButton4 mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsButton4(s string) bool {
	return s[8] == '4'
}

// IsButton5 mouse button macro.
//
// https://www.tecgraf.puc-rio.br/iup/en/call/iup_button_cb.html
func IsButton5(s string) bool {
	return s[9] == '5'
}
