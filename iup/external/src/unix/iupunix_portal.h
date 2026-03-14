/** \file
 * \brief XDG Desktop Portal Support - FileChooser and OpenURI
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPUNIX_PORTAL_H
#define __IUPUNIX_PORTAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iup.h"

#if defined(_WIN32) || defined(__APPLE__)

static int iupUnixPortalAvailable(void) { return 0; }
static int iupUnixPortalFileDialog(Ihandle* ih) { (void)ih; return IUP_ERROR; }
static int iupUnixPortalHelp(const char* url) { (void)url; return -1; }

#else

int iupUnixPortalAvailable(void);
int iupUnixPortalFileDialog(Ihandle* ih);
int iupUnixPortalHelp(const char* url);

#endif

#ifdef __cplusplus
}
#endif

#endif
