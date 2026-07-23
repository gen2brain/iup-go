/** \file
 * \brief Android Driver
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPANDROID_DRV_H
#define __IUPANDROID_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

#include "iup.h"
#include "iup_export.h"


IUP_SDK_API JNIEnv* iupAndroid_GetEnvThreadSafe(void);

/* logs + clears any pending exception; pending exceptions abort the process at the next JNI call */
IUP_SDK_API void iupAndroid_CheckException(JNIEnv* jni_env, const char* tag);

IUP_SDK_API void iupAndroid_RetainIhandle(JNIEnv* jni_env, jobject native_widget, Ihandle* ih);
IUP_SDK_API void iupAndroid_ReleaseIhandle(JNIEnv* jni_env, Ihandle* ih);

IUP_SDK_API jobject iupAndroid_GetApplication(JNIEnv* jni_env);
IUP_SDK_API jobject iupAndroid_GetCurrentActivity(JNIEnv* jni_env);

/* normalises the TYPE_VOID -1 sentinel to NULL */
IUP_SDK_API jobject iupAndroid_RealNativeHandle(Ihandle* ih);

IUP_SDK_API void iupAndroid_AddWidgetToParent(JNIEnv* jni_env, Ihandle* ih);

/* inverse of AddWidgetToParent; does not release ih->handle */
IUP_SDK_API void iupAndroid_RemoveFromParent(JNIEnv* jni_env, Ihandle* ih);

/* iupStrReturnStr(jstring) helper; handles NULL and releases JNI refs */
IUP_SDK_API char* iupAndroid_JStringToReturnStr(JNIEnv* jni_env, jstring j_string);

/* re-seeds DLG/TXT/MENU color defaults from the current UI_MODE_NIGHT flag */
IUP_SDK_API void iupAndroidUpdateGlobalColors(void);

IUP_SDK_API int iupandroidKeyDecode(int keycode, int meta_state);

/* replays Android-only Dialog attributes once the Activity exists; called from IupActivity.onCreate */
IUP_SDK_API void iupAndroid_DialogActivityCreated(Ihandle* ih);

IUP_SDK_API float iupAndroid_GetDisplayDensity(void);

IUP_SDK_API int   iupAndroid_DpToPx(float dp);
IUP_SDK_API float iupAndroid_PxToDp(int px);

/* empty MaterialButton overhead in px, cached */
IUP_SDK_API void  iupAndroid_GetButtonBorderSize(int* w, int* h);

/* declarations live here so widget _IcontrolData stays out of the header */
IUP_SDK_API void  iupAndroid_ScrollbarDispatch(Ihandle* ih, int op, double value);
IUP_SDK_API void  iupAndroid_ToggleActionFromJava(Ihandle* ih, int state);

/* returns ih->data->mask without exposing the widget _IcontrolData */
IUP_SDK_API void* iupAndroidTextGetMask(Ihandle* ih);
IUP_SDK_API void* iupAndroidListGetMask(Ihandle* ih);

IUP_SDK_API void  iupAndroid_MenuAttachActivity(Ihandle* dlg, Ihandle* menu);
IUP_SDK_API void  iupAndroid_DrawerAttachActivity(Ihandle* dlg, Ihandle* drawer_menu);
IUP_SDK_API void  iupAndroid_DrawerDetachActivity(Ihandle* dlg);

/* mutex-serialised IupOpen: Go init and Java IupEntry both reach IupOpen */
IUP_SDK_API void  iupAndroid_OpenInit(void);
IUP_SDK_API int   iupAndroid_OpenOnce(void);

/* clear sticky main-loop flags so each app launch starts fresh */
IUP_SDK_API void  iupAndroid_BeforeEntry(void);

/* C side guards against the spurious no-change notification Spinner emits after setSelection */
IUP_SDK_API void  iupAndroidListDispatchSelection(Ihandle* ih, int item);
IUP_SDK_API void  iupAndroidListDispatchDoubleClick(Ihandle* ih, int item);
IUP_SDK_API void  iupAndroidListDispatchMultiSelection(Ihandle* ih, int* pos, int count);
IUP_SDK_API void  iupAndroidListDispatchDragDrop(Ihandle* ih, int drag_id, int drop_id);
IUP_SDK_API int   iupAndroidTableRowDragDrop(Ihandle* ih, int from, int to);

/* invoked by IupIdleHelper.queueIdle; returns 1 to keep, 0 to remove (CLOSE also calls IupExitLoop) */
IUP_SDK_API int   iupAndroid_DispatchIdle(void);


#ifdef __cplusplus
}
#endif

#endif
