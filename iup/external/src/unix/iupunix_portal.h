/** \file
 * \brief XDG Desktop Portal Support - FileChooser, OpenURI and Settings
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
static int iupUnixPortalSettingsOpen(void) { return -1; }
static void iupUnixPortalSettingsClose(void) { }
static int iupUnixPortalSettingsDispatch(void) { return 0; }
static int iupUnixPortalGetDarkMode(int fallback) { return fallback; }
static void iupUnixPortalSetAccentColor(void) { }

#else

int iupUnixPortalAvailable(void);
int iupUnixPortalFileDialog(Ihandle* ih);
int iupUnixPortalHelp(const char* url);

int iupUnixPortalSettingsOpen(void);
void iupUnixPortalSettingsClose(void);
int iupUnixPortalSettingsDispatch(void);
int iupUnixPortalGetDarkMode(int fallback);
void iupUnixPortalSetAccentColor(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
