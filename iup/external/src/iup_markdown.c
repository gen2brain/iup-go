/** \file
 * \brief Markdown to IUP format tags converter.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_str.h"
#include "iup_class.h"
#include "iup_mask.h"
#include "iup_array.h"
#include "iup_text.h"
#include "iup_markdown.h"

/* Growable text buffer */
typedef struct {
  char* data;
  int len;      /* byte length */
  int alloc;
  int charlen;  /* UTF-8 character count (for SELECTIONPOS) */
} iMdBuf;

typedef struct {
  iMdBuf text;
  Ihandle* bulk_tag;
  int in_code_block;
  int pending_break;  /* blank line(s) seen before current block */
} iMdState;

static int iMdUtf8CharCount(const char* str, int byte_len)
{
  int count = 0, i;
  for (i = 0; i < byte_len; i++)
  {
    if ((str[i] & 0xC0) != 0x80)
      count++;
  }
  return count;
}

static void iMdBufInit(iMdBuf* buf)
{
  buf->alloc = 1024;
  buf->data = (char*)malloc(buf->alloc);
  buf->data[0] = 0;
  buf->len = 0;
  buf->charlen = 0;
}

static void iMdBufEnsure(iMdBuf* buf, int extra)
{
  if (buf->len + extra + 1 > buf->alloc)
  {
    int new_alloc = buf->alloc;
    char* new_data;
    while (buf->len + extra + 1 > new_alloc)
      new_alloc *= 2;
    new_data = (char*)realloc(buf->data, new_alloc);
    if (!new_data)
      return;
    buf->data = new_data;
    buf->alloc = new_alloc;
  }
}

static void iMdBufAppend(iMdBuf* buf, const char* str, int len)
{
  if (len <= 0)
    return;
  iMdBufEnsure(buf, len);
  memcpy(buf->data + buf->len, str, len);
  buf->len += len;
  buf->charlen += iMdUtf8CharCount(str, len);
  buf->data[buf->len] = 0;
}

static void iMdBufAppendChar(iMdBuf* buf, char c)
{
  iMdBufEnsure(buf, 1);
  buf->data[buf->len] = c;
  buf->len++;
  buf->charlen++;
  buf->data[buf->len] = 0;
}

static void iMdBufAppendStr(iMdBuf* buf, const char* str)
{
  iMdBufAppend(buf, str, (int)strlen(str));
}

static void iMdBufFree(iMdBuf* buf)
{
  if (buf->data)
    free(buf->data);
  buf->data = NULL;
  buf->len = 0;
  buf->charlen = 0;
  buf->alloc = 0;
}

static void iMdBlockSeparator(iMdState* s)
{
  if (s->text.len > 0)
  {
    iMdBufAppendChar(&s->text, '\n');
    if (s->pending_break)
      iMdBufAppendChar(&s->text, '\n');
  }
  s->pending_break = 0;
}

static Ihandle* iMdCreateTag(iMdState* s, int start, int end)
{
  Ihandle* tag = IupUser();
  IupSetStrf(tag, "SELECTIONPOS", "%d:%d", start, end);
  IupAppend(s->bulk_tag, tag);
  return tag;
}

static int iMdIsBlankLine(const char* line, int len)
{
  int i;
  for (i = 0; i < len; i++)
  {
    if (line[i] != ' ' && line[i] != '\t')
      return 0;
  }
  return 1;
}

static int iMdCountLeadingChar(const char* str, int len, char c)
{
  int count = 0;
  while (count < len && str[count] == c)
    count++;
  return count;
}

static int iMdIsHorizontalRule(const char* line, int len)
{
  int i, count = 0;
  char rule_char = 0;

  for (i = 0; i < len; i++)
  {
    if (line[i] == ' ' || line[i] == '\t')
      continue;
    if (line[i] == '-' || line[i] == '*' || line[i] == '_')
    {
      if (rule_char == 0)
        rule_char = line[i];
      else if (line[i] != rule_char)
        return 0;
      count++;
    }
    else
      return 0;
  }
  return count >= 3;
}

static int iMdIsOrderedListItem(const char* line, int len, int* num_end)
{
  int i = 0;
  if (i >= len || !iup_isdigit(line[i]))
    return 0;
  while (i < len && iup_isdigit(line[i]))
    i++;
  if (i >= len || (line[i] != '.' && line[i] != ')'))
    return 0;
  i++;
  if (i >= len || line[i] != ' ')
    return 0;
  *num_end = i + 1;
  return 1;
}

static int iMdIsUnorderedListItem(const char* line, int len)
{
  if (len < 2)
    return 0;
  if ((line[0] == '-' || line[0] == '*' || line[0] == '+') && line[1] == ' ')
    return 1;
  return 0;
}

static int iMdIsEscapable(char c)
{
  return c == '\\' || c == '`' || c == '*' || c == '_' || c == '{' || c == '}' ||
         c == '[' || c == ']' || c == '(' || c == ')' || c == '#' || c == '+' ||
         c == '-' || c == '.' || c == '!' || c == '|' || c == '>' || c == '~';
}


/* Find matching closing delimiter for emphasis.
   Returns position after the closing delimiter, or -1 if not found. */
static int iMdFindClosingDelimiter(const char* text, int len, char delim, int count)
{
  int i = 0;

  while (i < len)
  {
    if (text[i] == '\\' && i + 1 < len && iMdIsEscapable(text[i + 1]))
    {
      i += 2;
      continue;
    }

    if (text[i] == '`')
    {
      int bt = iMdCountLeadingChar(text + i, len - i, '`');
      i += bt;
      while (i < len)
      {
        int ct = iMdCountLeadingChar(text + i, len - i, '`');
        if (ct == bt)
        {
          i += ct;
          break;
        }
        if (ct > 0)
          i += ct;
        else
          i++;
      }
      continue;
    }

    if (text[i] == delim)
    {
      int dc = iMdCountLeadingChar(text + i, len - i, delim);
      if (dc >= count)
        return i + count;
      i += dc;
      continue;
    }

    i++;
  }
  return -1;
}

static void iMdParseInline(iMdState* s, const char* text, int len);

/* Parse inline code span: `code` or ``code`` */
static int iMdParseInlineCode(iMdState* s, const char* text, int len)
{
  int bt, i, start, end;

  bt = iMdCountLeadingChar(text, len, '`');
  if (bt == 0)
    return 0;

  /* Find closing backticks */
  i = bt;
  while (i < len)
  {
    int ct = iMdCountLeadingChar(text + i, len - i, '`');
    if (ct == bt)
    {
      /* Found matching close */
      start = s->text.charlen;
      iMdBufAppend(&s->text, text + bt, i - bt);
      end = s->text.charlen;

      if (end > start)
      {
        Ihandle* tag = iMdCreateTag(s, start, end);
        IupSetAttribute(tag, "FONTFACE", "Courier");
        IupSetAttribute(tag, "BGCOLOR", "230 230 230");
      }
      return i + ct;
    }
    if (ct > 0)
      i += ct;
    else
      i++;
  }
  return 0;
}

/* Parse link: [text](url) - returns chars consumed or 0 */
static int iMdParseLink(iMdState* s, const char* text, int len)
{
  int i, depth, text_start, text_end, url_start, url_end, tag_start;

  if (len < 4 || text[0] != '[')
    return 0;

  /* Find closing ] */
  i = 1;
  depth = 1;
  while (i < len && depth > 0)
  {
    if (text[i] == '\\' && i + 1 < len)
    {
      i += 2;
      continue;
    }
    if (text[i] == '[') depth++;
    else if (text[i] == ']') depth--;
    i++;
  }
  if (depth != 0)
    return 0;

  text_start = 1;
  text_end = i - 1;

  /* Expect ( immediately */
  if (i >= len || text[i] != '(')
    return 0;
  i++;
  url_start = i;

  /* Find closing ) */
  depth = 1;
  while (i < len && depth > 0)
  {
    if (text[i] == '(') depth++;
    else if (text[i] == ')') depth--;
    i++;
  }
  if (depth != 0)
    return 0;
  url_end = i - 1;

  /* Emit link text and create tag */
  {
    int content_len = text_end - text_start;
    int is_image_link = (content_len >= 5 && text[text_start] == '!' && text[text_start + 1] == '[');

    tag_start = s->text.charlen;
    iMdParseInline(s, text + text_start, text_end - text_start);

    if (s->text.charlen > tag_start)
    {
      char url_buf[2048];
      int url_len = url_end - url_start;
      Ihandle* tag;

      if (url_len > (int)sizeof(url_buf) - 1)
        url_len = (int)sizeof(url_buf) - 1;
      memcpy(url_buf, text + url_start, url_len);
      url_buf[url_len] = 0;

      tag = iMdCreateTag(s, tag_start, s->text.charlen);
      IupSetStrAttribute(tag, "LINK", url_buf);

      if (is_image_link)
        IupSetAttribute(tag, "UNDERLINE", "NO");
    }
  }

  return i;
}

/* Parse image: ![alt](name) - returns chars consumed or 0 */
static int iMdParseImage(iMdState* s, const char* text, int len)
{
  int i, depth, alt_end, name_start, name_end, tag_start;

  if (len < 5 || text[0] != '!' || text[1] != '[')
    return 0;

  /* Find closing ] for alt text */
  i = 2;
  depth = 1;
  while (i < len && depth > 0)
  {
    if (text[i] == '\\' && i + 1 < len)
    {
      i += 2;
      continue;
    }
    if (text[i] == '[') depth++;
    else if (text[i] == ']') depth--;
    i++;
  }
  if (depth != 0)
    return 0;

  alt_end = i;
  (void)alt_end;

  /* Expect ( immediately */
  if (i >= len || text[i] != '(')
    return 0;
  i++;
  name_start = i;

  /* Find closing ) */
  depth = 1;
  while (i < len && depth > 0)
  {
    if (text[i] == '(') depth++;
    else if (text[i] == ')') depth--;
    i++;
  }
  if (depth != 0)
    return 0;
  name_end = i - 1;

  /* Insert a placeholder space for the image and create tag */
  tag_start = s->text.charlen;
  iMdBufAppendChar(&s->text, ' ');
  {
    char name_buf[512];
    int name_len = name_end - name_start;
    Ihandle* tag;

    if (name_len > (int)sizeof(name_buf) - 1)
      name_len = (int)sizeof(name_buf) - 1;
    memcpy(name_buf, text + name_start, name_len);
    name_buf[name_len] = 0;

    tag = iMdCreateTag(s, tag_start, s->text.charlen);
    IupSetStrAttribute(tag, "IMAGE", name_buf);
  }

  return i;
}

/* Extract an HTML attribute value from a tag string.
   Searches for attr="value" or attr='value', copies value into buf.
   Returns 1 if found, 0 otherwise. */
static int iMdGetHtmlAttr(const char* tag, int tag_len, const char* attr, char* buf, int buf_size)
{
  int attr_len = (int)strlen(attr);
  int i;

  for (i = 0; i < tag_len - attr_len; i++)
  {
    if (iupStrEqualNoCasePartial(tag + i, attr) && tag[i + attr_len] == '=')
    {
      int start, end;
      char quote;
      i += attr_len + 1;

      /* Skip optional quote */
      if (i < tag_len && (tag[i] == '"' || tag[i] == '\''))
      {
        quote = tag[i];
        i++;
        start = i;
        while (i < tag_len && tag[i] != quote)
          i++;
        end = i;
      }
      else
      {
        start = i;
        while (i < tag_len && tag[i] != ' ' && tag[i] != '>' && tag[i] != '/')
          i++;
        end = i;
      }

      {
        int len = end - start;
        if (len > buf_size - 1)
          len = buf_size - 1;
        memcpy(buf, tag + start, len);
        buf[len] = 0;
      }
      return 1;
    }
  }
  return 0;
}

/* Parse HTML img tag: <img src="path"> with optional width/height.
   Returns chars consumed or 0. */
static int iMdParseHtmlImg(iMdState* s, const char* text, int len)
{
  int i, tag_end, tag_start;
  char src[512];

  if (len < 10 || text[0] != '<')
    return 0;

  /* Check for <img (case-insensitive) */
  if (!iupStrEqualNoCasePartial(text + 1, "img"))
    return 0;
  if (text[4] != ' ' && text[4] != '\t')
    return 0;

  /* Find closing > */
  i = 5;
  while (i < len && text[i] != '>')
    i++;
  if (i >= len)
    return 0;
  tag_end = i + 1;

  /* Extract src attribute */
  if (!iMdGetHtmlAttr(text, tag_end, "src", src, sizeof(src)))
    return 0;
  if (src[0] == 0)
    return 0;

  /* Insert a placeholder space for the image and create tag */
  tag_start = s->text.charlen;
  iMdBufAppendChar(&s->text, ' ');
  {
    Ihandle* tag = iMdCreateTag(s, tag_start, s->text.charlen);
    char attr_buf[64];

    IupSetStrAttribute(tag, "IMAGE", src);

    if (iMdGetHtmlAttr(text, tag_end, "width", attr_buf, sizeof(attr_buf)))
      IupSetStrAttribute(tag, "WIDTH", attr_buf);
    if (iMdGetHtmlAttr(text, tag_end, "height", attr_buf, sizeof(attr_buf)))
      IupSetStrAttribute(tag, "HEIGHT", attr_buf);
  }

  return tag_end;
}

/* Parse emphasis: *italic*, **bold**, ***both***, and _ variants */
static int iMdParseEmphasis(iMdState* s, const char* text, int len, int text_offset)
{
  char delim = text[0];
  int count, close_pos, inner_start, inner_end;

  /* For underscore, require word boundary before opening delimiter */
  if (delim == '_' && text_offset > 0)
  {
    /* We don't have the full buffer context here, so we check the char before
       the emphasis start in the inline text being parsed. The caller passes
       text_offset which is the position within the current inline span. */
    /* Can't easily check here, so skip this check for simplicity.
       Underscore emphasis works the same as asterisk. */
  }

  count = iMdCountLeadingChar(text, len, delim);
  if (count > 3)
    count = 3;
  if (count == 0)
    return 0;

  close_pos = iMdFindClosingDelimiter(text + count, len - count, delim, count);
  if (close_pos < 0)
    return 0;

  /* Parse inner content */
  inner_start = s->text.charlen;
  iMdParseInline(s, text + count, close_pos - count);
  inner_end = s->text.charlen;

  if (inner_end > inner_start)
  {
    if (count == 1)
    {
      Ihandle* tag = iMdCreateTag(s, inner_start, inner_end);
      IupSetAttribute(tag, "ITALIC", "YES");
    }
    else if (count == 2)
    {
      Ihandle* tag = iMdCreateTag(s, inner_start, inner_end);
      IupSetAttribute(tag, "WEIGHT", "BOLD");
    }
    else /* count == 3 */
    {
      Ihandle* tag1 = iMdCreateTag(s, inner_start, inner_end);
      IupSetAttribute(tag1, "WEIGHT", "BOLD");
      {
        Ihandle* tag2 = iMdCreateTag(s, inner_start, inner_end);
        IupSetAttribute(tag2, "ITALIC", "YES");
      }
    }
  }

  return count + close_pos;
}


static void iMdParseInline(iMdState* s, const char* text, int len)
{
  int i = 0;

  while (i < len)
  {
    /* Backslash escape */
    if (text[i] == '\\' && i + 1 < len)
    {
      if (iMdIsEscapable(text[i + 1]))
      {
        iMdBufAppendChar(&s->text, text[i + 1]);
        i += 2;
        continue;
      }
      /* Backslash at end of line = hard break (handled at block level) */
    }

    /* Inline code */
    if (text[i] == '`')
    {
      int consumed = iMdParseInlineCode(s, text + i, len - i);
      if (consumed > 0)
      {
        i += consumed;
        continue;
      }
    }

    /* Image (must check before link since it starts with ![) */
    if (text[i] == '!' && i + 1 < len && text[i + 1] == '[')
    {
      int consumed = iMdParseImage(s, text + i, len - i);
      if (consumed > 0)
      {
        i += consumed;
        continue;
      }
    }

    /* Link */
    if (text[i] == '[')
    {
      int consumed = iMdParseLink(s, text + i, len - i);
      if (consumed > 0)
      {
        i += consumed;
        continue;
      }
    }

    /* HTML img tag */
    if (text[i] == '<')
    {
      int consumed = iMdParseHtmlImg(s, text + i, len - i);
      if (consumed > 0)
      {
        i += consumed;
        continue;
      }
    }

    /* Emphasis */
    if (text[i] == '*' || text[i] == '_')
    {
      int consumed = iMdParseEmphasis(s, text + i, len - i, i);
      if (consumed > 0)
      {
        i += consumed;
        continue;
      }
    }

    /* Regular character */
    iMdBufAppendChar(&s->text, text[i]);
    i++;
  }
}

static void iMdParseHeading(iMdState* s, const char* line, int len)
{
  int level, start, end;
  Ihandle* tag;
  static const char* fontscales[] = { NULL, "XX-LARGE", "X-LARGE", "LARGE", "1.1", NULL, NULL };
  static const char* spaceafters[] = { NULL, "10", "8", "6", "4", "2", "2" };

  level = iMdCountLeadingChar(line, len, '#');
  if (level > 6) level = 6;

  /* Skip '#' chars and space */
  line += level;
  len -= level;
  if (len > 0 && line[0] == ' ')
  {
    line++;
    len--;
  }

  /* Trim trailing '#' and spaces */
  while (len > 0 && (line[len - 1] == '#' || line[len - 1] == ' '))
    len--;

  iMdBlockSeparator(s);

  start = s->text.charlen;
  iMdParseInline(s, line, len);
  end = s->text.charlen;

  if (end <= start)
    return;

  tag = iMdCreateTag(s, start, end);
  IupSetAttribute(tag, "WEIGHT", "BOLD");
  IupSetAttribute(tag, "SPACEAFTER", spaceafters[level]);

  if (level <= 4 && fontscales[level])
    IupSetAttribute(tag, "FONTSCALE", fontscales[level]);

  if (level == 6)
    IupSetAttribute(tag, "ITALIC", "YES");
}

static void iMdParseCodeBlock(iMdState* s, const char** p_input)
{
  const char* input = *p_input;
  int line_len;
  int first = 1;

  while (*input)
  {
    const char* next = iupStrNextLine(input, &line_len);

    /* Check for closing fence */
    if (line_len >= 3 && input[0] == '`' && input[1] == '`' && input[2] == '`')
    {
      *p_input = next;
      return;
    }

    if (first)
    {
      iMdBlockSeparator(s);
      first = 0;
    }
    else if (s->text.len > 0)
      iMdBufAppendChar(&s->text, '\n');

    {
      int start = s->text.charlen;
      Ihandle* tag;

      iMdBufAppend(&s->text, input, line_len);

      tag = iMdCreateTag(s, start, s->text.charlen);
      IupSetAttribute(tag, "FONTFACE", "Courier");
      IupSetAttribute(tag, "BGCOLOR", "230 230 230");
    }

    input = next;
  }

  *p_input = input;
}

static void iMdParseBlockquote(iMdState* s, const char* line, int len)
{
  int start;
  Ihandle* tag;

  /* Skip '>' and optional space */
  line++;
  len--;
  if (len > 0 && line[0] == ' ')
  {
    line++;
    len--;
  }

  iMdBlockSeparator(s);

  start = s->text.charlen;
  iMdParseInline(s, line, len);

  if (s->text.charlen > start)
  {
    tag = iMdCreateTag(s, start, s->text.charlen);
    IupSetAttribute(tag, "INDENT", "30");
    IupSetAttribute(tag, "ITALIC", "YES");
    IupSetAttribute(tag, "FGCOLOR", "100 100 100");
  }
}

static void iMdParseUnorderedList(iMdState* s, const char* line, int len)
{
  /* Skip marker and space */
  line += 2;
  len -= 2;

  iMdBlockSeparator(s);

  /* Insert bullet prefix (UTF-8 bullet character) */
  iMdBufAppendStr(&s->text, "  \xe2\x80\xa2 ");
  iMdParseInline(s, line, len);
}

static void iMdParseOrderedList(iMdState* s, const char* line, int len, int num_end)
{
  char prefix[32];
  int prefix_len;

  iMdBlockSeparator(s);

  /* Build prefix from the original number text */
  prefix_len = num_end;
  if (prefix_len > (int)sizeof(prefix) - 4)
    prefix_len = (int)sizeof(prefix) - 4;
  memcpy(prefix + 2, line, prefix_len);
  prefix[0] = ' ';
  prefix[1] = ' ';
  prefix[prefix_len + 2] = 0;
  iMdBufAppendStr(&s->text, prefix);

  /* Parse content after the number marker */
  iMdParseInline(s, line + num_end, len - num_end);
}

static void iMdParseHorizontalRule(iMdState* s)
{
  int start;
  Ihandle* tag;
  /* 20 horizontal bar characters (UTF-8: E2 94 81) */
  static const char rule[] =
    "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81"
    "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81"
    "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81"
    "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81";

  iMdBlockSeparator(s);

  start = s->text.charlen;
  iMdBufAppendStr(&s->text, rule);

  tag = iMdCreateTag(s, start, s->text.charlen);
  IupSetAttribute(tag, "FGCOLOR", "160 160 160");
  IupSetAttribute(tag, "ALIGNMENT", "CENTER");
}

static void iMdParseParagraph(iMdState* s, const char* text, int len)
{
  iMdBlockSeparator(s);

  iMdParseInline(s, text, len);
}

/* Collect continuation lines for a paragraph until a blank line or new block.
   Returns pointer to after the paragraph. */
static const char* iMdCollectParagraph(const char* input, iMdBuf* para)
{
  int first = 1;

  while (*input)
  {
    int line_len;
    const char* next = iupStrNextLine(input, &line_len);

    if (iMdIsBlankLine(input, line_len))
      break;

    /* Check for block-level markers that would break the paragraph */
    if (line_len > 0 && input[0] == '#')
      break;
    if (line_len >= 3 && input[0] == '`' && input[1] == '`' && input[2] == '`')
      break;
    if (line_len > 0 && input[0] == '>')
      break;
    if (iMdIsHorizontalRule(input, line_len))
      break;
    if (iMdIsUnorderedListItem(input, line_len))
      break;
    {
      int num_end;
      if (iMdIsOrderedListItem(input, line_len, &num_end))
        break;
    }

    /* Handle hard line break: backslash at end of line */
    if (!first)
    {
      if (para->len > 0 && para->data[para->len - 1] == '\\')
      {
        para->len--;
        para->charlen--;
        para->data[para->len] = 0;
        iMdBufAppendChar(para, '\n');
      }
      else
        iMdBufAppendChar(para, ' ');
    }

    iMdBufAppend(para, input, line_len);
    first = 0;
    input = next;
  }

  return input;
}

static void iMdParseDocument(iMdState* s, const char* input)
{
  while (*input)
  {
    int line_len;
    const char* next = iupStrNextLine(input, &line_len);

    /* Skip blank lines, mark block separation */
    if (iMdIsBlankLine(input, line_len))
    {
      if (s->text.len > 0)
        s->pending_break = 1;
      input = next;
      continue;
    }

    /* Fenced code block */
    if (line_len >= 3 && input[0] == '`' && input[1] == '`' && input[2] == '`')
    {
      input = next;
      iMdParseCodeBlock(s, &input);
      continue;
    }

    /* Heading */
    if (input[0] == '#')
    {
      int level = iMdCountLeadingChar(input, line_len, '#');
      if (level <= 6 && level < line_len && input[level] == ' ')
      {
        iMdParseHeading(s, input, line_len);
        input = next;
        continue;
      }
    }

    /* Horizontal rule */
    if (iMdIsHorizontalRule(input, line_len))
    {
      iMdParseHorizontalRule(s);
      input = next;
      continue;
    }

    /* Blockquote */
    if (input[0] == '>' && (line_len == 1 || input[1] == ' '))
    {
      iMdParseBlockquote(s, input, line_len);
      input = next;
      continue;
    }

    /* Unordered list */
    if (iMdIsUnorderedListItem(input, line_len))
    {
      iMdParseUnorderedList(s, input, line_len);
      input = next;
      continue;
    }

    /* Ordered list */
    {
      int num_end;
      if (iMdIsOrderedListItem(input, line_len, &num_end))
      {
        iMdParseOrderedList(s, input, line_len, num_end);
        input = next;
        continue;
      }
    }

    /* Paragraph (collect continuation lines) */
    {
      iMdBuf para;
      iMdBufInit(&para);
      input = iMdCollectParagraph(input, &para);
      if (para.len > 0)
        iMdParseParagraph(s, para.data, para.len);
      iMdBufFree(&para);
    }
  }
}

static void iMdConvert(iMdState* s, const char* markdown_text)
{
  iMdBufInit(&s->text);
  s->bulk_tag = IupUser();
  IupSetAttribute(s->bulk_tag, "BULK", "YES");
  s->in_code_block = 0;
  s->pending_break = 0;

  iMdParseDocument(s, markdown_text);
}

static void iMdOffsetTags(Ihandle* bulk_tag, int offset)
{
  int i, count = IupGetChildCount(bulk_tag);
  for (i = 0; i < count; i++)
  {
    Ihandle* child = IupGetChild(bulk_tag, i);
    char* selectionpos = IupGetAttribute(child, "SELECTIONPOS");
    int start, end;
    if (selectionpos && iupStrToIntInt(selectionpos, &start, &end, ':') == 2)
      IupSetStrf(child, "SELECTIONPOS", "%d:%d", start + offset, end + offset);
  }
}

void iupMarkdownSetValue(Ihandle* ih, const char* markdown_text)
{
  iMdState state;

  if (!markdown_text || !markdown_text[0])
    return;

  iMdConvert(&state, markdown_text);
  IupSetAttribute(state.bulk_tag, "CLEANOUT", "YES");

  IupSetStrAttribute(ih, "VALUE", state.text.data);

  iupTextSetAddFormatTagHandleAttrib(ih, (const char*)state.bulk_tag);

  iMdBufFree(&state.text);
}

static void iMdResetColor(Ihandle* ih, const char* name, Ihandle* tag, const char* tagname)
{
  unsigned char r, g, b;
  char* value = IupGetAttribute(ih, name);
  if (!value)
    return;
  if (!iupStrToRGB(value, &r, &g, &b))
  {
    char* global = IupGetGlobal(value);
    if (!global || !iupStrToRGB(global, &r, &g, &b))
      return;
  }
  IupSetStrf(tag, tagname, "%d %d %d", r, g, b);
}

static void iMdAppendResetTag(Ihandle* ih, Ihandle* bulk_tag, int charlen)
{
  Ihandle* reset = IupUser();
  char* font;

  IupSetStrf(reset, "SELECTIONPOS", "%d:%d", 0, charlen);
  IupSetAttribute(reset, "WEIGHT", "NORMAL");
  IupSetAttribute(reset, "ITALIC", "NO");
  IupSetAttribute(reset, "UNDERLINE", "NONE");
  IupSetAttribute(reset, "STRIKEOUT", "NO");
  IupSetAttribute(reset, "FONTSCALE", "1.0");
  IupSetAttribute(reset, "INDENT", "0");
  IupSetAttribute(reset, "ALIGNMENT", "LEFT");
  iMdResetColor(ih, "FGCOLOR", reset, "FGCOLOR");

  font = IupGetAttribute(ih, "FONT");
  if (font)
  {
    char family[256];
    const char* comma = strchr(font, ',');
    int n = comma ? (int)(comma - font) : (int)strlen(font);
    if (n > (int)sizeof(family) - 1)
      n = (int)sizeof(family) - 1;
    memcpy(family, font, n);
    while (n > 0 && family[n - 1] == ' ')
      n--;
    family[n] = 0;
    if (n > 0)
      IupSetStrAttribute(reset, "FONTFACE", family);
  }

  if (IupGetChildCount(bulk_tag) > 0)
    IupInsert(bulk_tag, IupGetChild(bulk_tag, 0), reset);
  else
    IupAppend(bulk_tag, reset);
}

void iupMarkdownAppendValue(Ihandle* ih, const char* markdown_text)
{
  iMdState state;
  int offset;

  if (!markdown_text || !markdown_text[0])
    return;

  iMdConvert(&state, markdown_text);

  if (state.text.charlen == 0)
  {
    IupDestroy(state.bulk_tag);
    iMdBufFree(&state.text);
    return;
  }

  IupSetStrAttribute(ih, "APPEND", state.text.data);

  offset = IupGetInt(ih, "COUNT") - state.text.charlen;
  if (offset < 0)
    offset = 0;

  iMdAppendResetTag(ih, state.bulk_tag, state.text.charlen);

  iMdOffsetTags(state.bulk_tag, offset);

  iupTextSetAddFormatTagHandleAttrib(ih, (const char*)state.bulk_tag);

  iMdBufFree(&state.text);
}


enum { IMD_BLANK, IMD_RULE, IMD_CODE, IMD_HEADING, IMD_QUOTE, IMD_ULIST, IMD_OLIST, IMD_PARA };

typedef struct {
  const char* link;
  const char* image;
  float scale;
  unsigned char bold, italic, strike, mono, quote;
} iMdFmt;

static int iMdFaceIsMono(const char* face)
{
  static const char* names[] = { "mono", "courier", "consol", "menlo", "monaco", "terminal", "fixed", "cascadia", NULL };
  int i, j, len;

  if (!face)
    return 0;

  len = (int)strlen(face);
  for (i = 0; i < len; i++)
  {
    for (j = 0; names[j]; j++)
    {
      if (iupStrEqualNoCasePartial(face + i, names[j]))
        return 1;
    }
  }
  return 0;
}

static float iMdNamedScale(const char* value)
{
  if (iupStrEqualNoCase(value, "XX-SMALL")) return 0.58f;
  if (iupStrEqualNoCase(value, "X-SMALL")) return 0.64f;
  if (iupStrEqualNoCase(value, "SMALL")) return 0.83f;
  if (iupStrEqualNoCase(value, "MEDIUM")) return 1.0f;
  if (iupStrEqualNoCase(value, "LARGE")) return 1.2f;
  if (iupStrEqualNoCase(value, "X-LARGE")) return 1.44f;
  if (iupStrEqualNoCase(value, "XX-LARGE")) return 1.73f;
  return 0;
}

static void iMdApplyTag(Ihandle* tag, iMdFmt* fmt, int charlen, int base_size)
{
  char *value;
  const char *link, *image;
  int start = 0, end = 0, i;
  int bold = -1, italic = -1, strike = -1, mono = -1, quote = -1;
  float scale = 0;

  value = IupGetAttribute(tag, "SELECTIONPOS");
  if (!value || iupStrToIntInt(value, &start, &end, ':') != 2)
    return;
  if (start < 0) start = 0;
  if (end > charlen) end = charlen;
  if (end <= start)
    return;

  value = IupGetAttribute(tag, "WEIGHT");
  if (value)
    bold = (iupStrEqualNoCase(value, "BOLD") || iupStrEqualNoCase(value, "SEMIBOLD") ||
            iupStrEqualNoCase(value, "EXTRABOLD") || iupStrEqualNoCase(value, "HEAVY")) ? 1 : 0;

  value = IupGetAttribute(tag, "ITALIC");
  if (value)
    italic = iupStrBoolean(value);

  value = IupGetAttribute(tag, "STRIKEOUT");
  if (value)
    strike = iupStrBoolean(value);

  value = IupGetAttribute(tag, "FONTFACE");
  if (value)
    mono = iMdFaceIsMono(value);

  value = IupGetAttribute(tag, "FONTSCALE");
  if (value)
  {
    double dval = 0;
    scale = iMdNamedScale(value);
    if (scale == 0 && iupStrToDouble(value, &dval) && dval > 0)
      scale = (float)dval;
  }

  value = IupGetAttribute(tag, "FONTSIZE");
  if (value && base_size > 0)
  {
    int size = 0;
    if (iupStrToInt(value, &size) && size != 0)
    {
      if (size < 0) size = -size;
      scale = (float)size / (float)base_size;
    }
  }

  value = IupGetAttribute(tag, "INDENT");
  if (value)
  {
    int indent = 0;
    iupStrToInt(value, &indent);
    quote = indent > 0 ? 1 : 0;
  }

  link = IupGetAttribute(tag, "LINK");
  image = IupGetAttribute(tag, "IMAGE");

  for (i = start; i < end; i++)
  {
    if (bold >= 0) fmt[i].bold = (unsigned char)bold;
    if (italic >= 0) fmt[i].italic = (unsigned char)italic;
    if (strike >= 0) fmt[i].strike = (unsigned char)strike;
    if (mono >= 0) fmt[i].mono = (unsigned char)mono;
    if (quote >= 0) fmt[i].quote = (unsigned char)quote;
    if (scale > 0) fmt[i].scale = scale;
    if (link) fmt[i].link = link;
    if (image) fmt[i].image = image;
  }
}

static int iMdCharIs(const char* text, const int* choff, int c, const char* utf8, int utf8_len)
{
  return (choff[c + 1] - choff[c]) == utf8_len && memcmp(text + choff[c], utf8, utf8_len) == 0;
}

static int iMdCharIsSpace(const char* text, const int* choff, int c)
{
  return (choff[c + 1] - choff[c]) == 1 && (text[choff[c]] == ' ' || text[choff[c]] == '\t');
}

static int iMdCharIsByte(const char* text, const int* choff, int c, char b)
{
  return (choff[c + 1] - choff[c]) == 1 && text[choff[c]] == b;
}

static int iMdLineKind(const char* text, const int* choff, const iMdFmt* fmt, int c0, int c1, int* level, int* content)
{
  int i, blank = 1, all_mono = 1, all_bold = 1, all_quote = 1, all_rule = 1, count = 0;
  float max_scale = 0;

  *level = 0;
  *content = c0;

  for (i = c0; i < c1; i++)
  {
    if (iMdCharIsSpace(text, choff, i))
      continue;

    blank = 0;
    count++;
    if (!fmt[i].mono) all_mono = 0;
    if (!fmt[i].bold) all_bold = 0;
    if (!fmt[i].quote) all_quote = 0;
    if (!iMdCharIs(text, choff, i, "\xe2\x94\x81", 3)) all_rule = 0;
    if (fmt[i].scale > max_scale) max_scale = fmt[i].scale;
  }

  if (blank)
    return IMD_BLANK;

  if (all_rule && count >= 3)
    return IMD_RULE;

  if (all_mono)
    return IMD_CODE;

  if (all_quote)
    return IMD_QUOTE;

  if (all_bold && max_scale >= 1.04f)
  {
    if (max_scale >= 1.6f) *level = 1;
    else if (max_scale >= 1.3f) *level = 2;
    else if (max_scale >= 1.12f) *level = 3;
    else *level = 4;
    return IMD_HEADING;
  }

  i = c0;
  while (i < c1 && iMdCharIsByte(text, choff, i, ' '))
    i++;

  if (i < c1 - 1 && iMdCharIs(text, choff, i, "\xe2\x80\xa2", 3) && iMdCharIsByte(text, choff, i + 1, ' '))
  {
    *content = i + 2;
    return IMD_ULIST;
  }

  if (i < c1 - 1 && iMdCharIsByte(text, choff, i + 1, ' ') &&
      (iMdCharIsByte(text, choff, i, '-') || iMdCharIsByte(text, choff, i, '*') || iMdCharIsByte(text, choff, i, '+')))
  {
    *content = i + 2;
    return IMD_ULIST;
  }

  {
    int digits = i;
    while (digits < c1 && (choff[digits + 1] - choff[digits]) == 1 && iup_isdigit(text[choff[digits]]))
      digits++;

    if (digits > i && digits < c1 - 1 &&
        (iMdCharIsByte(text, choff, digits, '.') || iMdCharIsByte(text, choff, digits, ')')) &&
        iMdCharIsByte(text, choff, digits + 1, ' '))
    {
      *content = i;
      return IMD_OLIST;
    }
  }

  return IMD_PARA;
}

static void iMdEmitEscaped(iMdBuf* out, const char* str, int len, int line_start)
{
  int i;
  for (i = 0; i < len; i++)
  {
    char c = str[i];
    int escape = (c == '\\' || c == '`' || c == '*' || c == '_' || c == '[' || c == ']' || c == '~');

    if (line_start && i == 0 && (c == '#' || c == '>'))
      escape = 1;

    if (escape)
      iMdBufAppendChar(out, '\\');
    iMdBufAppendChar(out, c);
  }
}

static int iMdFmtEqual(const iMdFmt* a, const iMdFmt* b)
{
  if (a->bold != b->bold || a->italic != b->italic || a->strike != b->strike || a->mono != b->mono)
    return 0;
  if (a->link != b->link && !(a->link && b->link && iupStrEqual(a->link, b->link)))
    return 0;
  return 1;
}

static int iMdFmtHasMarkers(const iMdFmt* f, int heading, int quote)
{
  if (f->link || f->strike || f->mono)
    return 1;
  if (f->bold && !heading)
    return 1;
  if (f->italic && !quote)
    return 1;
  return 0;
}

static void iMdOpenMarkers(iMdBuf* out, const iMdFmt* f, int heading, int quote)
{
  if (f->link) iMdBufAppendChar(out, '[');
  if (f->bold && !heading) iMdBufAppendStr(out, "**");
  if (f->italic && !quote) iMdBufAppendChar(out, '*');
  if (f->strike) iMdBufAppendStr(out, "~~");
  if (f->mono) iMdBufAppendChar(out, '`');
}

static void iMdCloseMarkers(iMdBuf* out, const iMdFmt* f, int content_start, int heading, int quote)
{
  int spaces = 0;

  while (out->len - spaces > content_start && out->data[out->len - spaces - 1] == ' ')
    spaces++;

  out->len -= spaces;
  out->charlen -= spaces;
  out->data[out->len] = 0;

  if (f->mono) iMdBufAppendChar(out, '`');
  if (f->strike) iMdBufAppendStr(out, "~~");
  if (f->italic && !quote) iMdBufAppendChar(out, '*');
  if (f->bold && !heading) iMdBufAppendStr(out, "**");
  if (f->link)
  {
    iMdBufAppendStr(out, "](");
    iMdBufAppendStr(out, f->link);
    iMdBufAppendChar(out, ')');
  }

  while (spaces-- > 0)
    iMdBufAppendChar(out, ' ');
}

static void iMdEmitInline(iMdBuf* out, const char* text, const int* choff, const iMdFmt* fmt, int c0, int c1, int heading, int quote)
{
  iMdFmt cur;
  int open = 0, content_start = 0, first = 1, i;

  memset(&cur, 0, sizeof(iMdFmt));

  for (i = c0; i < c1; i++)
  {
    const iMdFmt* f = &fmt[i];
    const char* ch = text + choff[i];
    int blen = choff[i + 1] - choff[i];

    if (f->image)
    {
      if (open)
      {
        iMdCloseMarkers(out, &cur, content_start, heading, quote);
        open = 0;
      }
      iMdBufAppendStr(out, "![](");
      iMdBufAppendStr(out, f->image);
      iMdBufAppendChar(out, ')');
      first = 0;
      continue;
    }

    if (open && !iMdFmtEqual(&cur, f))
    {
      iMdCloseMarkers(out, &cur, content_start, heading, quote);
      open = 0;
    }

    if (!open)
    {
      if (iMdCharIsSpace(text, choff, i) && iMdFmtHasMarkers(f, heading, quote))
      {
        iMdBufAppend(out, ch, blen);
        continue;
      }
      cur = *f;
      iMdOpenMarkers(out, &cur, heading, quote);
      content_start = out->len;
      open = 1;
    }

    if (cur.mono)
      iMdBufAppend(out, ch, blen);
    else
      iMdEmitEscaped(out, ch, blen, first);

    first = 0;
  }

  if (open)
    iMdCloseMarkers(out, &cur, content_start, heading, quote);
}

char* iupMarkdownGetValue(Ihandle* ih)
{
  char* value;
  int* choff;
  iMdFmt* fmt;
  iMdBuf out;
  Ihandle* bulk;
  int blen, charlen, base_size = 0, i, c;
  int in_code = 0, prev_kind = IMD_BLANK;

  value = IupGetAttribute(ih, "VALUE");
  if (!value)
    value = "";

  blen = (int)strlen(value);
  charlen = iMdUtf8CharCount(value, blen);

  choff = (int*)malloc(((size_t)charlen + 1) * sizeof(int));
  fmt = (iMdFmt*)calloc((size_t)charlen + 1, sizeof(iMdFmt));
  if (!choff || !fmt)
  {
    if (choff) free(choff);
    if (fmt) free(fmt);
    return iupStrDup(value);
  }

  c = 0;
  for (i = 0; i <= blen; i++)
  {
    if (i == blen || (value[i] & 0xC0) != 0x80)
      choff[c++] = i;
  }

  if (!iupStrToInt(IupGetAttribute(ih, "FONTSIZE"), &base_size))
    base_size = 0;
  if (base_size < 0)
    base_size = -base_size;

  bulk = IupUser();
  IupSetAttribute(bulk, "BULK", "YES");

  if (iupdrvTextGetFormatTags(ih, bulk))
  {
    int count = IupGetChildCount(bulk);
    for (i = 0; i < count; i++)
      iMdApplyTag(IupGetChild(bulk, i), fmt, charlen, base_size);
  }

  iMdBufInit(&out);

  i = 0;
  while (i <= charlen)
  {
    int line_end = i;
    int kind, level, content;
    int trim;

    while (line_end < charlen && !iMdCharIsByte(value, choff, line_end, '\n'))
      line_end++;

    trim = line_end;
    if (trim > i && iMdCharIsByte(value, choff, trim - 1, '\r'))
      trim--;

    kind = iMdLineKind(value, choff, fmt, i, trim, &level, &content);

    if (in_code && kind != IMD_CODE)
    {
      iMdBufAppendStr(&out, "\n```");
      in_code = 0;
    }
    else if (!in_code && kind == IMD_CODE)
    {
      if (out.len > 0)
        iMdBufAppendChar(&out, '\n');
      iMdBufAppendStr(&out, "```");
      in_code = 1;
    }

    if (out.len > 0)
    {
      iMdBufAppendChar(&out, '\n');
      if (prev_kind == IMD_PARA && kind == IMD_PARA)
        iMdBufAppendChar(&out, '\n');
    }

    switch (kind)
    {
    case IMD_RULE:
      iMdBufAppendStr(&out, "---");
      break;
    case IMD_CODE:
      iMdBufAppend(&out, value + choff[i], choff[trim] - choff[i]);
      break;
    case IMD_HEADING:
      for (c = 0; c < level; c++)
        iMdBufAppendChar(&out, '#');
      iMdBufAppendChar(&out, ' ');
      iMdEmitInline(&out, value, choff, fmt, content, trim, 1, 0);
      break;
    case IMD_QUOTE:
      iMdBufAppendStr(&out, "> ");
      iMdEmitInline(&out, value, choff, fmt, content, trim, 0, 1);
      break;
    case IMD_ULIST:
      iMdBufAppendStr(&out, "- ");
      iMdEmitInline(&out, value, choff, fmt, content, trim, 0, 0);
      break;
    case IMD_OLIST:
      iMdEmitInline(&out, value, choff, fmt, content, trim, 0, 0);
      break;
    case IMD_BLANK:
      break;
    default:
      iMdEmitInline(&out, value, choff, fmt, i, trim, 0, 0);
      break;
    }

    prev_kind = kind;
    i = line_end + 1;
  }

  if (in_code)
    iMdBufAppendStr(&out, "\n```");

  IupDestroy(bulk);
  free(choff);
  free(fmt);

  return out.data;
}
