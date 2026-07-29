/** \file
 * \brief WebAssembly IupFileDlg
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <emscripten.h>

#include "iup.h"

#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"


/* SAVE: a browser can't pick a save path, so prompt for a name and let the caller download */
EM_JS(int, iupwasmJsFilePrompt, (const char* title, const char* def), {
  var t = UTF8ToString(title), d = UTF8ToString(def);
  var res = (typeof window === 'undefined') ? globalThis.__iupReadSync({ op: 'prompt', msg: t, def: d }) : window.prompt(t, d);
  if (res === null) return 0;
  var len = lengthBytesUTF8(res) + 1, p = _malloc(len);
  stringToUTF8(res, p, len);
  return p;
})

/* OPEN on a Worker: block on the main thread's <input type=file>; returns byte size (-1 if canceled), name into namePtr, bytes pulled over the SAB */
EM_JS(int, iupwasmJsFileOpenStart, (int namePtr, int nameMax, const char* accept), {
  var name = globalThis.__iupReadSync({ op: 'fileopen', accept: UTF8ToString(accept) });
  if (name === null) return -1;
  stringToUTF8(name, namePtr, nameMax);
  return globalThis.__iupReadSync({ op: 'fileopensize' });
})

EM_JS(int, iupwasmJsFileOpenChunk, (int off, int len, int destPtr), {
  return globalThis.__iupReadBytes({ op: 'fileopenchunk', off: off, len: len }, HEAPU8, destPtr);
})

EM_JS(int, iupwasmJsIsWorker, (void), { return (typeof document === 'undefined') ? 1 : 0; })

static void wasmFileDlgBuildAccept(Ihandle* ih, char* accept, int accept_size)
{
  char* extfilter = iupAttribGet(ih, "EXTFILTER");
  char* filter = extfilter ? extfilter : iupAttribGet(ih, "FILTER");
  int is_ext = (extfilter != NULL);
  int part = 0, len = 0;
  const char* p;

  accept[0] = 0;
  if (!filter)
    return;

  for (p = filter; ; p++)
  {
    if (*p == '*' && *(p + 1) == '.' && (!is_ext || (part % 2) == 1))
    {
      const char* ext = p + 1;
      int n = 0;
      while (ext[n] && ext[n] != ';' && ext[n] != '|' && ext[n] != ' ')
        n++;
      if (n == 2 && ext[1] == '*')
      {
        accept[0] = 0;
        return;
      }
      if (len + n + 2 < accept_size)
      {
        if (len)
          accept[len++] = ',';
        memcpy(accept + len, ext, n);
        len += n;
        accept[len] = 0;
      }
      p += n;
    }
    if (*p == '|')
      part++;
    if (!*p)
      break;
  }
}

/* write the picked file into MEMFS so C fopen() can read it; VALUE is that path */
static void wasmFileDlgOpen(Ihandle* ih)
{
  char name[1024] = "";
  char path[1100];
  char accept[512];
  int size;

  wasmFileDlgBuildAccept(ih, accept, sizeof(accept));
  size = iupwasmJsFileOpenStart((int)(intptr_t)name, sizeof(name), accept);

  if (size < 0)
  {
    iupAttribSet(ih, "STATUS", "-1");
    iupAttribSet(ih, "VALUE", NULL);
    return;
  }

  snprintf(path, sizeof(path), "/%s", name[0] ? name : "openfile");
  if (size > 0)
  {
    FILE* f = fopen(path, "wb");
    if (f)
    {
      int chunk = 65536, off = 0;
      unsigned char* buf = malloc(chunk);
      while (off < size)
      {
        int want = (size - off < chunk) ? (size - off) : chunk;
        int n = iupwasmJsFileOpenChunk(off, want, (int)(intptr_t)buf);
        if (n <= 0) break;
        fwrite(buf, 1, n, f);
        off += n;
      }
      fclose(f);
      free(buf);
    }
  }

  iupAttribSetStr(ih, "VALUE", path);
  iupAttribSet(ih, "STATUS", "0");
}

static int wasmFileDlgPopup(Ihandle* ih, int x, int y)
{
  char* dtype = iupAttribGetStr(ih, "DIALOGTYPE");
  char* title = iupAttribGet(ih, "TITLE");
  char* file = iupAttribGet(ih, "FILE");
  int is_save = iupStrEqualNoCase(dtype, "SAVE");
  char* res;
  (void)x;
  (void)y;

  if (iupStrEqualNoCase(dtype, "DIR"))
  {
    iupAttribSet(ih, "STATUS", "-1");  /* no directory picker in a browser */
    return IUP_NOERROR;
  }

  if (!is_save && iupwasmJsIsWorker())
  {
    wasmFileDlgOpen(ih);
    return IUP_NOERROR;
  }

  if (!title)
    title = is_save ? (char*)"Save file name:" : (char*)"Open file name:";

  res = (char*)(intptr_t)iupwasmJsFilePrompt(title, file ? file : "");
  if (!res)
  {
    iupAttribSet(ih, "STATUS", "-1");
    iupAttribSet(ih, "VALUE", NULL);
    return IUP_NOERROR;
  }

  iupAttribSetStr(ih, "VALUE", res);
  iupAttribSet(ih, "STATUS", is_save ? "1" : "0");
  free(res);
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvFileDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = wasmFileDlgPopup;

  iupClassRegisterAttribute(ic, "EXTFILTER", NULL, NULL, NULL, NULL, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "FILTERINFO", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "FILTERUSED", NULL, NULL, NULL, NULL, IUPAF_NOT_SUPPORTED | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
}
