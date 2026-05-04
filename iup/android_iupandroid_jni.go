//go:build android

package iup

// JNI bridge sources (Java -> C entry points). One cgo unit includes them
// all so adding a new bridge does not require a matching Go shim.

/*
#include "external/src/android/iupandroid_jnicacheglobals.c"
#include "external/src/android/iupandroid_jni_IupActivity.c"
#include "external/src/android/iupandroid_jni_IupApplication.c"
#include "external/src/android/iupandroid_jni_IupButton.c"
#include "external/src/android/iupandroid_jni_IupCalendar.c"
#include "external/src/android/iupandroid_jni_IupCanvas.c"
#include "external/src/android/iupandroid_jni_IupCommon.c"
#include "external/src/android/iupandroid_jni_IupDatePick.c"
#include "external/src/android/iupandroid_jni_IupIdleHelper.c"
#include "external/src/android/iupandroid_jni_IupLaunchActivity.c"
#include "external/src/android/iupandroid_jni_IupList.c"
#include "external/src/android/iupandroid_jni_IupMenu.c"
#include "external/src/android/iupandroid_jni_IupNotify.c"
#include "external/src/android/iupandroid_jni_IupPostMessage.c"
#include "external/src/android/iupandroid_jni_IupScrollbar.c"
#include "external/src/android/iupandroid_jni_IupTable.c"
#include "external/src/android/iupandroid_jni_IupTabs.c"
#include "external/src/android/iupandroid_jni_IupText.c"
#include "external/src/android/iupandroid_jni_IupToggle.c"
#include "external/src/android/iupandroid_jni_IupTree.c"
#include "external/src/android/iupandroid_jni_IupVal.c"
*/
import "C"
