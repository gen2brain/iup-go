/** \file
 * \brief Ihandle Class Interface
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_CLASS_H 
#define __IUP_CLASS_H

#include "iup_table.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \defgroup iclass Ihandle Class
 * \par
 * See \ref iup_class.h
 * \ingroup cpi */

/** Known native types.
 * \ingroup iclass */
typedef enum _InativeType {
  IUP_TYPEVOID,     /**< No native representation - HBOX, VBOX, ZBOX, FILL, RADIO (handle==(void*)-1 always) */
  IUP_TYPECONTROL,  /**< Native controls - BUTTON, LABEL, TOGGLE, LIST, TEXT, MULTILINE, FRAME, others */
  IUP_TYPECANVAS,   /**< Drawing canvas, also used as a base control for custom controls. */ 
  IUP_TYPEDIALOG,   /**< DIALOG */
  IUP_TYPEIMAGE,    /**< IMAGE */
  IUP_TYPEMENU,     /**< MENU, SUBMENU, ITEM, SEPARATOR */
  IUP_TYPEOTHER     /**< Other resources - TIMER, CLIPBOARD, USER, etc */
} InativeType;

/** Possible number of children.
 * \ingroup iclass */
typedef enum _IchildType {
  IUP_CHILDNONE,  /**< can not add children using Append/Insert */
  IUP_CHILDMANY   /**< can add any number of children. /n
                       IUP_CHILDMANY+n can add n children. */
} IchildType;

typedef struct Iclass_ Iclass;

/** Ihandle Class Structure
 * \ingroup iclass */
struct Iclass_
{
  /* Class configuration parameters. */
  const char* name;     /**< class name. No default, must be initialized. */
  const char* cons;     /**< constructor name in C, if more than the first letter uppercase, or NULL. */
  const char* format;   /**< Creation parameters format of the class. \n
                   * Used only for LED parsing. \n
                   * It can have none (NULL), one or more of the following.
                   * - "b" = (unsigned char) - byte            >> unused <<
                   * - "j" = (int*) - array of integer         >> unused <<
                   * - "f" = (float) - real                    >> unused <<
                   * - "i" = (int) - integer                   >> used in IupImage* only <<
                   * - "c" = (unsigned char*) - array of byte  >> used in IupImage* only <<
                   * - "s" = (char*) - string                  >> usually is for TITLE <<
                   * - "a" = (char*) - name of the ACTION callback
                   * - "h" = (Ihandle*) - element handle
                   * - "g" = (Ihandle**) - array of element handle (when used there are no other parameters) */
  const char* format_attr; /**< attribute name of the first creation parameter when format[0] is 's' or 'a' */
  InativeType nativetype; /**< native type. Default is IUP_TYPEVOID. */
  int childtype;   /**< children count enum: none, many, or n, as described in \ref IchildType. Default is IUP_CHILDNONE. \n
                    * This identifies a container that can be manipulated with IupReparent, IupAppend and IupInsert. \n
                    * Used to control the allowed number of children and define its behavior in the layout processing. \n
                    * The element can still have hidden children even if this is none. */
  int is_interactive; /**< keyboard interactive boolean, 
                       * true if the class can have the keyboard input focus. Default is false. */
  int has_attrib_id;  /**< indicate if any attribute is numbered. Default is not. Can be 1 or 2. */
  int is_internal;    /**< indicate an internal class initialized in IupOpen. */

  Iclass* parent; /**< class parent to implement inheritance.
                   * Class name must be different. \n
                   * Creation parameters should be the same or replace the parents creation function. \n
                   * Native type should be the same.  \n
                   * Child type should be a more restrictive or equal type (many->one->none). \n
                   * Attribute functions will have only one common table. \n
                   * All methods can be changed, set to NULL, switched, etc. */

  Itable* attrib_func; /**< table of functions to handle attributes, only one per class tree */

  /* Class methods. */

  /** Method that allocates a new instance of the class. \n
   * Used by inherited classes in \ref iupClassNew.
   */
  Iclass* (*New)(void);


  /** Method that release the memory allocated by the class.
   * Called only once at \ref iupClassRelease.
   */
  void (*Release)(Iclass* ic);



  /** Method that creates the element and process the creation parameters. \n
   * Called only from IupCreate. \n
   * The parameters can be NULL for all the controls. \n
   * The control should also depend on attributes set before IupMap. \n
   * Must return IUP_NOERROR or IUP_ERROR. \n
   * Can be NULL, like all methods.
   */
  int (*Create)(Ihandle* ih, void** params);

  /** Method that map (create) the control to the native system. \n
   * Called only from IupMap. \n
   * Must return IUP_NOERROR or IUP_ERROR.
   */
  int (*Map)(Ihandle* ih);

  /** Method that unmap (destroy) the control from the native system. \n
   * Called only from IupUnmap if the control is mapped. \n
   * Must return IUP_NOERROR or IUP_ERROR.
   */
  void (*UnMap)(Ihandle* ih);

  /** Method that destroys the element. \n
   * Called only from IupDestroy. Always called even if the control is not mapped.
   */
  void (*Destroy)(Ihandle* ih);



  /** Returns the internal native parent. The default implementation returns the handle of itself. \n
    * Called from \ref iupChildTreeGetNativeParentHandle. \n
    * This allows native elements to have an internal container
    * that will be the actual native parent, or in other words allows native elements to be a combination of 
    * other native elements in a single IUP element. 
    * The actual native parent may depend on the child tree (see IupTabs for an example).
   */
  void* (*GetInnerNativeContainerHandle)(Ihandle* ih, Ihandle* child);

  /** Notifies the element that a child was added to the child tree hierarchy. \n
   * Called only from IupAppend, IupInsert or IupReparent. 
   * The child is not mapped yet, but the parent can be mapped.
   */
  void (*ChildAdded)(Ihandle* ih, Ihandle* child);

  /** Notifies the element that a child was removed using IupDetach. \n
   * Called only from IupDetach or IupReparent. 
   * The child is already detached.
   */
  void(*ChildRemoved)(Ihandle* ih, Ihandle* child, int pos);


  /** Method that update size and position of the native control. \n
   * Called only from iupLayoutUpdate and if the element is mapped.
   */
  void (*LayoutUpdate)(Ihandle* ih);



  /** Method that computes the natural size based on the user size and the actual natural size. \n
   * Should update expand if a container, but does NOT depends on expand to compute the natural size. \n
   * Must call the \ref iupBaseComputeNaturalSize for each children.
   * First calculate the native size for the children, then for the element. \n
   * Also called before the element is mapped, so it must be independent of the native control.
   * First call done at iupLayoutCompute for the dialog.
   */
  void (*ComputeNaturalSize)(Ihandle* ih, int *w, int *h, int *children_expand);

  /** Method that calculates and updates the current size of children based on the available size,
   * the natural size and the expand configuration. \n
   * Called only if there is any children.\n
   * Must call \ref iupBaseSetCurrentSize for each children. 
   * shrink is the dialog attribute passed here for optimization. \n
   * Also called before the element is mapped, so it must be independent of the native control.
   * First call done at iupLayoutCompute for the dialog.
   */
  void (*SetChildrenCurrentSize)(Ihandle* ih, int shrink);

  /** Method that calculates and updates the position relative to the parent. \n
   * Called only if there is any children.\n
   * Must call \ref iupBaseSetPosition for each children.
   * Also called before the element is mapped, so it must be independent of the native control.
   * First call done at iupLayoutCompute for the dialog.
   */
  void (*SetChildrenPosition)(Ihandle* ih, int x, int y);



  /** Method that shows a popup dialog. Called only for native pre-defined dialogs. \n
   * The element is not mapped. \n
   * Must return IUP_ERROR or IUP_NOERROR. \n
   * Called only from iupDialogPopup.
   */
  int (*DlgPopup)(Ihandle* ih, int x, int y);   
};



/** Allocates memory for the Iclass structure and 
 * initializes the attribute handling functions table. \n
 * If parent is specified then a new instance of the parent class is created
 * and set as the actual parent class.
 * \ingroup iclass */
IUP_SDK_API Iclass* iupClassNew(Iclass* ic_parent);

/** Release the memory allocated by the class.
 *  Calls the \ref Iclass::Release method. \n
 *  Called from iupRegisterFinish.
 * \ingroup iclass */
IUP_SDK_API void iupClassRelease(Iclass* ic);

/** Check if the class name match the given name. \n
 *  Parent classes are also checked.
 * \ingroup iclass */
IUP_SDK_API int iupClassMatch(Iclass* ic, const char* classname);


/** GetAttribute called for a specific attribute.
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef char* (*IattribGetFunc)(Ihandle* ih);

/** GetAttribute called for a specific attribute when has_attrib_id is 1. \n
 * Same as IattribGetFunc but handle attribute names with number ids at the end. \n
 * When calling iupClassRegisterAttribute just use a typecast. \n
 * -1 is used for invalid ids. \n
 * Pure numbers are translated into IDVALUEid.
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef char* (*IattribGetIdFunc)(Ihandle* ih, int id);

/** GetAttribute called for a specific attribute when has_attrib_id is 1. \n
 * Same as IattribGetFunc but handle attribute names with number ids at the end. \n
 * When calling iupClassRegisterAttribute just use a typecast. \n
 * -1 is used for invalid ids. \n
 * Pure numbers are translated into IDVALUEid.
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef char* (*IattribGetId2Func)(Ihandle* ih, int id1, int id2);

/** SetAttribute called for a specific attribute. \n
 * If returns 0, the attribute will not be stored in the hash table
 * (except inheritble attributes that are always stored in the hash table). \n
 * When IupSetAttribute is called using value=NULL, the default_value is passed to this function.
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef int (*IattribSetFunc)(Ihandle* ih, const char* value);

/** SetAttribute called for a specific attribute when has_attrib_id is 1. \n
 * Same as IattribSetFunc but handle attribute names with number ids at the end. \n
 * When calling iupClassRegisterAttribute just use a typecast. \n
 * -1 is used for invalid ids. \n
 * Pure numbers are translated into IDVALUEid, ex: "1" = "IDVALUE1".
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef int (*IattribSetIdFunc)(Ihandle* ih, int id, const char* value);

/** SetAttribute called for a specific attribute when has_attrib_id is 2. \n
 * Same as IattribSetFunc but handle attribute names with number ids at the end. \n
 * When calling iupClassRegisterAttribute just use a typecast. \n
 * -1 is used for invalid ids. \n
 * Pure numbers are translated into IDVALUEid, ex: "1" = "IDVALUE1".
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef int (*IattribSetId2Func)(Ihandle* ih, int id1, int id2, const char* value);

/** Attribute flags.
 * Used by \ref iupClassRegisterAttribute.
 * \ingroup iclass */
typedef enum _IattribFlags{
  IUPAF_DEFAULT=0,     /**< inheritable, can has a default value, is a string, can call the set/get functions only if mapped, no ID (to be used alone when there are no other flags) */
  IUPAF_NO_INHERIT=1,  /**< is not inheritable */
  IUPAF_NO_DEFAULTVALUE=2,  /**< can not has a default value */
  IUPAF_NO_STRING=4,   /**< is not a string */
  IUPAF_NOT_MAPPED=8,  /**< will call the set/get functions also when not mapped */
  IUPAF_HAS_ID=16,     /**< can has an ID at the end of the name, automatically set by \ref iupClassRegisterAttributeId */
  IUPAF_READONLY=32,   /**< is read-only, can not be changed (except when using internal functions), get is optional, set will never be used */
  IUPAF_WRITEONLY=64,  /**< is write-only, usually an action, set must exist, get will never be used */
  IUPAF_HAS_ID2=128,   /**< can has two IDs at the end of the name, automatically set by \ref iupClassRegisterAttributeId2 */
  IUPAF_CALLBACK=256,  /**< is a callback, not an attribute */
  IUPAF_NO_SAVE=512,   /**< can NOT be directly saved, should have at least manual processing */
  IUPAF_NOT_SUPPORTED=1024,  /**< not supported in that driver */
  IUPAF_IHANDLENAME=2048,    /**< is an Ihandle* name, associated with IupSetHandle */
  IUPAF_IHANDLE=4096         /**< is an Ihandle* */
} IattribFlags;

#define IUPAF_SAMEASSYSTEM ((char*)-1)  /**< means that the default value is the same as the system default value, used only in \ref iupClassRegisterAttribute */


/** Register attribute handling functions, defaults and flags. get, set and default_value can be NULL.
 * default_value should point to a constant string, it will not be duplicated internally. \n
 * Notice that when an attribute is not defined then default_value=NULL, 
 * is inheritable can has a default value and is a string. \n
 * Since there is only one attribute function table per class tree, 
 * if you register the same attribute in a child class, then it will replace the parent registration. \n
 * If an attribute is not inheritable or not a string then it MUST be registered.
 * Internal attributes (starting with "_IUP") can never be registered.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterAttribute(Iclass* ic, const char* name,
                                           IattribGetFunc get, 
                                           IattribSetFunc set, 
                                           const char* default_value, 
                                           const char* system_default, 
                                           int flags);

/** Same as \ref iupClassRegisterAttribute for attributes with Ids.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterAttributeId(Iclass* ic, const char* name,
                                           IattribGetIdFunc get, 
                                           IattribSetIdFunc set, 
                                           int flags);

/** Same as \ref iupClassRegisterAttribute for attributes with two Ids.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterAttributeId2(Iclass* ic, const char* name,
                                           IattribGetId2Func get, 
                                           IattribSetId2Func set, 
                                           int flags);

/** Returns the attribute handling functions, defaults and flags.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterGetAttribute(Iclass* ic, const char* name,
                                           IattribGetFunc *get, 
                                           IattribSetFunc *set, 
                                           const char* *default_value, 
                                           const char* *system_default, 
                                           int *flags);

/** Replaces the attribute handling functions of an already registered attribute.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterReplaceAttribFunc(Iclass* ic, const char* name, IattribGetFunc _get, IattribSetFunc _set);

/** Replaces the attribute handling default of an already registered attribute.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterReplaceAttribDef(Iclass* ic, const char* name, const char* _default_value, const char* _system_default);

/** Replaces the attribute handling functions of an already registered attribute.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterReplaceAttribFlags(Iclass* ic, const char* name, int _flags);


/** Register the parameters of a callback. \n
 * Format follows the \ref iupcbs.h header definitions. \n
 * Notice that these definitions are similar to the class registration
 * but have several differences and conflicts, for backward compatibility reasons. \n
 * It can have none, one or more of the following. \n
 * - "c" = (unsigned char) - byte
 * - "i" = (int) - integer
 * - "I" = (int*) - array of integers or pointer to integer
 * - "f" = (float) - real
 * - "d" = (double) - real
 * - "s" = (char*) - string 
 * - "V" = (void*) - generic pointer 
 * - "C" = (struct _cdCanvas*) - cdCanvas* structure, used along with the CD library
 * - "n" = (Ihandle*) - element handle
 * The default return value for all callbacks is "i" (int), 
 * but a different return value can be specified using one of the above parameters, 
 * after all parameters using "=" to separate it from them.
 * \ingroup iclass */
IUP_SDK_API void iupClassRegisterCallback(Iclass* ic, const char* name, const char* format);

/** Returns the format of the parameters of a registered callback. 
 * If NULL then the default callback definition is assumed.
 * \ingroup iclass */
IUP_SDK_API char* iupClassCallbackGetFormat(Iclass* ic, const char* name);



/** \defgroup iclassobject Class Object Functions
 * \par
 * Stubs for the class methods. They implement inheritance and check if method is NULL.
 * \par
 * See \ref iup_class.h
 * \ingroup iclass
 */

/** Calls \ref Iclass::Create method. 
 * \ingroup iclassobject
 */
IUP_SDK_API int iupClassObjectCreate(Ihandle* ih, void** params);

/** Calls \ref Iclass::Map method. 
 * \ingroup iclassobject
 */
IUP_SDK_API int iupClassObjectMap(Ihandle* ih);

/** Calls \ref Iclass::UnMap method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectUnMap(Ihandle* ih);

/** Calls \ref Iclass::Destroy method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectDestroy(Ihandle* ih);

/** Calls \ref Iclass::GetInnerNativeContainerHandle method. Returns ih->handle if there is no inner parent.
 * The parent class is ignored. If necessary the child class must handle the parent class internally.
 * \ingroup iclassobject
 */
IUP_SDK_API void* iupClassObjectGetInnerNativeContainerHandle(Ihandle* ih, Ihandle* child);

/** Calls \ref Iclass::ChildAdded method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectChildAdded(Ihandle* ih, Ihandle* child);

/** Calls \ref Iclass::ChildRemoved method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectChildRemoved(Ihandle* ih, Ihandle* child, int pos);

/** Calls \ref Iclass::LayoutUpdate method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectLayoutUpdate(Ihandle* ih);

/** Calls \ref Iclass::ComputeNaturalSize method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectComputeNaturalSize(Ihandle* ih, int *w, int *h, int *children_expand);

/** Calls \ref Iclass::SetChildrenCurrentSize method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectSetChildrenCurrentSize(Ihandle* ih, int shrink);

/** Calls \ref Iclass::SetChildrenPosition method. 
 * \ingroup iclassobject
 */
IUP_SDK_API void iupClassObjectSetChildrenPosition(Ihandle* ih, int x, int y);

/** Calls \ref Iclass::DlgPopup method. 
 * \ingroup iclassobject
 */
IUP_SDK_API int iupClassObjectDlgPopup(Ihandle* ih, int x, int y);

/** Checks if class has the \ref Iclass::DlgPopup method.
* \ingroup iclassobject
*/
IUP_SDK_API int iupClassObjectHasDlgPopup(Ihandle* ih);


/* Handle attributes, but since the attribute function table is shared by the class hierarchy,
 * the attribute function is retrieved only from the current class.
 * Set is called from iupAttribUpdate (IupMap), IupStoreAttribute and IupSetAttribute.
 * Get is called only from IupGetAttribute.
 */
int   iupClassObjectSetAttribute(Ihandle* ih, const char* name, const char* value, int *inherit);
char* iupClassObjectGetAttribute(Ihandle* ih, const char* name, char* *def_value, int *inherit);
int   iupClassObjectSetAttributeId(Ihandle* ih, const char* name, int id, const char* value);
char* iupClassObjectGetAttributeId(Ihandle* ih, const char* name, int id);
int   iupClassObjectSetAttributeId2(Ihandle* ih, const char* name, int id1, int id2, const char* value);
char* iupClassObjectGetAttributeId2(Ihandle* ih, const char* name, int id1, int id2);

/* Used only in iupAttribGetStr */
void  iupClassObjectGetAttributeInfo(Ihandle* ih, const char* name, char* *def_value, int *inherit);

/* Used only in iupAttribIsNotString */
int   iupClassObjectAttribIsNotString(Ihandle* ih, const char* name);

/* Used only in iupAttribIsIhandle */
int   iupClassObjectAttribIsIhandle(Ihandle* ih, const char* name);

IUP_SDK_API int iupClassObjectAttribIsCallback(Ihandle* ih, const char* name);

/* Used only in iupAttribSetTheme */
int   iupClassObjectAttribCanCopy(Ihandle* ih, const char* name);

/* Used only in iupAttribUpdateFromParent */
int   iupClassObjectCurAttribIsInherit(Iclass* ic);

/* Used in iupObjectCreate and IupMap */
void iupClassObjectEnsureDefaultAttributes(Ihandle* ih);

/* Used in iupRegisterUpdateClasses */
void iupClassUpdate(Iclass* ic);

/* Used in IupLayoutDialog */
int iupClassAttribIsRegistered(Iclass* ic, const char* name);
void iupClassGetAttribNameInfo(Iclass* ic, const char* name, char* *def_value, int *flags);

/* Used in iupClassRegisterAttribute and iGlobalChangingDefaultColor */
int iupClassIsGlobalDefault(const char* name, int colors);

IUP_SDK_API void iupClassInfoGetDesc(Iclass* ic, Ihandle* ih, const char* attrib_name);
IUP_SDK_API void iupClassInfoShowHelp(const char* className);

/* Other functions declared in <iup.h> and implemented here. 
IupGetClassType
IupGetClassName
IupClassMatch
*/


#ifdef __cplusplus
}
#endif

#include "iup_classbase.h"

#endif
