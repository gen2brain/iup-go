/** \file
 * \brief Markup Parser - IUP Markup Subset to Intermediate Representation
 *
 * Parses a Pango-like markup subset into styled text runs that
 * platform-specific drivers can render using their native APIs.
 *
 * Supported tags:
 *   <b>, <i>, <u>, <s>                - bold, italic, underline, strikethrough
 *   <sub>, <sup>                      - subscript, superscript
 *   <big>, <small>                    - relative size increase/decrease
 *   <span foreground="..." background="..." font_family="..." font_size="..." font_weight="..." font_style="...">
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_MARKUP_H
#define __IUP_MARKUP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ImarkupRun {
  char* text;           /* UTF-8 text for this run (allocated) */
  int bold;             /* 1 = bold */
  int italic;           /* 1 = italic */
  int underline;        /* 1 = underline */
  int strikethrough;    /* 1 = strikethrough */
  int superscript;      /* nesting level (0 = none, 1 = one <sup>, etc.) */
  int subscript;        /* nesting level */
  int big;              /* nesting level (each level ~1.2x) */
  int small_size;       /* nesting level (each level ~0.83x) */
  char* fg_color;       /* foreground color string or NULL (allocated) */
  char* bg_color;       /* background color string or NULL (allocated) */
  char* font_family;    /* font family name or NULL (allocated) */
  int font_size;        /* font size in points, 0 = default */
  int font_weight;      /* 0=default, values like 400=normal, 700=bold */
  int font_style;       /* 0=default, 1=italic, 2=oblique */
} ImarkupRun;

typedef struct _ImarkupData {
  ImarkupRun* runs;
  int count;
  int alloc;
} ImarkupData;

/* Parse IUP markup string into styled runs.
   Returns NULL on error (e.g. malformed markup).
   Caller must free with iupMarkupFree(). */
IUP_SDK_API ImarkupData* iupMarkupParse(const char* markup);

/* Free parsed markup data */
IUP_SDK_API void iupMarkupFree(ImarkupData* data);

/* Strip markup tags and return plain text.
   Returns allocated string, caller must free(). */
IUP_SDK_API char* iupMarkupStripTags(const char* markup);

/* Convert IUP markup string to HTML (for Qt).
   Returns allocated string, caller must free(). */
IUP_SDK_API char* iupMarkupToHtml(const char* markup);

/* Convert IUP markup string to EFL textblock markup.
   Returns allocated string, caller must free(). */
IUP_SDK_API char* iupMarkupToEfl(const char* markup);

/* Convert IUP markup string to Pango markup (for GTK).
   Fixes font_size values from points to Pango format ("Npt").
   Returns allocated string, caller must free(). */
IUP_SDK_API char* iupMarkupToPango(const char* markup);

#ifdef __cplusplus
}
#endif

#endif
