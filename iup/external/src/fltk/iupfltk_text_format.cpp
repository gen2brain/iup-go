/** \file
 * \brief Text Formatting - FLTK Implementation
 *
 * Implements IUP's FORMATTING/ADDFORMATTAG system using FLTK's
 * Fl_Text_Display highlight_data with Style_Table_Entry.
 *
 * See Copyright Notice in "iup.h"
 */

#include <FL/Fl.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_draw.H>

#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_array.h"
#include "iup_mask.h"
#include "iup_text.h"
}

#include "iupfltk_drv.h"


#define FLTK_MAX_STYLES 62

typedef struct _FltkFormatData
{
  Fl_Text_Buffer* style_buffer;
  Fl_Text_Display::Style_Table_Entry style_table[FLTK_MAX_STYLES];
  int num_styles;
  int insert_offset;
} FltkFormatData;

static FltkFormatData* fltkFormatGetData(Ihandle* ih)
{
  return (FltkFormatData*)iupAttribGet(ih, "_IUPFLTK_FORMATDATA");
}

static char fltkFormatFindOrAddStyle(FltkFormatData* fdata,
  Fl_Font font, Fl_Fontsize size, Fl_Color color, Fl_Color bgcolor, unsigned attr)
{
  for (int i = 0; i < fdata->num_styles; i++)
  {
    Fl_Text_Display::Style_Table_Entry* e = &fdata->style_table[i];
    if (e->font == font && e->size == size && e->color == color &&
        e->bgcolor == bgcolor && e->attr == attr)
      return (char)('A' + i);
  }

  if (fdata->num_styles >= FLTK_MAX_STYLES)
    return 'A';

  int idx = fdata->num_styles;
  fdata->style_table[idx].font = font;
  fdata->style_table[idx].size = size;
  fdata->style_table[idx].color = color;
  fdata->style_table[idx].bgcolor = bgcolor;
  fdata->style_table[idx].attr = attr;
  fdata->num_styles++;

  return (char)('A' + idx);
}

static void fltkFormatStyleUpdate(int pos, int nInserted, int nDeleted, int /*nRestyled*/, const char* /*deletedText*/, void* data)
{
  Ihandle* ih = (Ihandle*)data;
  FltkFormatData* fdata = fltkFormatGetData(ih);
  if (!fdata || !fdata->style_buffer)
    return;

  if (nInserted == 0 && nDeleted == 0)
  {
    fdata->style_buffer->unselect();
    return;
  }

  if (nInserted > 0)
  {
    char* style = (char*)malloc(nInserted + 1);
    memset(style, 'A', nInserted);
    style[nInserted] = '\0';
    fdata->style_buffer->replace(pos, pos + nDeleted, style);
    free(style);
  }
  else
  {
    fdata->style_buffer->remove(pos, pos + nDeleted);
  }
}

static int fltkFormatParseSelectionPos(Ihandle* ih, const char* value, int* start, int* end)
{
  Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
  Fl_Text_Buffer* buf = editor->buffer();

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    *start = editor->insert_position();
    *end = *start;
    return 1;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    *start = 0;
    *end = buf->length();
    return 1;
  }

  {
    int char_start = 0, char_end = 0;
    if (iupStrToIntInt(value, &char_start, &char_end, ':') != 2)
      return 0;

    if (char_start < 0) char_start = 0;
    if (char_end < 0) char_end = 0;

    int pos = 0, ch = 0;
    int buf_len = buf->length();
    *start = buf_len;
    *end = buf_len;

    while (pos < buf_len)
    {
      if (ch == char_start) *start = pos;
      if (ch == char_end) { *end = pos; break; }
      pos = buf->next_char(pos);
      ch++;
    }
  }

  return 1;
}

static int fltkFormatParseSelection(Ihandle* ih, const char* value, int* start, int* end)
{
  Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
  Fl_Text_Buffer* buf = editor->buffer();

  if (!value || iupStrEqualNoCase(value, "NONE"))
  {
    *start = editor->insert_position();
    *end = *start;
    return 1;
  }

  if (iupStrEqualNoCase(value, "ALL"))
  {
    *start = 0;
    *end = buf->length();
    return 1;
  }

  int lin1 = 1, col1 = 1, lin2 = 1, col2 = 1;
  if (sscanf(value, "%d,%d:%d,%d", &lin1, &col1, &lin2, &col2) != 4)
    return 0;

  int line_start;

  line_start = buf->skip_lines(0, lin1 - 1);
  *start = line_start + (col1 - 1);
  {
    int line_end = buf->line_end(line_start);
    if (*start > line_end) *start = line_end;
  }

  line_start = buf->skip_lines(0, lin2 - 1);
  *end = line_start + (col2 - 1);
  {
    int line_end = buf->line_end(line_start);
    if (*end > line_end) *end = line_end;
  }


  return 1;
}

static void fltkFormatFillStyleRange(FltkFormatData* fdata, Fl_Text_Buffer* text_buf, int start, int end, char style_char)
{
  if (start >= end || !fdata->style_buffer)
    return;

  int len = end - start;
  char* styles = (char*)malloc(len);

  for (int i = 0; i < len; i++)
  {
      unsigned char existing = (unsigned char)fdata->style_buffer->byte_at(start + i);
      int esi = existing - 'A';
      if (esi >= 0 && esi < fdata->num_styles)
      {
        Fl_Font merged = (Fl_Font)(fdata->style_table[esi].font | fdata->style_table[style_char - 'A'].font);
        Fl_Fontsize size = fdata->style_table[style_char - 'A'].size;
        Fl_Color color = fdata->style_table[style_char - 'A'].color;
        Fl_Color bgcolor = fdata->style_table[style_char - 'A'].bgcolor;
        unsigned attr = fdata->style_table[style_char - 'A'].attr;

        if (merged != fdata->style_table[style_char - 'A'].font)
          styles[i] = fltkFormatFindOrAddStyle(fdata, merged, size, color, bgcolor, attr);
        else
          styles[i] = style_char;
      }
      else
        styles[i] = style_char;
  }

  fdata->style_buffer->replace(start, end, styles, len);
  free(styles);
}

static void fltkIntToRoman(int value, char* buf, int bufsize, int upper)
{
  static const int vals[] = { 1000, 900, 500, 400, 100, 90, 50, 40, 10, 9, 5, 4, 1 };
  static const char* syms_lower[] = { "m","cm","d","cd","c","xc","l","xl","x","ix","v","iv","i" };
  static const char* syms_upper[] = { "M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I" };
  const char** syms = upper ? syms_upper : syms_lower;

  buf[0] = '\0';
  for (int i = 0; i < 13 && value > 0; i++)
  {
    while (value >= vals[i])
    {
      if ((int)strlen(buf) + (int)strlen(syms[i]) < bufsize - 1)
        strcat(buf, syms[i]);
      value -= vals[i];
    }
  }
}

static int fltkNumberingPrefix(int counter, const char* numbering, const char* style, char* buf, int bufsize)
{
  char number[64] = "";

  if (iupStrEqualNoCase(numbering, "BULLET"))
  {
    snprintf(buf, bufsize, "  \xe2\x80\xa2 ");
    return (int)strlen(buf);
  }

  if (iupStrEqualNoCase(numbering, "ARABIC"))
    snprintf(number, sizeof(number), "%d", counter);
  else if (iupStrEqualNoCase(numbering, "LCLETTER"))
  {
    if (counter >= 1 && counter <= 26)
      snprintf(number, sizeof(number), "%c", 'a' + counter - 1);
    else
      snprintf(number, sizeof(number), "%d", counter);
  }
  else if (iupStrEqualNoCase(numbering, "UCLETTER"))
  {
    if (counter >= 1 && counter <= 26)
      snprintf(number, sizeof(number), "%c", 'A' + counter - 1);
    else
      snprintf(number, sizeof(number), "%d", counter);
  }
  else if (iupStrEqualNoCase(numbering, "LCROMAN"))
    fltkIntToRoman(counter, number, sizeof(number), 0);
  else if (iupStrEqualNoCase(numbering, "UCROMAN"))
    fltkIntToRoman(counter, number, sizeof(number), 1);
  else
    return 0;

  if (style)
  {
    if (iupStrEqualNoCase(style, "RIGHTPARENTHESIS"))
      snprintf(buf, bufsize, "  %s) ", number);
    else if (iupStrEqualNoCase(style, "PARENTHESES"))
      snprintf(buf, bufsize, "  (%s) ", number);
    else if (iupStrEqualNoCase(style, "PERIOD"))
      snprintf(buf, bufsize, "  %s. ", number);
    else if (iupStrEqualNoCase(style, "NONUMBER"))
      snprintf(buf, bufsize, "    ");
    else
      snprintf(buf, bufsize, "  %s. ", number);
  }
  else
    snprintf(buf, bufsize, "  %s. ", number);

  return (int)strlen(buf);
}

static void fltkFormatApplyNumbering(Fl_Text_Buffer* buf, int start, int end, const char* numbering, const char* style)
{
  int start_line = buf->count_lines(0, start);
  int end_line = buf->count_lines(0, end);

  if (end_line > start_line)
  {
    int end_line_start = buf->line_start(end);
    if (end_line_start == end)
      end_line--;
  }

  for (int line = end_line; line >= start_line; line--)
  {
    int line_pos = buf->skip_lines(0, line);
    char prefix[128];

    if (fltkNumberingPrefix(line - start_line + 1, numbering, style, prefix, sizeof(prefix)) > 0)
      buf->insert(line_pos, prefix);
  }
}

static void fltkFormatApplyIndent(Fl_Text_Buffer* buf, int start, int end, int indent_pixels)
{
  int spaces = indent_pixels / 8;
  if (spaces < 1) spaces = 1;
  if (spaces > 40) spaces = 40;

  char indent[42];
  memset(indent, ' ', spaces);
  indent[spaces] = '\0';

  int start_line = buf->count_lines(0, start);
  int end_line = buf->count_lines(0, end);

  if (buf->line_start(start) != start)
    start_line++;

  for (int line = end_line; line >= start_line; line--)
  {
    int line_pos = buf->skip_lines(0, line);
    buf->insert(line_pos, indent);
  }
}

extern "C" IUP_SDK_API void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk)
{
  (void)bulk;

  if (!ih->data->is_multiline)
    return;

  FltkFormatData* fdata = fltkFormatGetData(ih);
  if (!fdata)
    return;

  Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
  Fl_Text_Buffer* buf = editor->buffer();

  int start = 0, end = 0;
  char* selection = iupAttribGet(formattag, "SELECTION");
  if (selection)
  {
    if (!fltkFormatParseSelection(ih, selection, &start, &end))
      return;
  }
  else
  {
    char* selectionpos = iupAttribGet(formattag, "SELECTIONPOS");
    if (selectionpos)
    {
      if (!fltkFormatParseSelectionPos(ih, selectionpos, &start, &end))
        return;
      start += fdata->insert_offset;
      end += fdata->insert_offset;
    }
    else
    {
      start = editor->insert_position();
      end = start;
    }
  }
  if (start > buf->length()) start = buf->length();
  if (end > buf->length()) end = buf->length();

  {
    char* numbering = iupAttribGet(formattag, "NUMBERING");
    if (numbering && !iupStrEqualNoCase(numbering, "NONE"))
    {
      int before = buf->length();
      char* numbering_style = iupAttribGet(formattag, "NUMBERINGSTYLE");
      fltkFormatApplyNumbering(buf, start, end, numbering, numbering_style);
      fdata->insert_offset += buf->length() - before;
      end += buf->length() - before;
    }
  }

  {
    char* indent = iupAttribGet(formattag, "INDENT");
    int indent_val = 0;
    if (indent && iupStrToInt(indent, &indent_val) && indent_val > 0)
    {
      int before = buf->length();
      fltkFormatApplyIndent(buf, start, end, indent_val);
      fdata->insert_offset += buf->length() - before;
      end += buf->length() - before;
    }
  }

  if (start >= end)
    return;

  Fl_Font font = fdata->style_table[0].font;
  Fl_Fontsize size = fdata->style_table[0].size;
  Fl_Color color = fdata->style_table[0].color;
  Fl_Color bgcolor = fdata->style_table[0].bgcolor;
  unsigned attr = 0;

  if (start < buf->length())
  {
    unsigned char existing_style = (unsigned char)fdata->style_buffer->byte_at(start);
    int si = existing_style - 'A';
    if (si >= 0 && si < fdata->num_styles)
    {
      font = fdata->style_table[si].font;
      size = fdata->style_table[si].size;
      color = fdata->style_table[si].color;
      bgcolor = fdata->style_table[si].bgcolor;
      attr = fdata->style_table[si].attr;
    }
  }

  int has_fontface = (iupAttribGet(formattag, "FONTFACE") != NULL);
  int has_fontsize = (iupAttribGet(formattag, "FONTSIZE") != NULL);
  int has_fontscale = (iupAttribGet(formattag, "FONTSCALE") != NULL);

  if (has_fontface || has_fontsize || has_fontscale)
    size = fdata->style_table[0].size;
  if (has_fontface)
    font = fdata->style_table[0].font;

  char* value;
  int val;

  value = iupAttribGet(formattag, "FONTFACE");
  if (value)
  {
    int is_bold = (font & FL_BOLD) ? 1 : 0;
    int is_italic = (font & FL_ITALIC) ? 1 : 0;
    int fl_font = iupfltkMapFontFace(value, is_bold, is_italic);
    if (fl_font >= 0)
      font = (Fl_Font)fl_font;
  }

  value = iupAttribGet(formattag, "FONTSIZE");
  if (value && iupStrToInt(value, &val))
  {
    if (val > 0)
      size = (Fl_Fontsize)val;
  }

  value = iupAttribGet(formattag, "FONTSCALE");
  if (value)
  {
    double scale = 1.0;
    if (iupStrEqualNoCase(value, "XX-LARGE")) scale = 2.0;
    else if (iupStrEqualNoCase(value, "X-LARGE")) scale = 1.5;
    else if (iupStrEqualNoCase(value, "LARGE")) scale = 1.2;
    else if (iupStrEqualNoCase(value, "MEDIUM")) scale = 1.0;
    else if (iupStrEqualNoCase(value, "SMALL")) scale = 0.83;
    else if (iupStrEqualNoCase(value, "X-SMALL")) scale = 0.69;
    else if (iupStrEqualNoCase(value, "XX-SMALL")) scale = 0.58;
    else iupStrToDouble(value, &scale);

    size = (Fl_Fontsize)(fdata->style_table[0].size * scale);
    if (size < 6) size = 6;
  }

  value = iupAttribGet(formattag, "ITALIC");
  if (value)
  {
    if (iupStrBoolean(value))
      font = (Fl_Font)(font | FL_ITALIC);
    else
      font = (Fl_Font)(font & ~FL_ITALIC);
  }

  value = iupAttribGet(formattag, "WEIGHT");
  if (value)
  {
    if (iupStrEqualNoCase(value, "BOLD") || iupStrEqualNoCase(value, "SEMIBOLD") ||
        iupStrEqualNoCase(value, "EXTRABOLD") || iupStrEqualNoCase(value, "HEAVY"))
      font = (Fl_Font)(font | FL_BOLD);
    else if (iupStrEqualNoCase(value, "NORMAL") || iupStrEqualNoCase(value, "LIGHT") ||
             iupStrEqualNoCase(value, "EXTRALIGHT"))
      font = (Fl_Font)(font & ~FL_BOLD);
  }

  value = iupAttribGet(formattag, "FGCOLOR");
  if (value)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
      color = fl_rgb_color(r, g, b);
  }

  value = iupAttribGet(formattag, "BGCOLOR");
  if (value)
  {
    unsigned char r, g, b;
    if (iupStrToRGB(value, &r, &g, &b))
    {
      bgcolor = fl_rgb_color(r, g, b);
      attr |= Fl_Text_Display::ATTR_BGCOLOR;
    }
  }

  value = iupAttribGet(formattag, "UNDERLINE");
  if (value)
  {
    attr &= ~Fl_Text_Display::ATTR_LINES_MASK;
    if (iupStrEqualNoCase(value, "SINGLE"))
      attr |= Fl_Text_Display::ATTR_UNDERLINE;
  }

  value = iupAttribGet(formattag, "STRIKEOUT");
  if (value)
  {
    attr &= ~Fl_Text_Display::ATTR_LINES_MASK;
    if (iupStrBoolean(value))
      attr |= Fl_Text_Display::ATTR_STRIKE_THROUGH;
  }

  value = iupAttribGet(formattag, "LINK");
  if (value)
  {
    if (!iupAttribGet(formattag, "FGCOLOR"))
      color = fl_rgb_color(0, 0, 255);
    attr &= ~Fl_Text_Display::ATTR_LINES_MASK;
    attr |= Fl_Text_Display::ATTR_UNDERLINE;
  }

  char style_char = fltkFormatFindOrAddStyle(fdata, font, size, color, bgcolor, attr);

  if (value)
  {
    char link_attr[32];
    snprintf(link_attr, sizeof(link_attr), "_IUPFLTK_LINK_%c", style_char);
    iupAttribSetStr(ih, link_attr, value);
  }

  fltkFormatFillStyleRange(fdata, buf, start, end, style_char);

  editor->highlight_data(fdata->style_buffer, fdata->style_table, fdata->num_styles, 0, NULL, NULL);
  editor->redisplay_range(start, end);
}

extern "C" IUP_SDK_API void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih)
{
  FltkFormatData* fdata = fltkFormatGetData(ih);
  if (fdata)
    fdata->insert_offset = 0;
  return NULL;
}

extern "C" IUP_SDK_API void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state)
{
  (void)ih;
  (void)state;
}

int iupfltkFormatSetRemoveFormattingAttrib(Ihandle* ih, const char* value)
{
  if (!ih->data->is_multiline)
    return 0;

  FltkFormatData* fdata = fltkFormatGetData(ih);
  if (!fdata || !fdata->style_buffer)
    return 0;

  Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
  Fl_Text_Buffer* buf = editor->buffer();

  int start = 0, end = buf->length();

  if (!iupStrEqualNoCase(value, "ALL"))
  {
    if (!buf->selection_position(&start, &end))
      return 0;
  }

  fltkFormatFillStyleRange(fdata, buf, start, end, 'A');
  editor->redisplay_range(start, end);

  return 0;
}

const char* iupfltkFormatGetLinkAtPos(Ihandle* ih, int pos)
{
  FltkFormatData* fdata = fltkFormatGetData(ih);
  if (!fdata || !fdata->style_buffer)
    return NULL;

  if (pos < 0 || pos >= fdata->style_buffer->length())
    return NULL;

  char style_char = fdata->style_buffer->byte_at(pos);
  if (style_char < 'A')
    return NULL;

  char link_attr[32];
  snprintf(link_attr, sizeof(link_attr), "_IUPFLTK_LINK_%c", style_char);
  return iupAttribGet(ih, link_attr);
}

void iupfltkFormatInit(Ihandle* ih)
{
  if (!ih->data->has_formatting)
    return;

  Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
  if (!editor)
    return;

  FltkFormatData* fdata = (FltkFormatData*)calloc(1, sizeof(FltkFormatData));

  fdata->style_buffer = new Fl_Text_Buffer();
  fdata->style_buffer->canUndo(0);

  fdata->style_table[0].font = editor->textfont();
  fdata->style_table[0].size = editor->textsize();
  fdata->style_table[0].color = editor->textcolor();
  fdata->style_table[0].bgcolor = 0;
  fdata->style_table[0].attr = 0;
  fdata->num_styles = 1;

  int len = editor->buffer()->length();
  if (len > 0)
  {
    char* style = (char*)malloc(len + 1);
    memset(style, 'A', len);
    style[len] = '\0';
    fdata->style_buffer->text(style);
    free(style);
  }

  editor->highlight_data(fdata->style_buffer, fdata->style_table, fdata->num_styles, 0, NULL, NULL);

  editor->buffer()->add_modify_callback(fltkFormatStyleUpdate, (void*)ih);

  iupAttribSet(ih, "_IUPFLTK_FORMATDATA", (char*)fdata);
}

void iupfltkFormatCleanup(Ihandle* ih)
{
  FltkFormatData* fdata = fltkFormatGetData(ih);
  if (!fdata)
    return;

  Fl_Text_Editor* editor = (Fl_Text_Editor*)ih->handle;
  if (editor)
  {
    editor->buffer()->remove_modify_callback(fltkFormatStyleUpdate, (void*)ih);
    editor->highlight_data(NULL, NULL, 0, 0, NULL, NULL);
  }

  delete fdata->style_buffer;
  free(fdata);

  iupAttribSet(ih, "_IUPFLTK_FORMATDATA", NULL);
}
