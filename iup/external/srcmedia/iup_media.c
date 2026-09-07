/** \file
 * \brief Media controls
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"
#include "iupmedia.h"

#include "iup_object.h"
#include "iup_register.h"
#include "iup_media.h"

IUPMEDIA_API int IupMediaOpen(void)
{
  if (!IupIsOpened())
    return IUP_ERROR;

  if (IupGetGlobal("_IUP_MEDIA_OPEN"))
    return IUP_OPENED;

  iupRegisterClass(iupAudioNewClass());
  iupRegisterClass(iupCameraNewClass());

  IupSetGlobal("_IUP_MEDIA_OPEN", "1");
  return IUP_NOERROR;
}

IUPMEDIA_API Ihandle *IupAudio(void)
{
  return IupCreate("audio");
}
