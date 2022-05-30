/** \file
 * \brief Ihandle Object Definition
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_OBJECT_H 
#define __IUP_OBJECT_H

#include <stdarg.h>
#include "iup_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup object Ihandle Object
 * \par
 * Object handle for all the elements.
 * \par
 * See \ref iup_object.h
 * \ingroup cpi */


/* SIZE to RASTERSIZE
 * \ingroup object */
#define iupWIDTH2RASTER(_w, _cw) iupRound((_w * _cw)/4.0)
/* SIZE to RASTERSIZE
 * \ingroup object */
#define iupHEIGHT2RASTER(_h, _ch) iupRound((_h * _ch)/8.0)

/* RASTERSIZE to SIZE
 * \ingroup object */
#define iupRASTER2WIDTH(_w, _cw) iupRound((_w * 4.0)/_cw)
/* RASTERSIZE to SIZE
 * \ingroup object */
#define iupRASTER2HEIGHT(_h, _ch) iupRound((_h * 8.0)/_ch)


/** Expand configuration
 * \ingroup object */
enum Iexpand {
  IUP_EXPAND_NONE = 0x00,
  IUP_EXPAND_H0   = 0x01,  /* only set by IupFill */
  IUP_EXPAND_H1   = 0x02,
  IUP_EXPAND_W0   = 0x04,  /* only set by IupFill */
  IUP_EXPAND_W1   = 0x08,
  IUP_EXPAND_HFREE = 0x10,  /* expand to the available space only, container is not affect */
  IUP_EXPAND_WFREE = 0x20,
};

/** Expand configuration
 * \ingroup object */
#define IUP_EXPAND_WIDTH (IUP_EXPAND_W1 | IUP_EXPAND_W0)
/** Expand configuration
 * \ingroup object */
#define IUP_EXPAND_HEIGHT (IUP_EXPAND_H1 | IUP_EXPAND_H0)
/** Expand configuration
 * \ingroup object */
#define IUP_EXPAND_BOTH (IUP_EXPAND_WIDTH | IUP_EXPAND_HEIGHT)


/** A simple definition that do not depends on the native system, 
   but helps a lot when writing native code. See \ref iup_object.h for definitions.
 * \ingroup object */
#if defined(GTK_MAJOR_VERSION)
typedef struct _GtkWidget InativeHandle;
#elif defined(XmVERSION)
typedef struct _WidgetRec InativeHandle;
#elif defined(WINVER)
typedef struct HWND__ InativeHandle;
#elif defined(__APPLE__)
//#import <CoreFoundation/CoreFoundation.h>
/* Both id and CFTypeRef are already pointers. */
typedef void InativeHandle;
#elif defined(__ANDROID__)
//#include <jni.h>
/* jobject already includes the pointer. Use _jobject instead. */
struct _jobject;
typedef struct _jobject InativeHandle;
#elif defined(__EMSCRIPTEN__)
#include <stdint.h>
#define IUP_EMSCRIPTEN_MAX_COMPOUND_ELEMENTS 2
struct InativeHandleEmscripten
{
  int handleID;
  _Bool isCompound;
  int numElemsIfCompound; /* only set if compound; otherwise 0 */
  int32_t compoundHandleIDArray[IUP_EMSCRIPTEN_MAX_COMPOUND_ELEMENTS];
};
typedef struct InativeHandleEmscripten InativeHandle;
#else
typedef struct _InativeHandle InativeHandle;
#endif

/** Each control may define its own structure in its private module.
 * \ingroup object */
typedef struct _IcontrolData IcontrolData;
/** IcontrolData allocation utility.
 * \ingroup object */
#define iupALLOCCTRLDATA() ((IcontrolData*)calloc(1, sizeof(IcontrolData)))

/** General flags.
 * \ingroup object */
enum Iflags {
  IUP_FLOATING         = 0x01,   /**< is a floating element. FLOATING=Yes */
  IUP_FLOATING_IGNORE  = 0x02,   /**< is a floating element. FLOATING=Ignore. Do not compute layout. */
  IUP_MAXSIZE     = 0x04,   /**< has the MAXSIZE attribute set */
  IUP_MINSIZE     = 0x08,   /**< has the MAXSIZE attribute set */
  IUP_INTERNAL    = 0x10    /**< it is an internal element of the container */
};


/** Structure used by all the elements.
 * \ingroup object */
struct Ihandle_
{
  char sig[4];           /**< IUP Signature, initialized with "IUP", cleared on destroy */
  Iclass* iclass;        /**< Ihandle Class */
  Itable* attrib;        /**< attributes table */
  int serial;            /**< serial number used for controls that need a numeric id, initialized with -1 */
  InativeHandle* handle; /**< native handle. initialized when mapped. InativeHandle definition is system dependent. */
  int expand;            /**< expand configuration, a combination of \ref Iexpand, for containers is a combination of the children expand's */
  int flags;             /**< flags configuration, a combination of \ref Iflags */
  int x, y;              /**< upper-left corner relative to the native parent. always 0 for the dialog. */
  int    userwidth,    userheight; /**< user defined size for the control using SIZE or RASTERSIZE */
  int naturalwidth, naturalheight; /**< the calculated size based in the control contents and the user size */
  int currentwidth, currentheight; /**< actual size of the control in pixels (window size, including decorations and margins). */
  Ihandle* parent;       /**< previous control in the hierarchy tree */
  Ihandle* firstchild;   /**< first child control in the hierarchy tree */
  Ihandle* brother;      /**< next control inside parent */
  IcontrolData* data;    /**< private control data. automatically freed if not NULL in destroy */
};


/* Creates an object initializes iclass and nativetype.
 * Called only from IupCreate and IupLoad. */
IUP_SDK_API Ihandle* iupObjectCreate(Iclass* ic, void** params);


/** Utility that returns an array of parameters. Must call free for the returned value after usage.
 * Used by the creation functions of objects that receives a NULL terminated array of parameters.
 * \ingroup object */
IUP_SDK_API void** iupObjectGetParamList(void* first, va_list arglist);
 
/** Checks if the handle is still valid based on the signature.
 * But if the handle was destroyed still can access invalid memory.
 * \ingroup object */
IUP_SDK_API int iupObjectCheck(Ihandle* ih);


/* Other functions declared in <iup.h> and implemented here. 
IupCreate 
IupCreatev
IupCreatep
IupDestroy
*/


#ifdef __cplusplus
}
#endif

#endif
