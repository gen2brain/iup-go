/** \file
 * \brief WebAssembly/Emscripten Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWASM_DRV_H
#define __IUPWASM_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iup.h"


int iupwasmIdOf(Ihandle* ih);
void iupwasmDialogModalShow(Ihandle* ih, int on);
void iupwasmInstallTheme(void);
int iupwasmJsIsDarkMode(void);
const char* iupwasmThemeColorVar(unsigned char r, unsigned char g, unsigned char b, int is_bg);

void iupwasmRegisterHandle(int id, Ihandle* ih);
void iupwasmUnregisterHandle(int id);
void iupwasmSetVisibleState(int id, int visible);
Ihandle* iupwasmHandleFromId(int id);
void iupwasmFillStatus(char* status, int mods);

void iupwasmAddToParent(Ihandle* ih);

int  iupwasmJsCreate(const char* tag);
void iupwasmJsDestroy(int id);
void iupwasmJsAddToBody(int id);
void iupwasmJsAddChild(int parent, int child);
void iupwasmJsSetPos(int id, int x, int y, int w, int h);
void iupwasmJsSetText(int id, const char* txt);
void iupwasmJsSetVisible(int id, int visible);
void iupwasmJsSetBgColor(int id, int r, int g, int b);
void iupwasmJsSetFgColor(int id, int r, int g, int b);
void iupwasmJsWireClick(int id);
void iupwasmJsDialogStyle(int id);
void iupwasmJsSetDocTitle(const char* txt);
void iupwasmJsInstallProxy(void);
void iupwasmJsInstallKeyHandler(void);
void iupwasmJsLabelStyle(int id);
void iupwasmJsFrameStyle(int id);
void iupwasmJsFrameSetTitle(int id, const char* txt);
int  iupwasmJsCreateToggle(int isRadio, int group);
void iupwasmJsToggleSetText(int id, const char* txt);
void iupwasmJsToggleSetValue(int id, int on);
void iupwasmJsToggleImageMode(int id);
void iupwasmJsToggleSwitchMode(int id);
int  iupwasmJsToggleGetValue(int id);
void iupwasmJsToggleWire(int id);
void iupwasmJsSetImage(int elId, int imgId);
void iupwasmJsSetInactiveImage(int elId, int imgId, int isBg);
void iupwasmJsButtonStyle(int id);
void iupwasmJsButtonAlign(int id, const char* css);
void iupwasmJsButtonSetText(int id, const char* txt);
void iupwasmJsButtonSetImage(int id, int imgId, int gap);
void iupwasmJsButtonSetImpress(int id, int imgId);
void iupwasmJsButtonDefault(int id, int on);
void iupwasmJsButtonFlat(int id);
void iupwasmJsSetMarkup(int id, const char* txt);
void iupwasmJsInstallMarkup(void);
void iupwasmJsSetPadding(int id, int h, int v);
void iupwasmJsSetTextAlign(int id, const char* align);

int  iupwasmJsCreateDate(void);
void iupwasmJsDateSet(int id, const char* iso);
int  iupwasmJsDateGet(int id);

void iupwasmJsListDndSource(int id, int on);
void iupwasmJsListDndTarget(int id, int on);
void iupwasmJsTreeDndSource(int id, int on);
void iupwasmJsTreeDndTarget(int id, int on);

char* iupwasmStripMnemonic(const char* title);

const char* iupwasmCssAlign(const char* iupalign);

void iupwasmFontToCss(const char* iupfont, char* css, int csslen);

void iupwasmCanvasRedraw(Ihandle* ih);
void iupwasmJsCanvasBlit(int cid);


#ifdef __cplusplus
}
#endif

#endif
