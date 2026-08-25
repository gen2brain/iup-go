/** \file
 * \brief Terminal Control escape sequence parser.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup_terminal.h"

enum {
  IPSTATE_GROUND,
  IPSTATE_ESC,
  IPSTATE_ESC2,
  IPSTATE_CSI,
  IPSTATE_OSC,
  IPSTATE_STR
};

/* DEC Special Graphics, 0x5F..0x7E */
static const unsigned short iterm_decgraph[] = {
  0x00A0, 0x25C6, 0x2592, 0x2409, 0x240C, 0x240D, 0x240A, 0x00B0,
  0x00B1, 0x2424, 0x240B, 0x2518, 0x2510, 0x250C, 0x2514, 0x253C,
  0x23BA, 0x23BB, 0x2500, 0x23BC, 0x23BD, 0x251C, 0x2524, 0x2534,
  0x252C, 0x2502, 0x2264, 0x2265, 0x03C0, 0x2260, 0x00A3, 0x00B7
};

void iupTermParseInit(Iterm* t)
{
  t->pstate = IPSTATE_GROUND;
  t->utf8_more = 0;
  t->osc_len = 0;
  t->osc_esc = 0;
  t->last_cp = 0;
}

static void itermRespond(Iterm* t, const char* str)
{
  if (t->cb.response)
    t->cb.response(t->cb_user, str, (int)strlen(str));
}

static int itermCsiArg(Iterm* t, int i, int def)
{
  if (i >= t->csi_argc || t->csi_args[i] < 0)
    return def;
  return t->csi_args[i];
}

static void itermPutChar(Iterm* t, unsigned int cp)
{
  if (t->charset[t->charset_gl] && cp >= 0x5F && cp <= 0x7E)
    cp = iterm_decgraph[cp - 0x5F];
  iupTermScreenPutChar(t, cp);
  t->last_cp = cp;
}

static void itermSetMode(Iterm* t, unsigned int mode, int set)
{
  if (set)
    t->modes |= mode;
  else
    t->modes &= ~mode;
}

static void itermDecMode(Iterm* t, int mode, int set)
{
  switch (mode)
  {
  case 1: itermSetMode(t, ITERM_MODE_CKM, set); break;
  case 6:
    itermSetMode(t, ITERM_MODE_ORIGIN, set);
    iupTermScreenSetCursor(t, 0, 0);
    break;
  case 7: itermSetMode(t, ITERM_MODE_AUTOWRAP, set); break;
  case 12: itermSetMode(t, ITERM_MODE_CURSORBLK, set); break;
  case 25:
    itermSetMode(t, ITERM_MODE_CURSORVIS, set);
    iupTermScreenDamageAll(t);
    break;
  case 47:
    iupTermScreenSetAltScreen(t, set, 0, 0);
    break;
  case 1047:
    iupTermScreenSetAltScreen(t, set, 0, set);
    break;
  case 1048:
    if (set) iupTermScreenSaveCursor(t);
    else iupTermScreenRestoreCursor(t);
    break;
  case 1049:
    iupTermScreenSetAltScreen(t, set, 1, set);
    break;
  case 1000: itermSetMode(t, ITERM_MODE_MOUSE_CLK, set); break;
  case 1002:
    itermSetMode(t, ITERM_MODE_MOUSE_CLK, set);
    itermSetMode(t, ITERM_MODE_MOUSE_DRAG, set);
    break;
  case 1006: itermSetMode(t, ITERM_MODE_MOUSE_SGR, set); break;
  case 2004: itermSetMode(t, ITERM_MODE_BRACKPASTE, set); break;
  }
}

static unsigned int iterm256Color(Iterm* t, int n)
{
  static const unsigned char cube[6] = { 0, 95, 135, 175, 215, 255 };
  if (n < 0) n = 0;
  if (n < 16)
    return t->palette[n];
  if (n < 232)
  {
    int v = n - 16;
    return ((unsigned int)cube[v / 36] << 16) |
           ((unsigned int)cube[(v / 6) % 6] << 8) |
           (unsigned int)cube[v % 6];
  }
  if (n < 256)
  {
    unsigned int g = 8 + 10 * (n - 232);
    return (g << 16) | (g << 8) | g;
  }
  return t->def_fg;
}

/* consumes 38/48 extended color args, returns count consumed after the introducer */
static int itermSgrExtColor(Iterm* t, int i, unsigned int* color)
{
  int kind = itermCsiArg(t, i + 1, 0);
  if (kind == 5)
  {
    *color = iterm256Color(t, itermCsiArg(t, i + 2, 0));
    return 2;
  }
  if (kind == 2)
  {
    unsigned int r = (unsigned int)itermCsiArg(t, i + 2, 0) & 0xFF;
    unsigned int g = (unsigned int)itermCsiArg(t, i + 3, 0) & 0xFF;
    unsigned int b = (unsigned int)itermCsiArg(t, i + 4, 0) & 0xFF;
    *color = (r << 16) | (g << 8) | b;
    return 4;
  }
  return 0;
}

static void itermSgr(Iterm* t)
{
  int i;
  if (t->csi_argc == 0)
  {
    t->pen.fg = t->def_fg;
    t->pen.bg = t->def_bg;
    t->pen.flags = ITERM_FL_FG_DEFAULT | ITERM_FL_BG_DEFAULT;
    return;
  }

  for (i = 0; i < t->csi_argc; i++)
  {
    int n = itermCsiArg(t, i, 0);
    switch (n)
    {
    case 0:
      t->pen.fg = t->def_fg;
      t->pen.bg = t->def_bg;
      t->pen.flags = ITERM_FL_FG_DEFAULT | ITERM_FL_BG_DEFAULT;
      break;
    case 1: t->pen.flags |= ITERM_FL_BOLD; break;
    case 2: t->pen.flags |= ITERM_FL_DIM; break;
    case 3: t->pen.flags |= ITERM_FL_ITALIC; break;
    case 4:
      if (t->csi_colon[i] && itermCsiArg(t, i + 1, 1) == 0)
      {
        t->pen.flags &= ~ITERM_FL_UNDERLINE;
        i++;
      }
      else
      {
        t->pen.flags |= ITERM_FL_UNDERLINE;
        if (t->csi_colon[i]) i++;
      }
      break;
    case 7: t->pen.flags |= ITERM_FL_INVERSE; break;
    case 8: t->pen.flags |= ITERM_FL_HIDDEN; break;
    case 9: t->pen.flags |= ITERM_FL_STRIKE; break;
    case 21: t->pen.flags |= ITERM_FL_UNDERLINE; break;
    case 22: t->pen.flags &= ~(ITERM_FL_BOLD | ITERM_FL_DIM); break;
    case 23: t->pen.flags &= ~ITERM_FL_ITALIC; break;
    case 24: t->pen.flags &= ~ITERM_FL_UNDERLINE; break;
    case 27: t->pen.flags &= ~ITERM_FL_INVERSE; break;
    case 28: t->pen.flags &= ~ITERM_FL_HIDDEN; break;
    case 29: t->pen.flags &= ~ITERM_FL_STRIKE; break;
    case 38:
      i += itermSgrExtColor(t, i, &t->pen.fg);
      t->pen.flags &= ~ITERM_FL_FG_DEFAULT;
      break;
    case 39:
      t->pen.fg = t->def_fg;
      t->pen.flags |= ITERM_FL_FG_DEFAULT;
      break;
    case 48:
      i += itermSgrExtColor(t, i, &t->pen.bg);
      t->pen.flags &= ~ITERM_FL_BG_DEFAULT;
      break;
    case 49:
      t->pen.bg = t->def_bg;
      t->pen.flags |= ITERM_FL_BG_DEFAULT;
      break;
    default:
      if (n >= 30 && n <= 37)
      {
        t->pen.fg = t->palette[n - 30];
        t->pen.flags &= ~ITERM_FL_FG_DEFAULT;
      }
      else if (n >= 40 && n <= 47)
      {
        t->pen.bg = t->palette[n - 40];
        t->pen.flags &= ~ITERM_FL_BG_DEFAULT;
      }
      else if (n >= 90 && n <= 97)
      {
        t->pen.fg = t->palette[n - 82];
        t->pen.flags &= ~ITERM_FL_FG_DEFAULT;
      }
      else if (n >= 100 && n <= 107)
      {
        t->pen.bg = t->palette[n - 92];
        t->pen.flags &= ~ITERM_FL_BG_DEFAULT;
      }
      break;
    }
  }
}

static void itermCsiDispatch(Iterm* t, char final)
{
  char buf[64];
  int i, n;

  if (t->csi_leader == '?')
  {
    if (final == 'h' || final == 'l')
    {
      for (i = 0; i < t->csi_argc; i++)
        itermDecMode(t, itermCsiArg(t, i, 0), final == 'h');
    }
    return;
  }
  if (t->csi_leader)
    return;

  switch (final)
  {
  case '@': iupTermScreenInsertChars(t, itermCsiArg(t, 0, 1)); break;
  case 'A': iupTermScreenMoveCursor(t, 0, -itermCsiArg(t, 0, 1)); break;
  case 'B': iupTermScreenMoveCursor(t, 0, itermCsiArg(t, 0, 1)); break;
  case 'C': iupTermScreenMoveCursor(t, itermCsiArg(t, 0, 1), 0); break;
  case 'D': iupTermScreenMoveCursor(t, -itermCsiArg(t, 0, 1), 0); break;
  case 'E':
    iupTermScreenMoveCursor(t, 0, itermCsiArg(t, 0, 1));
    iupTermScreenCarriageReturn(t);
    break;
  case 'F':
    iupTermScreenMoveCursor(t, 0, -itermCsiArg(t, 0, 1));
    iupTermScreenCarriageReturn(t);
    break;
  case 'G':
  case '`':
    iupTermScreenSetCursorCol(t, itermCsiArg(t, 0, 1) - 1);
    break;
  case 'H':
  case 'f':
    iupTermScreenSetCursor(t, itermCsiArg(t, 1, 1) - 1, itermCsiArg(t, 0, 1) - 1);
    break;
  case 'I': iupTermScreenTab(t, itermCsiArg(t, 0, 1), 0); break;
  case 'J':
    n = itermCsiArg(t, 0, 0);
    if (n == 3)
      iupTermScreenClearScrollback(t);
    else
      iupTermScreenEraseDisplay(t, n);
    break;
  case 'K': iupTermScreenEraseLine(t, itermCsiArg(t, 0, 0)); break;
  case 'L': iupTermScreenInsertLines(t, itermCsiArg(t, 0, 1)); break;
  case 'M': iupTermScreenDeleteLines(t, itermCsiArg(t, 0, 1)); break;
  case 'P': iupTermScreenDeleteChars(t, itermCsiArg(t, 0, 1)); break;
  case 'S': iupTermScreenScroll(t, itermCsiArg(t, 0, 1)); break;
  case 'T': iupTermScreenScroll(t, -itermCsiArg(t, 0, 1)); break;
  case 'X': iupTermScreenEraseChars(t, itermCsiArg(t, 0, 1)); break;
  case 'Z': iupTermScreenTab(t, itermCsiArg(t, 0, 1), 1); break;
  case 'a': iupTermScreenMoveCursor(t, itermCsiArg(t, 0, 1), 0); break;
  case 'b':
    if (t->last_cp)
    {
      n = itermCsiArg(t, 0, 1);
      for (i = 0; i < n; i++)
        iupTermScreenPutChar(t, t->last_cp);
    }
    break;
  case 'c':
    itermRespond(t, "\033[?62;22c");
    break;
  case 'd': iupTermScreenSetCursorRow(t, itermCsiArg(t, 0, 1) - 1); break;
  case 'e': iupTermScreenMoveCursor(t, 0, itermCsiArg(t, 0, 1)); break;
  case 'g': iupTermScreenClearTab(t, itermCsiArg(t, 0, 0) == 3); break;
  case 'n':
    n = itermCsiArg(t, 0, 0);
    if (n == 5)
      itermRespond(t, "\033[0n");
    else if (n == 6)
    {
      int row = t->cy + 1;
      if (t->modes & ITERM_MODE_ORIGIN)
        row -= t->scroll_top;
      sprintf(buf, "\033[%d;%dR", row, t->cx + 1);
      itermRespond(t, buf);
    }
    break;
  case 'q':
    if (t->csi_intermed == ' ')
    {
      n = itermCsiArg(t, 0, 0);
      switch (n)
      {
      case 0: case 1: case 2: t->cursor_style = ITERM_CURSOR_BLOCK; break;
      case 3: case 4: t->cursor_style = ITERM_CURSOR_UNDERLINE; break;
      case 5: case 6: t->cursor_style = ITERM_CURSOR_BAR; break;
      }
      itermSetMode(t, ITERM_MODE_CURSORBLK, n == 0 || n == 1 || n == 3 || n == 5);
      iupTermScreenDamageAll(t);
    }
    break;
  case 'm': itermSgr(t); break;
  case 'r':
    iupTermScreenSetScrollRegion(t, itermCsiArg(t, 0, 1) - 1, itermCsiArg(t, 1, t->rows) - 1);
    break;
  case 's': iupTermScreenSaveCursor(t); break;
  case 'u': iupTermScreenRestoreCursor(t); break;
  case 't':
    if (itermCsiArg(t, 0, 0) == 18)
    {
      sprintf(buf, "\033[8;%d;%dt", t->rows, t->cols);
      itermRespond(t, buf);
    }
    break;
  }
}

static unsigned int itermParseColorSpec(const char* spec)
{
  unsigned int r, g, b;
  if (sscanf(spec, "rgb:%x/%x/%x", &r, &g, &b) == 3)
  {
    if (r > 255) r >>= 8;
    if (g > 255) g >>= 8;
    if (b > 255) b >>= 8;
    return (r << 16) | (g << 8) | b;
  }
  if (spec[0] == '#' && sscanf(spec + 1, "%6x", &r) == 1)
    return r;
  return 0xFFFFFFFF;
}

static void itermOscDispatch(Iterm* t)
{
  char buf[64];
  t->osc_buf[t->osc_len] = 0;

  if (t->osc_cmd < 0)
  {
    t->osc_cmd = atoi(t->osc_buf);
    t->osc_len = 0;
    t->osc_buf[0] = 0;
  }

  switch (t->osc_cmd)
  {
  case 0:
  case 2:
    if (t->cb.title)
      t->cb.title(t->cb_user, t->osc_buf);
    break;
  case 4:
    {
      char* sep = strchr(t->osc_buf, ';');
      if (sep)
      {
        int index = atoi(t->osc_buf);
        if (index >= 0 && index < 16)
        {
          if (sep[1] == '?')
          {
            unsigned int c = t->palette[index];
            sprintf(buf, "\033]4;%d;rgb:%02x/%02x/%02x\033\\", index,
                    (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
            itermRespond(t, buf);
          }
          else
          {
            unsigned int c = itermParseColorSpec(sep + 1);
            if (c != 0xFFFFFFFF)
            {
              t->palette[index] = c;
              if (t->cb.palette)
                t->cb.palette(t->cb_user, index, c);
            }
          }
        }
      }
    }
    break;
  case 10:
  case 11:
    {
      unsigned int c = (t->osc_cmd == 10) ? t->def_fg : t->def_bg;
      if (t->osc_buf[0] == '?')
      {
        sprintf(buf, "\033]%d;rgb:%02x%02x/%02x%02x/%02x%02x\033\\", t->osc_cmd,
                (c >> 16) & 0xFF, (c >> 16) & 0xFF,
                (c >> 8) & 0xFF, (c >> 8) & 0xFF,
                c & 0xFF, c & 0xFF);
        itermRespond(t, buf);
      }
      else
      {
        c = itermParseColorSpec(t->osc_buf);
        if (c != 0xFFFFFFFF)
        {
          if (t->osc_cmd == 10) t->def_fg = c;
          else t->def_bg = c;
          iupTermScreenDamageAll(t);
        }
      }
    }
    break;
  case 104:
    iupTermScreenResetPalette(t, t->osc_len ? atoi(t->osc_buf) : -1);
    break;
  }
}

static void itermEscDispatch(Iterm* t, char c)
{
  switch (c)
  {
  case '7': iupTermScreenSaveCursor(t); break;
  case '8': iupTermScreenRestoreCursor(t); break;
  case 'D': iupTermScreenLineFeed(t); break;
  case 'E':
    iupTermScreenCarriageReturn(t);
    iupTermScreenLineFeed(t);
    break;
  case 'H': iupTermScreenSetTab(t); break;
  case 'M': iupTermScreenReverseLineFeed(t); break;
  case 'c':
    iupTermScreenClearScrollback(t);
    iupTermScreenReset(t);
    break;
  }
}

static void itermControl(Iterm* t, unsigned char c)
{
  switch (c)
  {
  case 0x07: if (t->cb.bell) t->cb.bell(t->cb_user); break;
  case 0x08: iupTermScreenBackspace(t); break;
  case 0x09: iupTermScreenTab(t, 1, 0); break;
  case 0x0A:
  case 0x0B:
  case 0x0C: iupTermScreenLineFeed(t); break;
  case 0x0D: iupTermScreenCarriageReturn(t); break;
  case 0x0E: t->charset_gl = 1; break;
  case 0x0F: t->charset_gl = 0; break;
  }
}

static void itermCsiInit(Iterm* t)
{
  t->csi_argc = 0;
  t->csi_has_arg = 0;
  t->csi_leader = 0;
  t->csi_intermed = 0;
  memset(t->csi_colon, 0, sizeof(t->csi_colon));
  t->csi_args[0] = -1;
}

static void itermByte(Iterm* t, unsigned char c)
{
  /* NUL and DEL are ignored in every state */
  if (c == 0x00 || c == 0x7F)
    return;

  switch (t->pstate)
  {
  case IPSTATE_GROUND:
    if (c == 0x1B)
    {
      t->utf8_more = 0;
      t->pstate = IPSTATE_ESC;
    }
    else if (c < 0x20)
    {
      t->utf8_more = 0;
      itermControl(t, c);
    }
    else if (c < 0x80)
    {
      t->utf8_more = 0;
      itermPutChar(t, c);
    }
    else if (c < 0xC0)
    {
      if (t->utf8_more > 0)
      {
        t->utf8_cp = (t->utf8_cp << 6) | (c & 0x3F);
        t->utf8_more--;
        if (t->utf8_more == 0)
          itermPutChar(t, t->utf8_cp < 0x80 ? 0xFFFD : t->utf8_cp);
      }
      else
        itermPutChar(t, 0xFFFD);
    }
    else if (c < 0xE0)
    {
      t->utf8_cp = c & 0x1F;
      t->utf8_more = 1;
    }
    else if (c < 0xF0)
    {
      t->utf8_cp = c & 0x0F;
      t->utf8_more = 2;
    }
    else if (c < 0xF8)
    {
      t->utf8_cp = c & 0x07;
      t->utf8_more = 3;
    }
    else
      itermPutChar(t, 0xFFFD);
    break;

  case IPSTATE_ESC:
    t->pstate = IPSTATE_GROUND;
    switch (c)
    {
    case '[':
      itermCsiInit(t);
      t->pstate = IPSTATE_CSI;
      break;
    case ']':
      t->osc_cmd = -1;
      t->osc_len = 0;
      t->osc_esc = 0;
      t->osc_buf[0] = 0;
      t->pstate = IPSTATE_OSC;
      break;
    case 'P':
    case 'X':
    case '^':
    case '_':
      t->osc_esc = 0;
      t->pstate = IPSTATE_STR;
      break;
    case '(':
    case ')':
    case '#':
      t->osc_cmd = c;
      t->pstate = IPSTATE_ESC2;
      break;
    case 0x1B:
      t->pstate = IPSTATE_ESC;
      break;
    default:
      itermEscDispatch(t, (char)c);
      break;
    }
    break;

  case IPSTATE_ESC2:
    if (t->osc_cmd == '(')
      t->charset[0] = (c == '0');
    else if (t->osc_cmd == ')')
      t->charset[1] = (c == '0');
    t->pstate = IPSTATE_GROUND;
    break;

  case IPSTATE_CSI:
    if (c >= '0' && c <= '9')
    {
      if (t->csi_argc < ITERM_CSI_MAXARGS)
      {
        if (t->csi_args[t->csi_argc] < 0)
          t->csi_args[t->csi_argc] = 0;
        t->csi_args[t->csi_argc] = t->csi_args[t->csi_argc] * 10 + (c - '0');
        if (t->csi_args[t->csi_argc] > 0xFFFF)
          t->csi_args[t->csi_argc] = 0xFFFF;
      }
      t->csi_has_arg = 1;
    }
    else if (c == ';' || c == ':')
    {
      if (t->csi_argc < ITERM_CSI_MAXARGS)
      {
        t->csi_colon[t->csi_argc] = (c == ':');
        t->csi_argc++;
        if (t->csi_argc < ITERM_CSI_MAXARGS)
          t->csi_args[t->csi_argc] = -1;
      }
      t->csi_has_arg = 1;
    }
    else if (c >= 0x3C && c <= 0x3F)
    {
      if (!t->csi_leader)
        t->csi_leader = (char)c;
    }
    else if (c >= 0x20 && c <= 0x2F)
      t->csi_intermed = (char)c;
    else if (c >= 0x40 && c <= 0x7E)
    {
      if (t->csi_has_arg && t->csi_argc < ITERM_CSI_MAXARGS)
        t->csi_argc++;
      t->pstate = IPSTATE_GROUND;
      itermCsiDispatch(t, (char)c);
    }
    else if (c == 0x1B)
      t->pstate = IPSTATE_ESC;
    else if (c == 0x18 || c == 0x1A)
      t->pstate = IPSTATE_GROUND;
    else if (c < 0x20)
      itermControl(t, c);
    break;

  case IPSTATE_OSC:
    if (t->osc_esc)
    {
      t->osc_esc = 0;
      if (c == '\\')
      {
        t->pstate = IPSTATE_GROUND;
        itermOscDispatch(t);
      }
      else
      {
        t->pstate = IPSTATE_ESC;
        itermByte(t, c);
      }
      break;
    }
    if (c == 0x07)
    {
      t->pstate = IPSTATE_GROUND;
      itermOscDispatch(t);
    }
    else if (c == 0x1B)
      t->osc_esc = 1;
    else if (c == 0x18 || c == 0x1A)
      t->pstate = IPSTATE_GROUND;
    else if (t->osc_cmd < 0)
    {
      if (c == ';')
      {
        t->osc_cmd = atoi(t->osc_buf);
        t->osc_len = 0;
        t->osc_buf[0] = 0;
      }
      else if (t->osc_len < ITERM_OSC_MAX - 1)
        t->osc_buf[t->osc_len++] = (char)c;
    }
    else if (t->osc_len < ITERM_OSC_MAX - 1)
      t->osc_buf[t->osc_len++] = (char)c;
    break;

  case IPSTATE_STR:
    if (t->osc_esc)
    {
      t->osc_esc = 0;
      if (c == '\\')
        t->pstate = IPSTATE_GROUND;
      else if (c == 0x1B)
        t->osc_esc = 1;
      break;
    }
    if (c == 0x1B)
      t->osc_esc = 1;
    else if (c == 0x07 || c == 0x18 || c == 0x1A)
      t->pstate = IPSTATE_GROUND;
    break;
  }
}

void iupTermParseBytes(Iterm* t, const char* buf, int len)
{
  int i;
  for (i = 0; i < len; i++)
    itermByte(t, (unsigned char)buf[i]);
}
