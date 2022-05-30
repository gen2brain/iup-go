/** \file
 * \brief Simple hash table C API.
 * Does not allow 0 values for items...
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_TABLE_H 
#define __IUP_TABLE_H

#ifdef __cplusplus
extern "C"
{
#endif

/** \defgroup table Hash Table
 * \par
 * The hash table can be indexed by strings or pointer address, 
 * and each value can contain strings, pointers or function pointers.
 * \par
 * See \ref iup_table.h
 * \ingroup util */


/** How the table key is interpreted.
 * \ingroup table */
typedef enum _Itable_IndexTypes
{
  IUPTABLE_POINTERINDEXED = 10, /**< a pointer address is used as key. */
  IUPTABLE_STRINGINDEXED        /**< a string as key */
} Itable_IndexTypes;

/** How the value is interpreted.
 * \ingroup table */
typedef enum _Itable_Types
{
  IUPTABLE_POINTER,     /**< regular pointer for strings and other pointers */
  IUPTABLE_STRING,      /**< string duplicated internally */
  IUPTABLE_FUNCPOINTER  /**< function pointer */
} Itable_Types;


typedef void (*Ifunc)(void);

struct _Itable;
typedef struct _Itable Itable;


/** Creates a hash table with an initial default size.
 * This function is equivalent to iupTableCreateSized(0);
 * \ingroup table */
IUP_SDK_API Itable *iupTableCreate(Itable_IndexTypes indexType);

/** Creates a hash table with the specified initial size.
 * Use this function if you expect the table to become very large.
 * initialSizeIndex is an array into the (internal) list of
 * possible hash table sizes. Currently only indexes from 0 to 8
 * are supported. If you specify a higher value here, the maximum
 * allowed value will be used.
 * \ingroup table */
IUP_SDK_API Itable *iupTableCreateSized(Itable_IndexTypes indexType, unsigned int initialSizeIndex);

/** Destroys the Itable.
 * Calls \ref iupTableClear.
 * \ingroup table */
IUP_SDK_API void iupTableDestroy(Itable *it);

/** Removes all items in the table.
 * This function does also free the memory of strings contained in the table!!!!
 * \ingroup table */
IUP_SDK_API void iupTableClear(Itable *it);

/** Returns the number of keys stored in the table.
 * \ingroup table */
IUP_SDK_API int iupTableCount(Itable *it);

/** Store an element in the table.
 * \ingroup table */
IUP_SDK_API void iupTableSet(Itable *it, const char *key, void *value, Itable_Types itemType);

/** Store a function pointer in the table.
 * Type is set to IUPTABLE_FUNCPOINTER.
 * \ingroup table */
IUP_SDK_API void iupTableSetFunc(Itable *it, const char *key, Ifunc func);

/** Retrieves an element from the table.
 * Returns NULL if not found.
 * \ingroup table */
IUP_SDK_API void *iupTableGet(Itable *it, const char *key);

/** Retrieves a function pointer from the table. 
 * If not a function or not found returns NULL.
 * value always contains the element pointer.
 * \ingroup table */
IUP_SDK_API Ifunc iupTableGetFunc(Itable *it, const char *key, void **value);

/** Retrieves an element from the table and its type.
 * \ingroup table */
IUP_SDK_API void *iupTableGetTyped(Itable *it, const char *key, Itable_Types *itemType);

/** Removes the entry at the specified key from the
 * hash table and frees the memory used by it if
 * it is a string...
 * \ingroup table */
IUP_SDK_API void iupTableRemove(Itable *it, const char *key);

/** Key iteration function. Returns a key.
 * To iterate over all keys call iupTableFirst at the first
 * and call iupTableNext in a loop
 * until 0 is returned...
 * Do NOT change the content of the hash table during iteration.
 * During an iteration you can use context with
 * iupTableGetCurr() to access the value of the key
 * very fast.
 * \ingroup table */
IUP_SDK_API char *iupTableFirst(Itable *it);

/** Key iteration function. See \ref iupTableNext.
 * \ingroup table */
IUP_SDK_API char *iupTableNext(Itable *it);

/** Returns the value at the current position.  \n
 * The current context is an iterator
 * that is filled by iupTableNext().  \n
 * iupTableGetCur() is faster then iupTableGet(),
 * so when you want to access an item stored
 * at a key returned by iupTableNext(),
 * use this function instead of iupTableGet().
 * \ingroup table */
IUP_SDK_API void *iupTableGetCurr(Itable *it);

/** Returns the type at the current position. \n
 * Same as \ref iupTableGetCurr but returns the type.
 * Returns -1 if failed.
 * \ingroup table */
IUP_SDK_API int iupTableGetCurrType(Itable *it);

/** Replaces the data at the current position.
 * \ingroup table */
IUP_SDK_API void iupTableSetCurr(Itable *it, void *value, Itable_Types itemType);

/** Removes the current element and returns the next key.
 * Use this function to remove an element during an iteration.
 * \ingroup table */
IUP_SDK_API char *iupTableRemoveCurr(Itable *it);


#ifdef __cplusplus
}
#endif

#endif

