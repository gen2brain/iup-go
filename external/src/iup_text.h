/** \file
 * \brief Text Control (not exported API)
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_TEXT_H 
#define __IUP_TEXT_H

#ifdef __cplusplus
extern "C" {
#endif


void iupdrvTextInitClass(Iclass* ic);
void iupdrvTextAddBorders(Ihandle* ih, int *w, int *h);
void iupdrvTextAddSpin(Ihandle* ih, int *w, int h);
void* iupdrvTextAddFormatTagStartBulk(Ihandle* ih);
void iupdrvTextAddFormatTagStopBulk(Ihandle* ih, void* state);
void iupdrvTextAddFormatTag(Ihandle* ih, Ihandle* formattag, int bulk);
void iupdrvTextConvertLinColToPos(Ihandle* ih, int lin, int col, int *pos);
void iupdrvTextConvertPosToLinCol(Ihandle* ih, int pos, int *lin, int *col);

/* Used by List and Text, implemented in Text */
int iupEditCallActionCb(Ihandle* ih, IFnis cb, const char* insert_value, int start, int end, void *mask, int nc, int remove_dir, int utf8);

void iupTextUpdateFormatTags(Ihandle* ih);

char* iupTextGetPaddingAttrib(Ihandle* ih);
char* iupTextGetNCAttrib(Ihandle* ih);
int iupTextSetFormattingAttrib(Ihandle* ih, const char* value);
char* iupTextGetFormattingAttrib(Ihandle* ih);
int iupTextSetAddFormatTagAttrib(Ihandle* ih, const char* value);
int iupTextSetAddFormatTagHandleAttrib(Ihandle* ih, const char* value);

struct _IcontrolData 
{
  int is_multiline,
      has_formatting,
      append_newline,
      disable_callbacks,
      nc,
      sb,                           /* scrollbar configuration, can be changed only before map */
      horiz_padding, vert_padding,  /* button margin */
      last_caret_pos;
  Iarray* formattags;
  Imask* mask;
};


#ifdef __cplusplus
}
#endif

#endif
