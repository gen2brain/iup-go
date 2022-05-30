/** \file
 * \brief Windows Driver Core
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>             
#include <stdlib.h>

#include "iup.h"

#include "iup_str.h"
#include "iup_globalattrib.h"
#include "iup_names.h"
#include "iup_func.h"
#include "iup_drv.h"
#include "iup_drvinfo.h"
#include "iup_drvfont.h"
#include "iup_predialogs.h"
#include "iup_class.h"
#include "iup_register.h"
#include "iup_key.h"
#include "iup_image.h"
#include "iup_dlglist.h"
#include "iup_assert.h"
#include "iup_strmessage.h"


static int iup_opened = 0;
static int iup_dummy_argc = 0;
static char** iup_dummy_argv = {0};

IUP_API int IupIsOpened(void)
{
  return iup_opened;
}

IUP_API int IupOpen(int *argc, char ***argv)
{
  if (iup_opened)
    return IUP_OPENED;
  iup_opened = 1;

  if (!argc || !(*argc) || !argv)
  {
    argc = &iup_dummy_argc;
    argv = &iup_dummy_argv;
  }

  iupNamesInit();
  iupFuncInit();
  iupStrMessageInit();
  iupGlobalAttribInit(); 
  iupRegisterInit();
  iupKeyInit();
  iupImageStockInit();

  IupSetLanguage("ENGLISH");
  IupSetGlobal("VERSION", IupVersion());
  IupSetGlobal("COPYRIGHT",  IUP_COPYRIGHT);

  if (iupdrvOpen(argc, argv) == IUP_NOERROR)
  {
    char* value;

    iupdrvFontInit();

    IupStoreGlobal("SYSTEM", iupdrvGetSystemName());
    IupStoreGlobal("SYSTEMVERSION", iupdrvGetSystemVersion());
    IupStoreGlobal("COMPUTERNAME", iupdrvGetComputerName());
    IupStoreGlobal("USERNAME", iupdrvGetUserName());

    IupSetGlobal("DEFAULTFONT", iupdrvGetSystemFont());  /* Use SetGlobal because iupdrvGetSystemFont returns a static string */
    IupSetGlobal("DEFAULTPRECISION", "2");
    IupSetGlobal("DEFAULTBUTTONPADDING", "12x4");  /* used by pre-defined dialogs */

    iupRegisterInternalClasses();

    value = getenv("IUP_QUIET");
    if (value && !iupStrBoolean(value)) /* if not defined do NOT print */
      printf("IUP %s %s\n", IupVersion(), IUP_COPYRIGHT);

    value = getenv("IUP_VERSION");
    if (iupStrBoolean(value))
      IupVersionShow();

    return IUP_NOERROR;
  }
  else
  {
#ifdef  IUP_ASSERT
    /* can not use pre-defined dialogs here, so only output to console. */
    fprintf(stderr, "IUP ERROR: IupOpen failed.\n");
#endif
    return IUP_ERROR;
  }
}

IUP_API void IupClose(void)
{
  if (!iup_opened)
    return;
  iup_opened = 0;

  iupdrvSetIdleFunction(NULL);  /* stop any idle */

  iupDlgListDestroyAll();    /* destroy all dialogs and their children */
  iupNamesDestroyHandles();  /* destroy everything that do not belong to a dialog */
  iupImageStockFinish();     /* release stock images hash table and the images */

  iupRegisterFinish();  /* release native classes */

  iupdrvFontFinish();   /* release font cache */
  iupdrvClose();        /* release native handles and allocated memory */

  iupGlobalAttribFinish();  /* release global hash table */
  iupStrMessageFinish();    /* release messages hash table */
  iupFuncFinish();          /* release callbacks hash table */
  iupNamesFinish();         /* release names hash table */

  iupStrGetMemory(-1); /* Frees internal buffer */
}
