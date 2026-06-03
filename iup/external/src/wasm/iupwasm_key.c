/** \file
 * \brief WebAssembly Keyboard Mapping
 *
 * See Copyright Notice in "iup.h"
 */

#include "iup.h"

#include "iup_drv.h"


/* No native key-event injection in a browser; core's key path runs entirely from the JS keydown handler. */
IUP_SDK_API void iupdrvKeyEncode(int key, unsigned int *keyval, unsigned int *state)
{
  (void)key;
  if (keyval) *keyval = 0;
  if (state) *state = 0;
}
