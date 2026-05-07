/** \file
 * \brief Tree Control - Android driver
 *
 * See Copyright Notice in iup.h
 */

#include <stdlib.h>
#include <stdint.h>

#include <jni.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_image.h"
#include "iup_tree.h"

#include "iupandroid_drv.h"


#define IUPANDROID_TREE_TYPEFACE_NORMAL      0
#define IUPANDROID_TREE_TYPEFACE_BOLD        1
#define IUPANDROID_TREE_TYPEFACE_ITALIC      2
#define IUPANDROID_TREE_TYPEFACE_BOLD_ITALIC 3
#define IUPANDROID_TREE_COMPLEX_UNIT_PX      0
#define IUPANDROID_TREE_COMPLEX_UNIT_SP      2

static jclass androidTreeFindClass(JNIEnv* env)
{
  jclass local = (*env)->FindClass(env, "io/github/gen2brain/iupgo/IupTreeHelper");
  iupAndroid_CheckException(env, "FindClass IupTreeHelper");
  return local;
}

static int androidTreeGetNodeKind(JNIEnv* env, jobject node_obj)
{
  if (!node_obj) return 0;
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeKind", "(Ljava/lang/Object;)I");
  jint k = (*env)->CallStaticIntMethod(env, cls, m, node_obj);
  iupAndroid_CheckException(env, "getNodeKind");
  (*env)->DeleteLocalRef(env, cls);
  return (int)k;
}

IUP_SDK_API void iupdrvTreeAddNode(Ihandle* ih, int prev_id, int kind, const char* title, int add)
{
  if (!ih->handle) return;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();

  jobject prev_obj = NULL;
  int kind_prev = 0;
  InodeHandle* inode_prev = iupTreeGetNode(ih, prev_id);

  if (!inode_prev && prev_id != -1) return;

  if (inode_prev)
  {
    prev_obj = (jobject)inode_prev;
    kind_prev = androidTreeGetNodeKind(env, prev_obj);
  }

  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls,
      "addNode", "(Landroid/view/View;Ljava/lang/Object;ILjava/lang/String;I)Ljava/lang/Object;");
  jstring j_title = title ? (*env)->NewStringUTF(env, title) : NULL;
  jobject new_local = (*env)->CallStaticObjectMethod(env, cls, m, ih->handle, prev_obj,
      (jint)kind, j_title, (jint)add);
  iupAndroid_CheckException(env, "IupTreeHelper.addNode");
  if (j_title) (*env)->DeleteLocalRef(env, j_title);
  (*env)->DeleteLocalRef(env, cls);

  if (!new_local) return;
  jobject new_global = (*env)->NewGlobalRef(env, new_local);
  (*env)->DeleteLocalRef(env, new_local);

  iupTreeAddToCache(ih, add, kind_prev, inode_prev, (InodeHandle*)new_global);
}

IUP_SDK_API int iupdrvTreeTotalChildCount(Ihandle* ih, InodeHandle* node_handle)
{
  (void)ih;
  if (!node_handle) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeTotalDescendants", "(Ljava/lang/Object;)I");
  jint n = (*env)->CallStaticIntMethod(env, cls, m, (jobject)node_handle);
  iupAndroid_CheckException(env, "getNodeTotalDescendants");
  (*env)->DeleteLocalRef(env, cls);
  return (int)n;
}

IUP_SDK_API InodeHandle* iupdrvTreeGetFocusNode(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  if (ih->data->node_count <= 0) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getFocusNodeId", "(Landroid/view/View;)I");
  jint id = (*env)->CallStaticIntMethod(env, cls, m, ih->handle);
  iupAndroid_CheckException(env, "getFocusNodeId");
  (*env)->DeleteLocalRef(env, cls);
  if (id < 0 || id >= ih->data->node_count) return ih->data->node_cache[0].node_handle;
  return ih->data->node_cache[id].node_handle;
}

IUP_SDK_API void iupdrvTreeUpdateMarkMode(Ihandle* ih)
{
  (void)ih;
}

static char* androidTreeCallNavInt(Ihandle* ih, int id, const char* method)
{
  if (!ih->handle) return NULL;
  if (id < 0 || id >= ih->data->node_count) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, "(Landroid/view/View;I)I");
  jint r = (*env)->CallStaticIntMethod(env, cls, m, ih->handle, (jint)id);
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
  if (r < 0) return NULL;
  return iupStrReturnInt((int)r);
}

static char* androidTreeGetParentIdAttrib(Ihandle* ih, int id)   { return androidTreeCallNavInt(ih, id, "getParentNodeId"); }
static char* androidTreeGetFirstIdAttrib(Ihandle* ih, int id)    { return androidTreeCallNavInt(ih, id, "getFirstNodeId"); }
static char* androidTreeGetLastIdAttrib(Ihandle* ih, int id)     { return androidTreeCallNavInt(ih, id, "getLastNodeId"); }
static char* androidTreeGetNextIdAttrib(Ihandle* ih, int id)     { return androidTreeCallNavInt(ih, id, "getNextNodeId"); }
static char* androidTreeGetPreviousIdAttrib(Ihandle* ih, int id) { return androidTreeCallNavInt(ih, id, "getPreviousNodeId"); }


/* refresh node_cache[start..start+count-1] from Java; COPY allocates fresh GlobalRefs */
static void androidTreeRefillCacheSlots(Ihandle* ih, int start, int count, JNIEnv* env)
{
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeAtIdForCache", "(Landroid/view/View;I)Ljava/lang/Object;");
  for (int i = 0; i < count; i++)
  {
    int id = start + i;
    jobject local = (*env)->CallStaticObjectMethod(env, cls, m, ih->handle, (jint)id);
    iupAndroid_CheckException(env, "getNodeAtIdForCache");
    if (local)
    {
      ih->data->node_cache[id].node_handle = (InodeHandle*)((*env)->NewGlobalRef(env, local));
      (*env)->DeleteLocalRef(env, local);
    }
  }
  (*env)->DeleteLocalRef(env, cls);
}

static int androidTreeCopyMoveCommon(Ihandle* ih, int id, const char* value, int is_copy)
{
  if (!ih->handle || !value) return 0;
  int dst_id;
  if (!iupStrToInt(value, &dst_id)) return 0;

  InodeHandle* src = iupTreeGetNode(ih, id);
  if (!src) return 0;
  int count = 1 + iupdrvTreeTotalChildCount(ih, src);

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, is_copy ? "copyNode" : "moveNode", "(Landroid/view/View;II)I");
  jint new_id = (*env)->CallStaticIntMethod(env, cls, m, ih->handle, (jint)id, (jint)dst_id);
  iupAndroid_CheckException(env, is_copy ? "copyNode" : "moveNode");
  (*env)->DeleteLocalRef(env, cls);
  if (new_id < 0) return 0;

  if (is_copy)
  {
    ih->data->node_count += count;
    iupTreeCopyMoveCache(ih, id, new_id, count, 1);
    androidTreeRefillCacheSlots(ih, new_id, count, env);
  }
  else
  {
    iupTreeCopyMoveCache(ih, id, new_id, count, 0);
  }
  return 0;
}

static int androidTreeSetMoveNodeAttrib(Ihandle* ih, int id, const char* value)
{
  return androidTreeCopyMoveCommon(ih, id, value, 0);
}

static int androidTreeSetCopyNodeAttrib(Ihandle* ih, int id, const char* value)
{
  return androidTreeCopyMoveCommon(ih, id, value, 1);
}

IUP_SDK_API void iupdrvTreeDragDropCopyNode(Ihandle* src, Ihandle* dst, InodeHandle* src_node, InodeHandle* dst_node)
{
  if (!src || !dst || !src->handle || !dst->handle || !src_node || !dst_node) return;

  int src_id = iupTreeFindNodeId(src, src_node);
  int dst_id = iupTreeFindNodeId(dst, dst_node);
  if (src_id < 0 || dst_id < 0) return;

  int count = 1 + iupdrvTreeTotalChildCount(src, src_node);
  int old_count = dst->data->node_count;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "copyNodeAcross", "(Landroid/view/View;ILandroid/view/View;I)I");
  jint new_id = (*env)->CallStaticIntMethod(env, cls, m, src->handle, (jint)src_id, dst->handle, (jint)dst_id);
  iupAndroid_CheckException(env, "IupTreeHelper.copyNodeAcross");
  (*env)->DeleteLocalRef(env, cls);
  if (new_id < 0) return;

  dst->data->node_count = old_count + count;
  iupTreeCopyMoveCache(dst, dst_id, new_id, count, 1);
  androidTreeRefillCacheSlots(dst, new_id, count, env);
}

static int androidTreeConvertXYToPos(Ihandle* ih, int x, int y)
{
  if (!ih->handle) return -1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "nodeIdAt", "(Landroid/view/View;II)I");
  jint id = (*env)->CallStaticIntMethod(env, cls, m, ih->handle, (jint)x, (jint)y);
  iupAndroid_CheckException(env, "IupTreeHelper.nodeIdAt");
  (*env)->DeleteLocalRef(env, cls);
  return (id < 0) ? -1 : (int)id;
}


static int androidTreeSetTitleIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeTitle",
      "(Landroid/view/View;Ljava/lang/Object;Ljava/lang/String;)V");
  jstring j_value = value ? (*env)->NewStringUTF(env, value) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jobject)h, j_value);
  iupAndroid_CheckException(env, "setNodeTitle");
  if (j_value) (*env)->DeleteLocalRef(env, j_value);
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static char* androidTreeGetTitleIdAttrib(Ihandle* ih, int id)
{
  if (!ih->handle) return NULL;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeTitle", "(Ljava/lang/Object;)Ljava/lang/String;");
  jstring js = (jstring)(*env)->CallStaticObjectMethod(env, cls, m, (jobject)h);
  iupAndroid_CheckException(env, "getNodeTitle");
  (*env)->DeleteLocalRef(env, cls);
  if (!js) return NULL;
  const char* s = (*env)->GetStringUTFChars(env, js, NULL);
  char* r = s ? iupStrReturnStr(s) : NULL;
  if (s) (*env)->ReleaseStringUTFChars(env, js, s);
  (*env)->DeleteLocalRef(env, js);
  return r;
}

static int androidTreeSetStateIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;
  int expanded = iupStrEqualNoCase(value, "EXPANDED") ? 1 : 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeExpanded",
      "(Landroid/view/View;Ljava/lang/Object;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jobject)h, (jboolean)expanded);
  iupAndroid_CheckException(env, "setNodeExpanded");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static char* androidTreeGetStateIdAttrib(Ihandle* ih, int id)
{
  if (!ih->handle) return NULL;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID mk = (*env)->GetStaticMethodID(env, cls, "getNodeKind", "(Ljava/lang/Object;)I");
  int kind = (*env)->CallStaticIntMethod(env, cls, mk, (jobject)h);
  if (kind != ITREE_BRANCH) { (*env)->DeleteLocalRef(env, cls); return NULL; }
  jmethodID me = (*env)->GetStaticMethodID(env, cls, "isNodeExpanded", "(Ljava/lang/Object;)Z");
  jboolean exp = (*env)->CallStaticBooleanMethod(env, cls, me, (jobject)h);
  iupAndroid_CheckException(env, "isNodeExpanded");
  (*env)->DeleteLocalRef(env, cls);
  return exp ? "EXPANDED" : "COLLAPSED";
}

static char* androidTreeGetKindIdAttrib(Ihandle* ih, int id)
{
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  return (androidTreeGetNodeKind(env, (jobject)h) == ITREE_LEAF) ? "LEAF" : "BRANCH";
}

static char* androidTreeGetDepthIdAttrib(Ihandle* ih, int id)
{
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeDepth", "(Ljava/lang/Object;)I");
  jint d = (*env)->CallStaticIntMethod(env, cls, m, (jobject)h);
  iupAndroid_CheckException(env, "getNodeDepth");
  (*env)->DeleteLocalRef(env, cls);
  return iupStrReturnInt((int)d);
}

static char* androidTreeGetChildCountIdAttrib(Ihandle* ih, int id)
{
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeChildCount", "(Ljava/lang/Object;)I");
  jint c = (*env)->CallStaticIntMethod(env, cls, m, (jobject)h);
  iupAndroid_CheckException(env, "getNodeChildCount");
  (*env)->DeleteLocalRef(env, cls);
  return iupStrReturnInt((int)c);
}

static char* androidTreeGetRootCountAttrib(Ihandle* ih)
{
  if (!ih->handle) return iupStrReturnInt(0);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getRootCount", "(Landroid/view/View;)I");
  jint r = (*env)->CallStaticIntMethod(env, cls, m, ih->handle);
  iupAndroid_CheckException(env, "getRootCount");
  (*env)->DeleteLocalRef(env, cls);
  return iupStrReturnInt((int)r);
}

static int androidTreeSetValueAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value) return 0;

  int cur = 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);

  if (iupStrEqualNoCase(value, "CLEAR"))
  {
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "setFocusNodeById", "(Landroid/view/View;I)V");
    (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)-1);
    iupAndroid_CheckException(env, "setFocusNodeById");
    (*env)->DeleteLocalRef(env, cls);
    return 0;
  }

  int id = -1;
  if (iupStrToInt(value, &id))
  {
    /* numeric id */
  }
  else
  {
    jmethodID mc = (*env)->GetStaticMethodID(env, cls, "getFocusNodeId", "(Landroid/view/View;)I");
    cur = (*env)->CallStaticIntMethod(env, cls, mc, ih->handle);
    iupAndroid_CheckException(env, "getFocusNodeId");
    jmethodID mn = (*env)->GetStaticMethodID(env, cls, "resolveNavId", "(Landroid/view/View;ILjava/lang/String;)I");
    jstring j_kw = (*env)->NewStringUTF(env, value);
    id = (*env)->CallStaticIntMethod(env, cls, mn, ih->handle, (jint)cur, j_kw);
    iupAndroid_CheckException(env, "resolveNavId");
    (*env)->DeleteLocalRef(env, j_kw);
  }

  if (id < 0 || id >= ih->data->node_count) { (*env)->DeleteLocalRef(env, cls); return 0; }

  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setFocusNodeById", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id);
  iupAndroid_CheckException(env, "setFocusNodeById");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetTopItemAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value) return 0;
  int id;
  if (!iupStrToInt(value, &id)) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "scrollToId", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id);
  iupAndroid_CheckException(env, "scrollToId");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetDelNodeAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;

  int old_count = ih->data->node_count;
  int start_id = 0;
  int is_children = (value && iupStrEqualNoCase(value, "CHILDREN"));
  int is_all = (value && iupStrEqualNoCase(value, "ALL"));
  int is_selected = (value && iupStrEqualNoCase(value, "SELECTED"));
  int is_marked = (value && iupStrEqualNoCase(value, "MARKED"));

  if (is_marked)
  {
    JNIEnv* env = iupAndroid_GetEnvThreadSafe();
    jclass cls = androidTreeFindClass(env);

    jmethodID m_top = (*env)->GetStaticMethodID(env, cls, "getMarkedTopLevelIdsDescending", "(Landroid/view/View;)[I");
    jintArray ids_arr = (jintArray)(*env)->CallStaticObjectMethod(env, cls, m_top, ih->handle);
    iupAndroid_CheckException(env, "getMarkedTopLevelIdsDescending");

    if (ids_arr)
    {
      jsize len = (*env)->GetArrayLength(env, ids_arr);
      jint* ids = (*env)->GetIntArrayElements(env, ids_arr, NULL);
      for (jsize k = 0; k < len; k++)
      {
        int marked_id = ids[k];
        InodeHandle* target = iupTreeGetNode(ih, marked_id);
        if (!target) continue;
        int count = 1 + iupdrvTreeTotalChildCount(ih, target);
        int n_count = ih->data->node_count;
        for (int j = marked_id; j < marked_id + count && j < n_count; j++)
        {
          jobject g = (jobject)ih->data->node_cache[j].node_handle;
          if (g) (*env)->DeleteGlobalRef(env, g);
        }
        ih->data->node_count -= count;
        iupTreeDelFromCache(ih, marked_id, count);
      }
      (*env)->ReleaseIntArrayElements(env, ids_arr, ids, JNI_ABORT);
      (*env)->DeleteLocalRef(env, ids_arr);
    }

    jmethodID m_del = (*env)->GetStaticMethodID(env, cls, "deleteNode", "(Landroid/view/View;ILjava/lang/String;)V");
    jstring j_scope = (*env)->NewStringUTF(env, "MARKED");
    (*env)->CallStaticVoidMethod(env, cls, m_del, ih->handle, (jint)-1, j_scope);
    iupAndroid_CheckException(env, "deleteNode");
    (*env)->DeleteLocalRef(env, j_scope);
    (*env)->DeleteLocalRef(env, cls);
    return 0;
  }

  int descendants = 0;
  InodeHandle* target = NULL;
  int resolve_id = -1;
  if (!is_all)
  {
    if (is_selected)
    {
      target = iupdrvTreeGetFocusNode(ih);
      if (target) resolve_id = iupTreeFindNodeId(ih, target);
    }
    else
    {
      target = iupTreeGetNode(ih, id);
      resolve_id = id;
    }
    if (!target) return 0;
    descendants = iupdrvTreeTotalChildCount(ih, target);
    start_id = is_children ? (resolve_id + 1) : resolve_id;
  }

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  if (is_all)
  {
    for (int i = 0; i < old_count; i++)
    {
      jobject g = (jobject)ih->data->node_cache[i].node_handle;
      if (g) (*env)->DeleteGlobalRef(env, g);
      ih->data->node_cache[i].node_handle = NULL;
    }
  }
  else
  {
    int count = is_children ? descendants : (descendants + 1);
    int del_start = is_children ? (resolve_id + 1) : resolve_id;
    for (int i = del_start; i < del_start + count && i < old_count; i++)
    {
      jobject g = (jobject)ih->data->node_cache[i].node_handle;
      if (g) (*env)->DeleteGlobalRef(env, g);
    }
  }

  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "deleteNode", "(Landroid/view/View;ILjava/lang/String;)V");
  jstring j_scope = value ? (*env)->NewStringUTF(env, value) : NULL;
  int resolved = is_selected ? resolve_id : id;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)resolved, j_scope);
  iupAndroid_CheckException(env, "deleteNode");
  if (j_scope) (*env)->DeleteLocalRef(env, j_scope);
  (*env)->DeleteLocalRef(env, cls);

  if (is_all)
  {
    ih->data->node_count = 0;
    iupAttribSet(ih, "LASTADDNODE", NULL);
  }
  else
  {
    int count = is_children ? descendants : (descendants + 1);
    ih->data->node_count = old_count - count;
    iupTreeDelFromCache(ih, start_id, count);
  }

  return 0;
}

static int androidTreeSetMarkedIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;
  int mark = iupStrBoolean(value);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeMarked", "(Landroid/view/View;IZ)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id, (jboolean)mark);
  iupAndroid_CheckException(env, "setNodeMarked");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static char* androidTreeGetMarkedIdAttrib(Ihandle* ih, int id)
{
  if (!ih->handle) return NULL;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "isNodeMarked", "(Ljava/lang/Object;Landroid/view/View;)Z");
  jboolean b = (*env)->CallStaticBooleanMethod(env, cls, m, (jobject)h, ih->handle);
  iupAndroid_CheckException(env, "isNodeMarked");
  (*env)->DeleteLocalRef(env, cls);
  return iupStrReturnBoolean((int)b);
}

static int androidTreeSetMarkAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);

  if (iupStrEqualNoCase(value, "CLEARALL"))
  {
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "markAll", "(Landroid/view/View;Z)V");
    (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)0);
    iupAndroid_CheckException(env, "markAll");
  }
  else if (iupStrEqualNoCase(value, "MARKALL"))
  {
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "markAll", "(Landroid/view/View;Z)V");
    (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)1);
    iupAndroid_CheckException(env, "markAll");
  }
  else if (iupStrEqualNoCase(value, "INVERTALL"))
  {
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "invertMarks", "(Landroid/view/View;)V");
    (*env)->CallStaticVoidMethod(env, cls, m, ih->handle);
    iupAndroid_CheckException(env, "invertMarks");
  }
  else if (iupStrEqualPartial(value, "BLOCK"))
  {
    int from_id = -1, to_id = -1;
    const char* p = value + 5;
    if (iupStrToIntInt(p, &from_id, &to_id, '-') == 2
        || iupStrToIntInt(p, &from_id, &to_id, ':') == 2)
    {
      jmethodID m = (*env)->GetStaticMethodID(env, cls, "markBlock", "(Landroid/view/View;II)V");
      (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)from_id, (jint)to_id);
      iupAndroid_CheckException(env, "markBlock");
    }
  }
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetMarkStartAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle || !value) return 0;
  int id;
  if (!iupStrToInt(value, &id)) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setMarkStart", "(Landroid/view/View;I)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id);
  iupAndroid_CheckException(env, "setMarkStart");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetMarkedNodesAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setMarkedNodesStr", "(Landroid/view/View;Ljava/lang/String;)V");
  jstring js = value ? (*env)->NewStringUTF(env, value) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, js);
  iupAndroid_CheckException(env, "setMarkedNodesStr");
  if (js) (*env)->DeleteLocalRef(env, js);
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static char* androidTreeGetMarkedNodesAttrib(Ihandle* ih)
{
  if (!ih->handle) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getMarkedNodesStr", "(Landroid/view/View;)Ljava/lang/String;");
  jstring js = (jstring)(*env)->CallStaticObjectMethod(env, cls, m, ih->handle);
  iupAndroid_CheckException(env, "getMarkedNodesStr");
  (*env)->DeleteLocalRef(env, cls);
  if (!js) return NULL;
  const char* s = (*env)->GetStringUTFChars(env, js, NULL);
  char* r = s ? iupStrReturnStr(s) : NULL;
  if (s) (*env)->ReleaseStringUTFChars(env, js, s);
  (*env)->DeleteLocalRef(env, js);
  return r;
}

static int androidTreeSetMarkModeAttrib(Ihandle* ih, const char* value)
{
  int multi = iupStrEqualNoCase(value, "MULTIPLE");
  ih->data->mark_mode = multi ? ITREE_MARK_MULTIPLE : ITREE_MARK_SINGLE;
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setMarkMultiple", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)multi);
  iupAndroid_CheckException(env, "setMarkMultiple");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTreeSetColorIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;
  unsigned char rr = 0, gg = 0, bb = 0;
  int r = -1, g = -1, b = -1;
  if (value && iupStrToRGB(value, &rr, &gg, &bb)) { r = rr; g = gg; b = bb; }

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeColor", "(Landroid/view/View;IIII)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(env, "setNodeColor");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static char* androidTreeGetColorIdAttrib(Ihandle* ih, int id)
{
  if (!ih->handle) return NULL;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNodeColorStr", "(Landroid/view/View;I)Ljava/lang/String;");
  jstring js = (jstring)(*env)->CallStaticObjectMethod(env, cls, m, ih->handle, (jint)id);
  iupAndroid_CheckException(env, "getNodeColorStr");
  (*env)->DeleteLocalRef(env, cls);
  if (!js) return NULL;
  const char* s = (*env)->GetStringUTFChars(env, js, NULL);
  char* r = s ? iupStrReturnStr(s) : NULL;
  if (s) (*env)->ReleaseStringUTFChars(env, js, s);
  (*env)->DeleteLocalRef(env, js);
  return r;
}

static int androidTreeSetTitleFontIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;

  const char* family = NULL;
  int style = IUPANDROID_TREE_TYPEFACE_NORMAL;
  int size_unit = IUPANDROID_TREE_COMPLEX_UNIT_SP;
  float size_value = 0.0f;

  char typeface[1024] = "";
  int size = 0, is_bold = 0, is_italic = 0, is_underline = 0, is_strikeout = 0;
  int parsed = 0;
  if (value && value[0])
    parsed = iupGetFontInfo(value, typeface, &size, &is_bold, &is_italic, &is_underline, &is_strikeout);

  if (parsed)
  {
    family = typeface[0] ? typeface : NULL;
    if (is_bold && is_italic) style = IUPANDROID_TREE_TYPEFACE_BOLD_ITALIC;
    else if (is_bold)         style = IUPANDROID_TREE_TYPEFACE_BOLD;
    else if (is_italic)       style = IUPANDROID_TREE_TYPEFACE_ITALIC;
    if (size < 0) { size_unit = IUPANDROID_TREE_COMPLEX_UNIT_PX; size_value = (float)(-size); }
    else          { size_unit = IUPANDROID_TREE_COMPLEX_UNIT_SP; size_value = (float)size; }
  }

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeFont",
      "(Landroid/view/View;ILjava/lang/String;IIF)V");
  jstring j_family = family ? (*env)->NewStringUTF(env, family) : NULL;
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id, j_family, (jint)style, (jint)size_unit, (jfloat)size_value);
  iupAndroid_CheckException(env, "setNodeFont");
  if (j_family) (*env)->DeleteLocalRef(env, j_family);
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetImageIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jobject bmp = value ? (jobject)iupImageGetImage(value, ih, 0, NULL) : NULL;
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeImage", "(Landroid/view/View;ILandroid/graphics/Bitmap;)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id, bmp);
  iupAndroid_CheckException(env, "setNodeImage");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetImageExpandedIdAttrib(Ihandle* ih, int id, const char* value)
{
  if (!ih->handle) return 0;
  InodeHandle* h = iupTreeGetNode(ih, id);
  if (!h) return 0;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jobject bmp = value ? (jobject)iupImageGetImage(value, ih, 0, NULL) : NULL;
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setNodeImageExpanded", "(Landroid/view/View;ILandroid/graphics/Bitmap;)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)id, bmp);
  iupAndroid_CheckException(env, "setNodeImageExpanded");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetDefaultImage(Ihandle* ih, const char* value, const char* method)
{
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jobject bmp = value ? (jobject)iupImageGetImage(value, ih, 0, NULL) : NULL;
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, method, "(Landroid/view/View;Landroid/graphics/Bitmap;)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, bmp);
  iupAndroid_CheckException(env, method);
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTreeSetImageLeafAttrib(Ihandle* ih, const char* value)
{
  return androidTreeSetDefaultImage(ih, value, "setDefaultImageLeaf");
}

static int androidTreeSetImageBranchCollapsedAttrib(Ihandle* ih, const char* value)
{
  return androidTreeSetDefaultImage(ih, value, "setDefaultImageBranchCollapsed");
}

static int androidTreeSetImageBranchExpandedAttrib(Ihandle* ih, const char* value)
{
  return androidTreeSetDefaultImage(ih, value, "setDefaultImageBranchExpanded");
}


static int androidTreeSetShowRenameAttrib(Ihandle* ih, const char* value)
{
  ih->data->show_rename = iupStrBoolean(value);
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setShowRename", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)ih->data->show_rename);
  iupAndroid_CheckException(env, "setShowRename");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTreeSetShowDragDropAttrib(Ihandle* ih, const char* value)
{
  ih->data->show_dragdrop = iupStrBoolean(value);
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setShowDragDrop", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)ih->data->show_dragdrop);
  iupAndroid_CheckException(env, "setShowDragDrop");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static char* androidTreeGetShowRenameAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->show_rename);
}

static int androidTreeSetRenameAttrib(Ihandle* ih, const char* value)
{
  (void)value;
  if (!ih->handle) return 0;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID mf = (*env)->GetStaticMethodID(env, cls, "getFocusNodeId", "(Landroid/view/View;)I");
  jint id = (*env)->CallStaticIntMethod(env, cls, mf, ih->handle);
  iupAndroid_CheckException(env, "getFocusNodeId");
  if (id >= 0 && id < ih->data->node_count)
  {
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "startRename", "(Landroid/view/View;I)V");
    (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, id);
    iupAndroid_CheckException(env, "startRename");
  }
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static char* androidTreeGetValueAttrib(Ihandle* ih)
{
  if (!ih->handle) return iupStrReturnInt(0);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "getFocusNodeId", "(Landroid/view/View;)I");
  jint id = (*env)->CallStaticIntMethod(env, cls, m, ih->handle);
  iupAndroid_CheckException(env, "getFocusNodeId");
  (*env)->DeleteLocalRef(env, cls);
  return iupStrReturnInt((int)id);
}

static int androidTreeSetExpandAllAttrib(Ihandle* ih, const char* value)
{
  if (!ih->handle) return 0;
  int exp = iupStrBoolean(value);
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "expandAll", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)exp);
  iupAndroid_CheckException(env, "expandAll");
  (*env)->DeleteLocalRef(env, cls);
  return 0;
}

static int androidTreeSetAddExpandedAttrib(Ihandle* ih, const char* value)
{
  int exp = iupStrBoolean(value);
  ih->data->add_expanded = exp;
  if (!ih->handle) return 1;
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setAddExpanded", "(Landroid/view/View;Z)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jboolean)exp);
  iupAndroid_CheckException(env, "setAddExpanded");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

static int androidTreeMapMethod(Ihandle* ih)
{
  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "createTree", "(J)Landroid/view/View;");
  jobject widget = (*env)->CallStaticObjectMethod(env, cls, m, (jlong)(intptr_t)ih);
  iupAndroid_CheckException(env, "IupTreeHelper.createTree");
  (*env)->DeleteLocalRef(env, cls);
  if (!widget) return IUP_ERROR;

  ih->handle = (jobject)((*env)->NewGlobalRef(env, widget));
  (*env)->DeleteLocalRef(env, widget);

  char* v = iupAttribGet(ih, "ADDEXPANDED");
  if (v) androidTreeSetAddExpandedAttrib(ih, v);

  if (iupAttribGetInt(ih, "ADDROOT"))
    iupdrvTreeAddNode(ih, -1, ITREE_BRANCH, "", 0);

  iupAndroid_AddWidgetToParent(env, ih);

  IupSetCallback(ih, "_IUP_XY2POS_CB", (Icallback)androidTreeConvertXYToPos);
  return IUP_NOERROR;
}

static void androidTreeUnMapMethod(Ihandle* ih)
{
  if (!ih || !ih->handle) return;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();

  /* Release all per-node global refs. */
  if (ih->data && ih->data->node_cache)
  {
    for (int i = 0; i < ih->data->node_count; i++)
    {
      jobject node_obj = (jobject)ih->data->node_cache[i].node_handle;
      if (node_obj) (*env)->DeleteGlobalRef(env, node_obj);
      ih->data->node_cache[i].node_handle = NULL;
    }
  }

  iupAndroid_RemoveFromParent(env, ih);
  iupAndroid_ReleaseIhandle(env, ih);
}

static int androidTreeSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b)) return 0;
  if (!ih->handle) return 1;

  JNIEnv* env = iupAndroid_GetEnvThreadSafe();
  jclass cls = androidTreeFindClass(env);
  jmethodID m = (*env)->GetStaticMethodID(env, cls, "setBgColor", "(Landroid/view/View;III)V");
  (*env)->CallStaticVoidMethod(env, cls, m, ih->handle, (jint)r, (jint)g, (jint)b);
  iupAndroid_CheckException(env, "IupTreeHelper.setBgColor");
  (*env)->DeleteLocalRef(env, cls);
  return 1;
}

IUP_SDK_API void iupdrvTreeAddBorders(Ihandle* ih, int *w, int *h)
{
  (void)ih;
  *w += iupAndroid_DpToPx(48);  /* chevron + leading icon + 1 indent step */
  *h += iupAndroid_DpToPx(8);
}

IUP_SDK_API void iupdrvTreeInitClass(Iclass* ic)
{
  ic->Map = androidTreeMapMethod;
  ic->UnMap = androidTreeUnMapMethod;

  iupClassRegisterAttribute(ic, "BGCOLOR", NULL, androidTreeSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "TXTBGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "ADDEXPANDED", NULL, androidTreeSetAddExpandedAttrib, IUPAF_SAMEASSYSTEM, "YES", IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "EXPANDALL", NULL, androidTreeSetExpandAllAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "VALUE", androidTreeGetValueAttrib, androidTreeSetValueAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ROOTCOUNT", androidTreeGetRootCountAttrib, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TOPITEM", NULL, androidTreeSetTopItemAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TITLE", androidTreeGetTitleIdAttrib, androidTreeSetTitleIdAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "STATE", androidTreeGetStateIdAttrib, androidTreeSetStateIdAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "KIND", androidTreeGetKindIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "DEPTH", androidTreeGetDepthIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "CHILDCOUNT", androidTreeGetChildCountIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "PARENT", androidTreeGetParentIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "FIRST", androidTreeGetFirstIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "LAST", androidTreeGetLastIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NEXT", androidTreeGetNextIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "PREVIOUS", androidTreeGetPreviousIdAttrib, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "MOVENODE", NULL, androidTreeSetMoveNodeAttrib, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "COPYNODE", NULL, androidTreeSetCopyNodeAttrib, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "DELNODE", NULL, androidTreeSetDelNodeAttrib, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "MARKED", androidTreeGetMarkedIdAttrib, androidTreeSetMarkedIdAttrib, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARK", NULL, androidTreeSetMarkAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "STARTING", NULL, androidTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKSTART", NULL, androidTreeSetMarkStartAttrib, NULL, NULL, IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKEDNODES", androidTreeGetMarkedNodesAttrib, androidTreeSetMarkedNodesAttrib, NULL, NULL, IUPAF_NO_SAVE|IUPAF_NO_DEFAULTVALUE|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARKMODE", NULL, androidTreeSetMarkModeAttrib, IUPAF_SAMEASSYSTEM, "SINGLE", IUPAF_NOT_MAPPED);

  iupClassRegisterAttributeId(ic, "COLOR", androidTreeGetColorIdAttrib, androidTreeSetColorIdAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TITLEFONT", NULL, androidTreeSetTitleFontIdAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGE", NULL, androidTreeSetImageIdAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "IMAGEEXPANDED", NULL, androidTreeSetImageExpandedIdAttrib, IUPAF_IHANDLENAME|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "IMAGELEAF", NULL, androidTreeSetImageLeafAttrib, IUPAF_SAMEASSYSTEM, "IMGLEAF", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHCOLLAPSED", NULL, androidTreeSetImageBranchCollapsedAttrib, IUPAF_SAMEASSYSTEM, "IMGCOLLAPSED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "IMAGEBRANCHEXPANDED", NULL, androidTreeSetImageBranchExpandedAttrib, IUPAF_SAMEASSYSTEM, "IMGEXPANDED", IUPAF_IHANDLENAME|IUPAF_NO_INHERIT);

  iupClassRegisterReplaceAttribFunc(ic, "SHOWRENAME", androidTreeGetShowRenameAttrib, androidTreeSetShowRenameAttrib);
  iupClassRegisterAttribute(ic, "RENAME", NULL, androidTreeSetRenameAttrib, NULL, NULL, IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "SHOWDRAGDROP", NULL, androidTreeSetShowDragDropAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
