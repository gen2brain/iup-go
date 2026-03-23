/** \file
 * \brief Markup Parser - IUP Markup Subset to Intermediate Representation
 *
 * Parses a Pango-like markup subset into styled text runs.
 * Also provides string-to-string converters for HTML (Qt) and EFL markup.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "iup.h"
#include "iup_str.h"
#include "iup_markup.h"

#define IUP_MARKUP_MAX_DEPTH 32

typedef struct _ImarkupStyle {
  int bold;
  int italic;
  int underline;
  int strikethrough;
  int superscript;
  int subscript;
  int big;
  int small_size;
  char* fg_color;
  char* bg_color;
  char* font_family;
  int font_size;
  int font_weight;
  int font_style;
} ImarkupStyle;

typedef struct _ImarkupStyleStack {
  ImarkupStyle styles[IUP_MARKUP_MAX_DEPTH];
  int top;
} ImarkupStyleStack;

static void iMarkupStyleInit(ImarkupStyle* style)
{
  memset(style, 0, sizeof(ImarkupStyle));
}

static void iMarkupStyleCopy(ImarkupStyle* dst, const ImarkupStyle* src)
{
  dst->bold = src->bold;
  dst->italic = src->italic;
  dst->underline = src->underline;
  dst->strikethrough = src->strikethrough;
  dst->superscript = src->superscript;
  dst->subscript = src->subscript;
  dst->big = src->big;
  dst->small_size = src->small_size;
  dst->font_size = src->font_size;
  dst->font_weight = src->font_weight;
  dst->font_style = src->font_style;
  dst->fg_color = src->fg_color ? iupStrDup(src->fg_color) : NULL;
  dst->bg_color = src->bg_color ? iupStrDup(src->bg_color) : NULL;
  dst->font_family = src->font_family ? iupStrDup(src->font_family) : NULL;
}

static void iMarkupStyleFreeStrings(ImarkupStyle* style)
{
  if (style->fg_color) { free(style->fg_color); style->fg_color = NULL; }
  if (style->bg_color) { free(style->bg_color); style->bg_color = NULL; }
  if (style->font_family) { free(style->font_family); style->font_family = NULL; }
}

static void iMarkupAddRun(ImarkupData* data, const char* text, int text_len, const ImarkupStyle* style)
{
  ImarkupRun* run;

  if (text_len <= 0)
    return;

  if (data->count >= data->alloc)
  {
    int new_alloc = data->alloc * 2;
    if (new_alloc < 8) new_alloc = 8;
    data->runs = (ImarkupRun*)realloc(data->runs, new_alloc * sizeof(ImarkupRun));
    data->alloc = new_alloc;
  }

  run = &data->runs[data->count];
  memset(run, 0, sizeof(ImarkupRun));

  run->text = (char*)malloc(text_len + 1);
  memcpy(run->text, text, text_len);
  run->text[text_len] = '\0';

  run->bold = style->bold;
  run->italic = style->italic;
  run->underline = style->underline;
  run->strikethrough = style->strikethrough;
  run->superscript = style->superscript;
  run->subscript = style->subscript;
  run->big = style->big;
  run->small_size = style->small_size;
  run->font_size = style->font_size;
  run->font_weight = style->font_weight;
  run->font_style = style->font_style;
  run->fg_color = style->fg_color ? iupStrDup(style->fg_color) : NULL;
  run->bg_color = style->bg_color ? iupStrDup(style->bg_color) : NULL;
  run->font_family = style->font_family ? iupStrDup(style->font_family) : NULL;

  data->count++;
}

static const char* iMarkupSkipSpaces(const char* p)
{
  while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;
  return p;
}

static const char* iMarkupParseAttrValue(const char* p, char* value, int max_len)
{
  int i = 0;
  char quote;

  p = iMarkupSkipSpaces(p);
  if (*p != '=')
    return NULL;
  p++;
  p = iMarkupSkipSpaces(p);

  if (*p == '"' || *p == '\'')
  {
    quote = *p;
    p++;
    while (*p && *p != quote && i < max_len - 1)
      value[i++] = *p++;
    if (*p == quote)
      p++;
  }
  else
  {
    while (*p && *p != ' ' && *p != '>' && *p != '/' && i < max_len - 1)
      value[i++] = *p++;
  }
  value[i] = '\0';
  return p;
}

static const char* iMarkupParseSpanAttrs(const char* p, ImarkupStyle* style)
{
  char name[64];
  char value[256];

  while (*p && *p != '>' && *p != '/')
  {
    int i = 0;

    p = iMarkupSkipSpaces(p);
    if (*p == '>' || *p == '/')
      break;

    while (*p && *p != '=' && *p != ' ' && *p != '>' && i < 63)
      name[i++] = *p++;
    name[i] = '\0';

    if (*p == '=')
    {
      p = iMarkupParseAttrValue(p, value, sizeof(value));
      if (!p)
        return NULL;

      if (iupStrEqualNoCase(name, "foreground") || iupStrEqualNoCase(name, "color") || iupStrEqualNoCase(name, "fgcolor"))
      {
        if (style->fg_color) free(style->fg_color);
        style->fg_color = iupStrDup(value);
      }
      else if (iupStrEqualNoCase(name, "background") || iupStrEqualNoCase(name, "bgcolor"))
      {
        if (style->bg_color) free(style->bg_color);
        style->bg_color = iupStrDup(value);
      }
      else if (iupStrEqualNoCase(name, "font_family") || iupStrEqualNoCase(name, "face"))
      {
        if (style->font_family) free(style->font_family);
        style->font_family = iupStrDup(value);
      }
      else if (iupStrEqualNoCase(name, "font_size") || iupStrEqualNoCase(name, "size"))
      {
        style->font_size = atoi(value);
      }
      else if (iupStrEqualNoCase(name, "font_weight") || iupStrEqualNoCase(name, "weight"))
      {
        if (iupStrEqualNoCase(value, "bold"))
          style->font_weight = 700;
        else if (iupStrEqualNoCase(value, "normal"))
          style->font_weight = 400;
        else
          style->font_weight = atoi(value);
      }
      else if (iupStrEqualNoCase(name, "font_style") || iupStrEqualNoCase(name, "style"))
      {
        if (iupStrEqualNoCase(value, "italic"))
          style->font_style = 1;
        else if (iupStrEqualNoCase(value, "oblique"))
          style->font_style = 2;
        else
          style->font_style = 0;
      }
    }
    else
    {
      p = iMarkupSkipSpaces(p);
    }
  }

  return p;
}

static int iMarkupDecodeEntity(const char* p, const char** end)
{
  if (strncmp(p, "&amp;", 5) == 0) { *end = p + 5; return '&'; }
  if (strncmp(p, "&lt;", 4) == 0) { *end = p + 4; return '<'; }
  if (strncmp(p, "&gt;", 4) == 0) { *end = p + 4; return '>'; }
  if (strncmp(p, "&quot;", 6) == 0) { *end = p + 6; return '"'; }
  if (strncmp(p, "&apos;", 6) == 0) { *end = p + 6; return '\''; }
  return 0;
}

IUP_SDK_API ImarkupData* iupMarkupParse(const char* markup)
{
  ImarkupData* data;
  ImarkupStyleStack stack;
  ImarkupStyle current;
  const char* p;
  char text_buf[4096];
  int text_len = 0;

  if (!markup || !markup[0])
    return NULL;

  data = (ImarkupData*)calloc(1, sizeof(ImarkupData));
  stack.top = 0;
  iMarkupStyleInit(&current);

  p = markup;
  while (*p)
  {
    if (*p == '<')
    {
      int is_closing = 0;
      char tag_name[64];
      int i = 0;

      /* flush accumulated text */
      if (text_len > 0)
      {
        iMarkupAddRun(data, text_buf, text_len, &current);
        text_len = 0;
      }

      p++;
      if (*p == '/')
      {
        is_closing = 1;
        p++;
      }

      while (*p && *p != '>' && *p != ' ' && *p != '/' && i < 63)
        tag_name[i++] = *p++;
      tag_name[i] = '\0';

      if (is_closing)
      {
        /* closing tag, pop style */
        while (*p && *p != '>')
          p++;
        if (*p == '>')
          p++;

        if (stack.top > 0)
        {
          stack.top--;
          iMarkupStyleFreeStrings(&current);
          iMarkupStyleCopy(&current, &stack.styles[stack.top]);
          iMarkupStyleFreeStrings(&stack.styles[stack.top]);
        }
      }
      else
      {
        /* opening tag, push style */
        if (stack.top < IUP_MARKUP_MAX_DEPTH)
        {
          iMarkupStyleInit(&stack.styles[stack.top]);
          iMarkupStyleCopy(&stack.styles[stack.top], &current);
          stack.top++;
        }

        if (iupStrEqualNoCase(tag_name, "b"))
          current.bold = 1;
        else if (iupStrEqualNoCase(tag_name, "i"))
          current.italic = 1;
        else if (iupStrEqualNoCase(tag_name, "u"))
          current.underline = 1;
        else if (iupStrEqualNoCase(tag_name, "s"))
          current.strikethrough = 1;
        else if (iupStrEqualNoCase(tag_name, "sub"))
          current.subscript++;
        else if (iupStrEqualNoCase(tag_name, "sup"))
          current.superscript++;
        else if (iupStrEqualNoCase(tag_name, "big"))
          current.big++;
        else if (iupStrEqualNoCase(tag_name, "small"))
          current.small_size++;
        else if (iupStrEqualNoCase(tag_name, "span"))
        {
          p = iMarkupSkipSpaces(p);
          p = iMarkupParseSpanAttrs(p, &current);
          if (!p)
          {
            iupMarkupFree(data);
            iMarkupStyleFreeStrings(&current);
            /* free remaining stack entries */
            while (stack.top > 0)
            {
              stack.top--;
              iMarkupStyleFreeStrings(&stack.styles[stack.top]);
            }
            return NULL;
          }
        }

        /* skip self-closing slash */
        p = iMarkupSkipSpaces(p);
        if (*p == '/')
          p++;

        /* skip closing '>' */
        while (*p && *p != '>')
          p++;
        if (*p == '>')
          p++;
      }
    }
    else if (*p == '&')
    {
      const char* end;
      int ch = iMarkupDecodeEntity(p, &end);
      if (ch)
      {
        if (text_len < (int)sizeof(text_buf) - 1)
          text_buf[text_len++] = (char)ch;
        p = end;
      }
      else
      {
        if (text_len < (int)sizeof(text_buf) - 1)
          text_buf[text_len++] = *p;
        p++;
      }
    }
    else
    {
      if (text_len < (int)sizeof(text_buf) - 1)
        text_buf[text_len++] = *p;
      p++;
    }
  }

  /* flush remaining text */
  if (text_len > 0)
    iMarkupAddRun(data, text_buf, text_len, &current);

  iMarkupStyleFreeStrings(&current);
  while (stack.top > 0)
  {
    stack.top--;
    iMarkupStyleFreeStrings(&stack.styles[stack.top]);
  }

  if (data->count == 0)
  {
    iupMarkupFree(data);
    return NULL;
  }

  return data;
}

IUP_SDK_API void iupMarkupFree(ImarkupData* data)
{
  int i;
  if (!data)
    return;

  for (i = 0; i < data->count; i++)
  {
    ImarkupRun* run = &data->runs[i];
    if (run->text) free(run->text);
    if (run->fg_color) free(run->fg_color);
    if (run->bg_color) free(run->bg_color);
    if (run->font_family) free(run->font_family);
  }

  if (data->runs)
    free(data->runs);

  free(data);
}


IUP_SDK_API char* iupMarkupStripTags(const char* markup)
{
  ImarkupData* data;
  int total_len = 0;
  int i;
  char* result;
  char* p;

  data = iupMarkupParse(markup);
  if (!data)
    return iupStrDup(markup);

  for (i = 0; i < data->count; i++)
    total_len += (int)strlen(data->runs[i].text);

  result = (char*)malloc(total_len + 1);
  p = result;

  for (i = 0; i < data->count; i++)
  {
    int len = (int)strlen(data->runs[i].text);
    memcpy(p, data->runs[i].text, len);
    p += len;
  }
  *p = '\0';

  iupMarkupFree(data);
  return result;
}

/* ---- HTML converter (for Qt) ---- */

static void iMarkupAppendStr(char** buf, int* len, int* alloc, const char* str)
{
  int slen = (int)strlen(str);
  if (*len + slen >= *alloc)
  {
    int new_alloc = (*alloc) * 2;
    if (new_alloc < *len + slen + 64) new_alloc = *len + slen + 64;
    *buf = (char*)realloc(*buf, new_alloc);
    *alloc = new_alloc;
  }
  memcpy(*buf + *len, str, slen);
  *len += slen;
  (*buf)[*len] = '\0';
}

static void iMarkupAppendHtmlEscaped(char** buf, int* len, int* alloc, const char* text)
{
  const char* p = text;
  while (*p)
  {
    switch (*p)
    {
    case '&': iMarkupAppendStr(buf, len, alloc, "&amp;"); break;
    case '<': iMarkupAppendStr(buf, len, alloc, "&lt;"); break;
    case '>': iMarkupAppendStr(buf, len, alloc, "&gt;"); break;
    case '"': iMarkupAppendStr(buf, len, alloc, "&quot;"); break;
    default:
    {
      if (*len + 1 >= *alloc)
      {
        int new_alloc = (*alloc) * 2;
        *buf = (char*)realloc(*buf, new_alloc);
        *alloc = new_alloc;
      }
      (*buf)[*len] = *p;
      (*len)++;
      (*buf)[*len] = '\0';
      break;
    }
    }
    p++;
  }
}

IUP_SDK_API char* iupMarkupToHtml(const char* markup)
{
  ImarkupData* data;
  char* html = NULL;
  int html_len = 0;
  int html_alloc = 0;
  int i;
  char tmp[256];

  data = iupMarkupParse(markup);
  if (!data)
    return iupStrDup(markup);

  html = (char*)malloc(256);
  html[0] = '\0';
  html_alloc = 256;

  for (i = 0; i < data->count; i++)
  {
    ImarkupRun* run = &data->runs[i];
    int close_count = 0;
    char* close_tags[16];

    /* open style tags */
    if (run->font_family || run->font_size || run->fg_color || run->bg_color ||
        run->font_weight || run->font_style || run->big || run->small_size)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<span style=\"");

      if (run->font_family)
      {
        snprintf(tmp, sizeof(tmp), "font-family:%s;", run->font_family);
        iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
      }
      if (run->font_size)
      {
        snprintf(tmp, sizeof(tmp), "font-size:%dpt;", run->font_size);
        iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
      }
      else if (run->big || run->small_size)
      {
        int net = run->big - run->small_size;
        if (net > 0)
        {
          snprintf(tmp, sizeof(tmp), "font-size:larger;");
          iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
        }
        else if (net < 0)
        {
          snprintf(tmp, sizeof(tmp), "font-size:smaller;");
          iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
        }
      }
      if (run->font_weight)
      {
        snprintf(tmp, sizeof(tmp), "font-weight:%d;", run->font_weight);
        iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
      }
      if (run->font_style == 1)
        iMarkupAppendStr(&html, &html_len, &html_alloc, "font-style:italic;");
      else if (run->font_style == 2)
        iMarkupAppendStr(&html, &html_len, &html_alloc, "font-style:oblique;");
      if (run->fg_color)
      {
        snprintf(tmp, sizeof(tmp), "color:%s;", run->fg_color);
        iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
      }
      if (run->bg_color)
      {
        snprintf(tmp, sizeof(tmp), "background-color:%s;", run->bg_color);
        iMarkupAppendStr(&html, &html_len, &html_alloc, tmp);
      }

      iMarkupAppendStr(&html, &html_len, &html_alloc, "\">");
      close_tags[close_count++] = "</span>";
    }

    if (run->bold)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<b>");
      close_tags[close_count++] = "</b>";
    }
    if (run->italic)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<i>");
      close_tags[close_count++] = "</i>";
    }
    if (run->underline)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<u>");
      close_tags[close_count++] = "</u>";
    }
    if (run->strikethrough)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<s>");
      close_tags[close_count++] = "</s>";
    }
    if (run->superscript)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<sup>");
      close_tags[close_count++] = "</sup>";
    }
    if (run->subscript)
    {
      iMarkupAppendStr(&html, &html_len, &html_alloc, "<sub>");
      close_tags[close_count++] = "</sub>";
    }

    iMarkupAppendHtmlEscaped(&html, &html_len, &html_alloc, run->text);

    /* close in reverse order */
    while (close_count > 0)
    {
      close_count--;
      iMarkupAppendStr(&html, &html_len, &html_alloc, close_tags[close_count]);
    }
  }

  iupMarkupFree(data);
  return html;
}

/* ---- Pango markup converter (for GTK) ---- */

IUP_SDK_API char* iupMarkupToPango(const char* markup)
{
  char* result;
  int result_len = 0;
  int result_alloc = 0;
  const char* p;
  int in_tag = 0;

  if (!markup || !markup[0])
    return iupStrDup("");

  result = (char*)malloc(256);
  result[0] = '\0';
  result_alloc = 256;

  p = markup;
  while (*p)
  {
    if (*p == '<')
      in_tag = 1;
    else if (*p == '>')
      in_tag = 0;

    if (in_tag && (strncmp(p, "font_size=", 10) == 0 || strncmp(p, "size=", 5) == 0))
    {
      int is_font_size = (p[0] == 'f');
      char value[64];
      int vi = 0;
      char quote = 0;
      int needs_pt = 0;
      int j;

      if (is_font_size)
        p += 10;
      else
        p += 5;

      iMarkupAppendStr(&result, &result_len, &result_alloc, is_font_size ? "font_size=" : "size=");

      if (*p == '"' || *p == '\'')
      {
        quote = *p;
        p++;
      }

      while (*p && ((quote && *p != quote) || (!quote && *p != ' ' && *p != '>' && *p != '/')) && vi < 63)
        value[vi++] = *p++;
      value[vi] = '\0';

      /* check if value is a bare number (needs "pt" suffix for Pango) */
      if (vi > 0 && isdigit((unsigned char)value[0]))
      {
        needs_pt = 1;
        for (j = 0; j < vi; j++)
        {
          if (!isdigit((unsigned char)value[j]))
          {
            needs_pt = 0;
            break;
          }
        }
      }

      if (quote)
      {
        char q[2] = { quote, '\0' };
        iMarkupAppendStr(&result, &result_len, &result_alloc, q);
      }

      iMarkupAppendStr(&result, &result_len, &result_alloc, value);

      if (needs_pt)
        iMarkupAppendStr(&result, &result_len, &result_alloc, "pt");

      if (quote)
      {
        char q[2] = { quote, '\0' };
        iMarkupAppendStr(&result, &result_len, &result_alloc, q);
        if (*p == quote)
          p++;
      }
    }
    else
    {
      if (result_len + 1 >= result_alloc)
      {
        result_alloc *= 2;
        result = (char*)realloc(result, result_alloc);
      }
      result[result_len++] = *p;
      result[result_len] = '\0';
      p++;
    }
  }

  return result;
}

/* ---- EFL textblock markup converter ---- */

static void iMarkupAppendEflEscaped(char** buf, int* len, int* alloc, const char* text)
{
  const char* p = text;
  while (*p)
  {
    switch (*p)
    {
    case '<': iMarkupAppendStr(buf, len, alloc, "&lt;"); break;
    case '>': iMarkupAppendStr(buf, len, alloc, "&gt;"); break;
    case '&': iMarkupAppendStr(buf, len, alloc, "&amp;"); break;
    default:
    {
      if (*len + 1 >= *alloc)
      {
        int new_alloc = (*alloc) * 2;
        *buf = (char*)realloc(*buf, new_alloc);
        *alloc = new_alloc;
      }
      (*buf)[*len] = *p;
      (*len)++;
      (*buf)[*len] = '\0';
      break;
    }
    }
    p++;
  }
}

IUP_SDK_API char* iupMarkupToEfl(const char* markup)
{
  ImarkupData* data;
  char* efl = NULL;
  int efl_len = 0;
  int efl_alloc = 0;
  int i;
  char tmp[256];

  data = iupMarkupParse(markup);
  if (!data)
    return iupStrDup(markup);

  efl = (char*)malloc(256);
  efl[0] = '\0';
  efl_alloc = 256;

  for (i = 0; i < data->count; i++)
  {
    ImarkupRun* run = &data->runs[i];
    int close_count = 0;
    char* close_tags[16];

    /* EFL uses tags like <b>, <i>, and inline attributes like font_size=N */

    /* span-like attributes via EFL format string */
    if (run->font_family || run->font_size || run->fg_color || run->bg_color ||
        run->font_weight || run->font_style || run->big || run->small_size)
    {
      int has_attr = 0;
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<");

      if (run->fg_color)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        snprintf(tmp, sizeof(tmp), "color=%s", run->fg_color);
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, tmp);
        has_attr = 1;
      }
      if (run->bg_color)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        snprintf(tmp, sizeof(tmp), "backing=on backing_color=%s", run->bg_color);
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, tmp);
        has_attr = 1;
      }
      if (run->font_family)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        snprintf(tmp, sizeof(tmp), "font=%s", run->font_family);
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, tmp);
        has_attr = 1;
      }
      if (run->font_size)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        snprintf(tmp, sizeof(tmp), "font_size=%d", run->font_size);
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, tmp);
        has_attr = 1;
      }
      else if (run->big || run->small_size)
      {
        int net = run->big - run->small_size;
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        if (net > 0)
          iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "font_size=16");
        else if (net < 0)
          iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "font_size=10");
        has_attr = 1;
      }
      if (run->font_weight == 700)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "font_weight=Bold");
        has_attr = 1;
      }
      else if (run->font_weight && run->font_weight != 400)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        snprintf(tmp, sizeof(tmp), "font_weight=%d", run->font_weight);
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, tmp);
        has_attr = 1;
      }
      if (run->font_style == 1)
      {
        if (has_attr) iMarkupAppendStr(&efl, &efl_len, &efl_alloc, " ");
        iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "font_style=Italic");
        has_attr = 1;
      }

      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, ">");
      close_tags[close_count++] = "</>";
    }

    if (run->bold)
    {
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<b>");
      close_tags[close_count++] = "</b>";
    }
    if (run->italic)
    {
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<i>");
      close_tags[close_count++] = "</i>";
    }
    if (run->underline)
    {
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<underline=on>");
      close_tags[close_count++] = "</underline>";
    }
    if (run->strikethrough)
    {
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<strikethrough=on>");
      close_tags[close_count++] = "</strikethrough>";
    }
    if (run->superscript)
    {
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<rise=5>");
      close_tags[close_count++] = "</rise>";
    }
    if (run->subscript)
    {
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, "<rise=-5>");
      close_tags[close_count++] = "</rise>";
    }

    iMarkupAppendEflEscaped(&efl, &efl_len, &efl_alloc, run->text);

    while (close_count > 0)
    {
      close_count--;
      iMarkupAppendStr(&efl, &efl_len, &efl_alloc, close_tags[close_count]);
    }
  }

  iupMarkupFree(data);
  return efl;
}
