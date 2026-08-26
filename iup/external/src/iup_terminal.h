/** \file
 * \brief Terminal Control internal model.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_TERMINAL_H
#define __IUP_TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* self-contained model: no IUP dependencies, colors packed 0x00RRGGBB */

#define ITERM_FL_BOLD       0x0001
#define ITERM_FL_DIM        0x0002
#define ITERM_FL_ITALIC     0x0004
#define ITERM_FL_UNDERLINE  0x0008
#define ITERM_FL_INVERSE    0x0010
#define ITERM_FL_STRIKE     0x0020
#define ITERM_FL_HIDDEN     0x0040
#define ITERM_FL_WIDE       0x0080
#define ITERM_FL_WIDECONT   0x0100
#define ITERM_FL_FG_DEFAULT 0x0200
#define ITERM_FL_BG_DEFAULT 0x0400

#define ITERM_FL_STYLEMASK (ITERM_FL_BOLD|ITERM_FL_DIM|ITERM_FL_ITALIC|ITERM_FL_UNDERLINE|\
                            ITERM_FL_INVERSE|ITERM_FL_STRIKE|ITERM_FL_HIDDEN|\
                            ITERM_FL_FG_DEFAULT|ITERM_FL_BG_DEFAULT)

#define ITERM_MODE_CKM       0x0001
#define ITERM_MODE_ORIGIN    0x0002
#define ITERM_MODE_AUTOWRAP  0x0004
#define ITERM_MODE_CURSORVIS 0x0008
#define ITERM_MODE_CURSORBLK 0x0010
#define ITERM_MODE_ALTSCREEN 0x0020
#define ITERM_MODE_MOUSE_CLK 0x0040
#define ITERM_MODE_MOUSE_DRAG 0x0080
#define ITERM_MODE_MOUSE_SGR 0x0100
#define ITERM_MODE_BRACKPASTE 0x0200

#define ITERM_CURSOR_BLOCK     0
#define ITERM_CURSOR_UNDERLINE 1
#define ITERM_CURSOR_BAR       2

#define ITERM_CSI_MAXARGS 32
#define ITERM_OSC_MAX 4096
#define ITERM_DEF_SCROLLBACK 5000

typedef struct _ItermCell
{
  unsigned int cp;
  unsigned int fg, bg;
  unsigned short flags;
} ItermCell;

typedef struct _ItermLine
{
  ItermCell* cells;
  short ncells;
  unsigned char wrapped;
} ItermLine;

typedef struct _ItermPen
{
  unsigned int fg, bg;
  unsigned short flags;
} ItermPen;

typedef struct _ItermCallbacks
{
  void (*response)(void* user, const char* bytes, int len);
  void (*bell)(void* user);
  void (*title)(void* user, const char* title);
  void (*palette)(void* user, int index, unsigned int rgb);
  void (*clipboard)(void* user, const char* text);
} ItermCallbacks;

typedef struct _Iterm
{
  int cols, rows;
  ItermLine* lines;
  ItermLine* alt_lines;

  ItermLine* sb;
  int sb_max, sb_count, sb_head;
  int sb_disabled;
  int sb_pushed;

  int cx, cy;
  int pending_wrap;
  int scroll_top, scroll_bot;

  ItermPen pen;
  int saved_cx, saved_cy;
  ItermPen saved_pen;
  int saved_charset;
  int alt_saved_cx, alt_saved_cy;
  ItermPen alt_saved_pen;

  unsigned int modes;
  int cursor_style;
  unsigned int last_cp;

  unsigned int palette[16];
  unsigned int def_fg, def_bg;

  unsigned char* tabs;
  int charset_gl;
  int charset[2];

  unsigned char* dirty;
  int all_dirty;

  /* parser */
  int pstate;
  unsigned int utf8_cp;
  int utf8_more;
  int csi_args[ITERM_CSI_MAXARGS];
  unsigned char csi_colon[ITERM_CSI_MAXARGS];
  int csi_argc;
  int csi_has_arg;
  char csi_leader;
  char csi_intermed;
  int osc_cmd;
  char osc_buf[ITERM_OSC_MAX];
  int osc_len;
  int osc_esc;

  ItermCallbacks cb;
  void* cb_user;
} Iterm;

/* iup_termscreen.c */
int  iupTermScreenInit(Iterm* t, int cols, int rows, int sb_max);
void iupTermScreenRelease(Iterm* t);
void iupTermScreenResize(Iterm* t, int cols, int rows);
void iupTermScreenReset(Iterm* t);
ItermLine* iupTermScreenLine(Iterm* t, int row);
ItermLine* iupTermScreenHistoryLine(Iterm* t, int index);
void iupTermScreenPutChar(Iterm* t, unsigned int cp);
void iupTermScreenLineFeed(Iterm* t);
void iupTermScreenReverseLineFeed(Iterm* t);
void iupTermScreenCarriageReturn(Iterm* t);
void iupTermScreenBackspace(Iterm* t);
void iupTermScreenTab(Iterm* t, int count, int backward);
void iupTermScreenSetTab(Iterm* t);
void iupTermScreenClearTab(Iterm* t, int all);
void iupTermScreenMoveCursor(Iterm* t, int dx, int dy);
void iupTermScreenSetCursor(Iterm* t, int col, int row);
void iupTermScreenSetCursorCol(Iterm* t, int col);
void iupTermScreenSetCursorRow(Iterm* t, int row);
void iupTermScreenSaveCursor(Iterm* t);
void iupTermScreenRestoreCursor(Iterm* t);
void iupTermScreenEraseDisplay(Iterm* t, int mode);
void iupTermScreenEraseLine(Iterm* t, int mode);
void iupTermScreenEraseChars(Iterm* t, int count);
void iupTermScreenInsertChars(Iterm* t, int count);
void iupTermScreenDeleteChars(Iterm* t, int count);
void iupTermScreenInsertLines(Iterm* t, int count);
void iupTermScreenDeleteLines(Iterm* t, int count);
void iupTermScreenScroll(Iterm* t, int count);
void iupTermScreenSetScrollRegion(Iterm* t, int top, int bot);
void iupTermScreenSetAltScreen(Iterm* t, int alt, int save_cursor, int clear);
void iupTermScreenClearScrollback(Iterm* t);
void iupTermScreenResetPalette(Iterm* t, int index);
void iupTermScreenDamageAll(Iterm* t);
int  iupTermCharWidth(unsigned int cp);

/* iup_termparse.c */
void iupTermParseInit(Iterm* t);
void iupTermParseBytes(Iterm* t, const char* buf, int len);

/* iup_termpty.c */
typedef struct _ItermPty ItermPty;

int  iupTermPtyAvailable(void);
ItermPty* iupTermPtyStart(const char* cmd, const char* term_name, int cols, int rows, char* error, int error_size);
void iupTermPtyClose(ItermPty* pty);
int  iupTermPtyRead(ItermPty* pty, char* buf, int max);
int  iupTermPtyWrite(ItermPty* pty, const char* buf, int len);
void iupTermPtyResize(ItermPty* pty, int cols, int rows);
int  iupTermPtyCheckExit(ItermPty* pty, int* status);
long iupTermPtyGetPid(ItermPty* pty);
void iupTermPtyKill(ItermPty* pty, int force);

#ifdef __cplusplus
}
#endif

#endif
