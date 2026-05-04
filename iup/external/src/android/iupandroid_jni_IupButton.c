/** \file
 * \brief JNI bridge: IupButtonHelper (BUTTON_CB dispatch).
 *
 * See Copyright Notice in "iup.h"
 */

#include <jni.h>
#include <stdint.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_key.h"

#include "iupandroid_drv.h"


/* IUP_BUTTON1..3 are char codes '1'..'3', not int 1..3. */
static int androidButtonToIupButton(int button)
{
  switch (button)
  {
    case 1: return IUP_BUTTON1;
    case 2: return IUP_BUTTON2;
    case 3: return IUP_BUTTON3;
    default: return IUP_BUTTON1;
  }
}

JNIEXPORT void JNICALL Java_io_github_gen2brain_iupgo_IupButtonHelper_dispatchButton(
    JNIEnv* jni_env, jclass cls, jlong ihandle_ptr,
    jint button, jint pressed, jint x, jint y)
{
  (void)jni_env;
  (void)cls;
  Ihandle* ih = (Ihandle*)(intptr_t)ihandle_ptr;
  if (!ih) return;

  IFniiiis cb = (IFniiiis)IupGetCallback(ih, "BUTTON_CB");
  if (!cb) return;

  char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
  if (pressed) iupKEY_SETBUTTON1(status);

  int ret = cb(ih, androidButtonToIupButton((int)button), (int)pressed, (int)x, (int)y, status);
  if (ret == IUP_CLOSE) IupExitLoop();
}
