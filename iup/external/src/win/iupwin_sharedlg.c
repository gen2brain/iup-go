/** \file
 * \brief IupShareDlg pre-defined dialog
 *
 * See Copyright Notice in "iup.h"
 */

#include <windows.h>

#include <stdlib.h>
#include <string.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_class.h"
#include "iup_stdcontrols.h"


typedef void* IwinHString;

typedef HRESULT (WINAPI *PtrRoInitialize)(int type);
typedef void (WINAPI *PtrRoUninitialize)(void);
typedef HRESULT (WINAPI *PtrRoGetActivationFactory)(IwinHString id, REFIID iid, void** factory);
typedef HRESULT (WINAPI *PtrWindowsCreateString)(const WCHAR* str, UINT32 len, IwinHString* hstr);
typedef HRESULT (WINAPI *PtrWindowsDeleteString)(IwinHString hstr);

static PtrRoInitialize winRoInitialize = NULL;
static PtrRoUninitialize winRoUninitialize = NULL;
static PtrRoGetActivationFactory winRoGetActivationFactory = NULL;
static PtrWindowsCreateString winWindowsCreateString = NULL;
static PtrWindowsDeleteString winWindowsDeleteString = NULL;

static const GUID iwin_IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID iwin_IID_IInspectable = { 0xaf86e2e0, 0xb12d, 0x4c6a, { 0x9c, 0x5a, 0xd7, 0xaa, 0x65, 0x10, 0x1e, 0x90 } };
static const GUID iwin_IID_IAgileObject = { 0x94ea2b94, 0xe9cc, 0x49e0, { 0xc0, 0xff, 0xee, 0x64, 0xca, 0x8f, 0x5b, 0x90 } };
static const GUID iwin_IID_IAsyncInfo = { 0x00000036, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID iwin_IID_IDataTransferManagerInterop = { 0x3a3dcd6c, 0x3eab, 0x43dc, { 0xbc, 0xde, 0x45, 0x67, 0x1c, 0xe8, 0x00, 0xc8 } };
static const GUID iwin_IID_IDataTransferManager = { 0xa5caee9b, 0x8708, 0x49d1, { 0x8d, 0x36, 0x67, 0xd2, 0x5a, 0x8d, 0xa0, 0x0c } };
static const GUID iwin_IID_IDataPackage2 = { 0x041c1fe9, 0x2409, 0x45e1, { 0xa5, 0x38, 0x4c, 0x53, 0xee, 0xee, 0x04, 0xa7 } };
static const GUID iwin_IID_IDataPackage4 = { 0x13a24ec8, 0x9382, 0x536f, { 0x85, 0x2a, 0x30, 0x45, 0xe1, 0xb2, 0x9a, 0x3b } };
static const GUID iwin_IID_IUriRuntimeClassFactory = { 0x44a9796f, 0x723e, 0x4fdf, { 0xa2, 0x18, 0x03, 0x3e, 0x75, 0xb0, 0xc0, 0x84 } };
static const GUID iwin_IID_IStorageFileStatics = { 0x5984c710, 0xdaf2, 0x43c8, { 0x8b, 0xb4, 0xa4, 0xd3, 0xea, 0xcf, 0xd0, 0x3f } };
static const GUID iwin_IID_IStorageItem = { 0x4207a996, 0xca2f, 0x42f7, { 0xbd, 0xe8, 0x8b, 0x10, 0x45, 0x7a, 0x7f, 0x30 } };
static const GUID iwin_IID_IIterableStorageItem = { 0xbb8b8418, 0x65d1, 0x544b, { 0xb0, 0x83, 0x6d, 0x17, 0x2f, 0x56, 0x8c, 0x73 } };
static const GUID iwin_IID_IIteratorStorageItem = { 0x05b487c2, 0x3830, 0x5d3c, { 0x98, 0xda, 0x25, 0xfa, 0x11, 0x54, 0x2d, 0xbd } };
static const GUID iwin_IID_DataRequestedHandler = { 0xec6f9cc8, 0x46d0, 0x5e0e, { 0xb4, 0xd2, 0x7d, 0x77, 0x73, 0xae, 0x37, 0xa0 } };
static const GUID iwin_IID_TargetChosenHandler = { 0xc4ac1ba2, 0x7851, 0x5a44, { 0xbc, 0x8d, 0x3d, 0x7c, 0x71, 0x3f, 0x1f, 0x41 } };
static const GUID iwin_IID_ShareCanceledHandler = { 0xc156b0c3, 0x1cbc, 0x5ca4, { 0x90, 0x1c, 0x62, 0xc5, 0xa8, 0xca, 0x5c, 0xb5 } };

typedef struct _IwinUnknown IwinUnknown;
typedef struct _IwinUnknownVtbl {
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinUnknown* This, REFIID iid, void** out);
  ULONG (STDMETHODCALLTYPE *AddRef)(IwinUnknown* This);
  ULONG (STDMETHODCALLTYPE *Release)(IwinUnknown* This);
} IwinUnknownVtbl;
struct _IwinUnknown { const IwinUnknownVtbl* lpVtbl; };

#define IWIN_INSPECTABLE_METHODS(T) \
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(T* This, REFIID iid, void** out); \
  ULONG (STDMETHODCALLTYPE *AddRef)(T* This); \
  ULONG (STDMETHODCALLTYPE *Release)(T* This); \
  HRESULT (STDMETHODCALLTYPE *GetIids)(T* This, ULONG* count, GUID** iids); \
  HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(T* This, IwinHString* name); \
  HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(T* This, int* level);

typedef struct _IwinDataTransferManagerInterop IwinDataTransferManagerInterop;
typedef struct _IwinDataTransferManagerInteropVtbl {
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinDataTransferManagerInterop* This, REFIID iid, void** out);
  ULONG (STDMETHODCALLTYPE *AddRef)(IwinDataTransferManagerInterop* This);
  ULONG (STDMETHODCALLTYPE *Release)(IwinDataTransferManagerInterop* This);
  HRESULT (STDMETHODCALLTYPE *GetForWindow)(IwinDataTransferManagerInterop* This, HWND hwnd, REFIID iid, void** manager);
  HRESULT (STDMETHODCALLTYPE *ShowShareUIForWindow)(IwinDataTransferManagerInterop* This, HWND hwnd);
} IwinDataTransferManagerInteropVtbl;
struct _IwinDataTransferManagerInterop { const IwinDataTransferManagerInteropVtbl* lpVtbl; };

typedef struct _IwinDataTransferManager IwinDataTransferManager;
typedef struct _IwinDataTransferManagerVtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataTransferManager)
  HRESULT (STDMETHODCALLTYPE *add_DataRequested)(IwinDataTransferManager* This, void* handler, INT64* token);
  HRESULT (STDMETHODCALLTYPE *remove_DataRequested)(IwinDataTransferManager* This, INT64 token);
  HRESULT (STDMETHODCALLTYPE *add_TargetApplicationChosen)(IwinDataTransferManager* This, void* handler, INT64* token);
  HRESULT (STDMETHODCALLTYPE *remove_TargetApplicationChosen)(IwinDataTransferManager* This, INT64 token);
} IwinDataTransferManagerVtbl;
struct _IwinDataTransferManager { const IwinDataTransferManagerVtbl* lpVtbl; };

typedef struct _IwinDataRequestedEventArgs IwinDataRequestedEventArgs;
typedef struct _IwinDataRequestedEventArgsVtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataRequestedEventArgs)
  HRESULT (STDMETHODCALLTYPE *get_Request)(IwinDataRequestedEventArgs* This, void** request);
} IwinDataRequestedEventArgsVtbl;
struct _IwinDataRequestedEventArgs { const IwinDataRequestedEventArgsVtbl* lpVtbl; };

typedef struct _IwinDataRequest IwinDataRequest;
typedef struct _IwinDataRequestVtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataRequest)
  HRESULT (STDMETHODCALLTYPE *get_Data)(IwinDataRequest* This, void** package);
} IwinDataRequestVtbl;
struct _IwinDataRequest { const IwinDataRequestVtbl* lpVtbl; };

typedef struct _IwinDataPackage IwinDataPackage;
typedef struct _IwinDataPackageVtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataPackage)
  HRESULT (STDMETHODCALLTYPE *GetView)(IwinDataPackage* This, void** view);
  HRESULT (STDMETHODCALLTYPE *get_Properties)(IwinDataPackage* This, void** properties);
  HRESULT (STDMETHODCALLTYPE *get_RequestedOperation)(IwinDataPackage* This, UINT32* operation);
  HRESULT (STDMETHODCALLTYPE *put_RequestedOperation)(IwinDataPackage* This, UINT32 operation);
  HRESULT (STDMETHODCALLTYPE *add_OperationCompleted)(IwinDataPackage* This, void* handler, INT64* token);
  HRESULT (STDMETHODCALLTYPE *remove_OperationCompleted)(IwinDataPackage* This, INT64 token);
  HRESULT (STDMETHODCALLTYPE *add_Destroyed)(IwinDataPackage* This, void* handler, INT64* token);
  HRESULT (STDMETHODCALLTYPE *remove_Destroyed)(IwinDataPackage* This, INT64 token);
  HRESULT (STDMETHODCALLTYPE *SetData)(IwinDataPackage* This, IwinHString format, void* value);
  HRESULT (STDMETHODCALLTYPE *SetDataProvider)(IwinDataPackage* This, IwinHString format, void* provider);
  HRESULT (STDMETHODCALLTYPE *SetText)(IwinDataPackage* This, IwinHString text);
  HRESULT (STDMETHODCALLTYPE *SetUri)(IwinDataPackage* This, void* uri);
  HRESULT (STDMETHODCALLTYPE *SetHtmlFormat)(IwinDataPackage* This, IwinHString html);
  HRESULT (STDMETHODCALLTYPE *get_ResourceMap)(IwinDataPackage* This, void** map);
  HRESULT (STDMETHODCALLTYPE *SetRtf)(IwinDataPackage* This, IwinHString rtf);
  HRESULT (STDMETHODCALLTYPE *SetBitmap)(IwinDataPackage* This, void* bitmap);
  HRESULT (STDMETHODCALLTYPE *SetStorageItemsReadOnly)(IwinDataPackage* This, void* items);
} IwinDataPackageVtbl;
struct _IwinDataPackage { const IwinDataPackageVtbl* lpVtbl; };

typedef struct _IwinDataPackage2 IwinDataPackage2;
typedef struct _IwinDataPackage2Vtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataPackage2)
  HRESULT (STDMETHODCALLTYPE *SetApplicationLink)(IwinDataPackage2* This, void* uri);
  HRESULT (STDMETHODCALLTYPE *SetWebLink)(IwinDataPackage2* This, void* uri);
} IwinDataPackage2Vtbl;
struct _IwinDataPackage2 { const IwinDataPackage2Vtbl* lpVtbl; };

typedef struct _IwinDataPackageEvent IwinDataPackageEvent;
typedef struct _IwinDataPackageEventVtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataPackageEvent)
  HRESULT (STDMETHODCALLTYPE *add_Event)(IwinDataPackageEvent* This, void* handler, INT64* token);
  HRESULT (STDMETHODCALLTYPE *remove_Event)(IwinDataPackageEvent* This, INT64 token);
} IwinDataPackageEventVtbl;
struct _IwinDataPackageEvent { const IwinDataPackageEventVtbl* lpVtbl; };

typedef struct _IwinDataPackagePropertySet IwinDataPackagePropertySet;
typedef struct _IwinDataPackagePropertySetVtbl {
  IWIN_INSPECTABLE_METHODS(IwinDataPackagePropertySet)
  HRESULT (STDMETHODCALLTYPE *get_Title)(IwinDataPackagePropertySet* This, IwinHString* title);
  HRESULT (STDMETHODCALLTYPE *put_Title)(IwinDataPackagePropertySet* This, IwinHString title);
} IwinDataPackagePropertySetVtbl;
struct _IwinDataPackagePropertySet { const IwinDataPackagePropertySetVtbl* lpVtbl; };

typedef struct _IwinUriFactory IwinUriFactory;
typedef struct _IwinUriFactoryVtbl {
  IWIN_INSPECTABLE_METHODS(IwinUriFactory)
  HRESULT (STDMETHODCALLTYPE *CreateUri)(IwinUriFactory* This, IwinHString uri, void** instance);
} IwinUriFactoryVtbl;
struct _IwinUriFactory { const IwinUriFactoryVtbl* lpVtbl; };

typedef struct _IwinStorageFileStatics IwinStorageFileStatics;
typedef struct _IwinStorageFileStaticsVtbl {
  IWIN_INSPECTABLE_METHODS(IwinStorageFileStatics)
  HRESULT (STDMETHODCALLTYPE *GetFileFromPathAsync)(IwinStorageFileStatics* This, IwinHString path, void** operation);
} IwinStorageFileStaticsVtbl;
struct _IwinStorageFileStatics { const IwinStorageFileStaticsVtbl* lpVtbl; };

typedef struct _IwinAsyncInfo IwinAsyncInfo;
typedef struct _IwinAsyncInfoVtbl {
  IWIN_INSPECTABLE_METHODS(IwinAsyncInfo)
  HRESULT (STDMETHODCALLTYPE *get_Id)(IwinAsyncInfo* This, UINT32* id);
  HRESULT (STDMETHODCALLTYPE *get_Status)(IwinAsyncInfo* This, int* status);
} IwinAsyncInfoVtbl;
struct _IwinAsyncInfo { const IwinAsyncInfoVtbl* lpVtbl; };

typedef struct _IwinAsyncOperation IwinAsyncOperation;
typedef struct _IwinAsyncOperationVtbl {
  IWIN_INSPECTABLE_METHODS(IwinAsyncOperation)
  HRESULT (STDMETHODCALLTYPE *put_Completed)(IwinAsyncOperation* This, void* handler);
  HRESULT (STDMETHODCALLTYPE *get_Completed)(IwinAsyncOperation* This, void** handler);
  HRESULT (STDMETHODCALLTYPE *GetResults)(IwinAsyncOperation* This, void** results);
} IwinAsyncOperationVtbl;
struct _IwinAsyncOperation { const IwinAsyncOperationVtbl* lpVtbl; };

typedef struct _IwinShareState {
  volatile LONG refcount;
  volatile LONG status;
  int wait_cancel;
  IwinHString title;
  IwinHString text;
  void* uri;
  IwinUnknown* item;
  IwinUnknown* canceled;
} IwinShareState;

static void winShareStateRelease(IwinShareState* state)
{
  if (InterlockedDecrement(&state->refcount) != 0)
    return;
  if (state->title) winWindowsDeleteString(state->title);
  if (state->text) winWindowsDeleteString(state->text);
  if (state->uri) ((IwinUnknown*)state->uri)->lpVtbl->Release((IwinUnknown*)state->uri);
  if (state->item) state->item->lpVtbl->Release(state->item);
  if (state->canceled) state->canceled->lpVtbl->Release(state->canceled);
  free(state);
}

typedef struct _IwinShareHandler IwinShareHandler;
typedef struct _IwinShareHandlerVtbl {
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinShareHandler* This, REFIID iid, void** out);
  ULONG (STDMETHODCALLTYPE *AddRef)(IwinShareHandler* This);
  ULONG (STDMETHODCALLTYPE *Release)(IwinShareHandler* This);
  HRESULT (STDMETHODCALLTYPE *Invoke)(IwinShareHandler* This, void* sender, void* args);
} IwinShareHandlerVtbl;
struct _IwinShareHandler {
  const IwinShareHandlerVtbl* lpVtbl;
  volatile LONG refcount;
  const GUID* iid;
  IwinShareState* state;
  void (*invoke)(IwinShareState* state, void* sender, void* args);
};

static HRESULT STDMETHODCALLTYPE winShareHandlerQueryInterface(IwinShareHandler* This, REFIID iid, void** out)
{
  if (IsEqualGUID(iid, &iwin_IID_IUnknown) || IsEqualGUID(iid, &iwin_IID_IAgileObject) || IsEqualGUID(iid, This->iid))
  {
    *out = This;
    InterlockedIncrement(&This->refcount);
    return S_OK;
  }
  *out = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE winShareHandlerAddRef(IwinShareHandler* This)
{
  return (ULONG)InterlockedIncrement(&This->refcount);
}

static ULONG STDMETHODCALLTYPE winShareHandlerRelease(IwinShareHandler* This)
{
  LONG count = InterlockedDecrement(&This->refcount);
  if (count == 0)
  {
    winShareStateRelease(This->state);
    free(This);
  }
  return (ULONG)count;
}

static HRESULT STDMETHODCALLTYPE winShareHandlerInvoke(IwinShareHandler* This, void* sender, void* args)
{
  This->invoke(This->state, sender, args);
  return S_OK;
}

static const IwinShareHandlerVtbl iwin_share_handler_vtbl = {
  winShareHandlerQueryInterface,
  winShareHandlerAddRef,
  winShareHandlerRelease,
  winShareHandlerInvoke
};

static IwinUnknown* winShareHandlerNew(const GUID* iid, IwinShareState* state, void (*invoke)(IwinShareState*, void*, void*))
{
  IwinShareHandler* handler = (IwinShareHandler*)calloc(1, sizeof(IwinShareHandler));
  handler->lpVtbl = &iwin_share_handler_vtbl;
  handler->refcount = 1;
  handler->iid = iid;
  handler->state = state;
  handler->invoke = invoke;
  InterlockedIncrement(&state->refcount);
  return (IwinUnknown*)handler;
}

typedef struct _IwinShareItems IwinShareItems;
typedef struct _IwinShareItemsVtbl {
  IWIN_INSPECTABLE_METHODS(IwinShareItems)
  HRESULT (STDMETHODCALLTYPE *First)(IwinShareItems* This, void** iterator);
} IwinShareItemsVtbl;
typedef struct _IwinShareIterator IwinShareIterator;
typedef struct _IwinShareIteratorVtbl {
  IWIN_INSPECTABLE_METHODS(IwinShareIterator)
  HRESULT (STDMETHODCALLTYPE *get_Current)(IwinShareIterator* This, void** item);
  HRESULT (STDMETHODCALLTYPE *get_HasCurrent)(IwinShareIterator* This, unsigned char* has);
  HRESULT (STDMETHODCALLTYPE *MoveNext)(IwinShareIterator* This, unsigned char* has);
  HRESULT (STDMETHODCALLTYPE *GetMany)(IwinShareIterator* This, UINT32 capacity, void** items, UINT32* count);
} IwinShareIteratorVtbl;

struct _IwinShareItems {
  const IwinShareItemsVtbl* lpVtbl;
  volatile LONG refcount;
  IwinUnknown* item;
};
struct _IwinShareIterator {
  const IwinShareIteratorVtbl* lpVtbl;
  volatile LONG refcount;
  IwinUnknown* item;
  int done;
};

static HRESULT winShareObjectQuery(void* This, volatile LONG* refcount, const GUID* own, REFIID iid, void** out)
{
  if (IsEqualGUID(iid, &iwin_IID_IUnknown) || IsEqualGUID(iid, &iwin_IID_IInspectable) || IsEqualGUID(iid, &iwin_IID_IAgileObject) || IsEqualGUID(iid, own))
  {
    *out = This;
    InterlockedIncrement(refcount);
    return S_OK;
  }
  *out = NULL;
  return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE winShareItemsQueryInterface(IwinShareItems* This, REFIID iid, void** out)
{
  return winShareObjectQuery(This, &This->refcount, &iwin_IID_IIterableStorageItem, iid, out);
}

static ULONG STDMETHODCALLTYPE winShareItemsAddRef(IwinShareItems* This)
{
  return (ULONG)InterlockedIncrement(&This->refcount);
}

static ULONG STDMETHODCALLTYPE winShareItemsRelease(IwinShareItems* This)
{
  LONG count = InterlockedDecrement(&This->refcount);
  if (count == 0)
  {
    This->item->lpVtbl->Release(This->item);
    free(This);
  }
  return (ULONG)count;
}

static HRESULT STDMETHODCALLTYPE winShareItemsGetIids(IwinShareItems* This, ULONG* count, GUID** iids)
{
  (void)This;
  *count = 0;
  *iids = NULL;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareItemsGetRuntimeClassName(IwinShareItems* This, IwinHString* name)
{
  (void)This;
  *name = NULL;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareItemsGetTrustLevel(IwinShareItems* This, int* level)
{
  (void)This;
  *level = 0;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorQueryInterface(IwinShareIterator* This, REFIID iid, void** out)
{
  return winShareObjectQuery(This, &This->refcount, &iwin_IID_IIteratorStorageItem, iid, out);
}

static ULONG STDMETHODCALLTYPE winShareIteratorAddRef(IwinShareIterator* This)
{
  return (ULONG)InterlockedIncrement(&This->refcount);
}

static ULONG STDMETHODCALLTYPE winShareIteratorRelease(IwinShareIterator* This)
{
  LONG count = InterlockedDecrement(&This->refcount);
  if (count == 0)
  {
    This->item->lpVtbl->Release(This->item);
    free(This);
  }
  return (ULONG)count;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorGetIids(IwinShareIterator* This, ULONG* count, GUID** iids)
{
  (void)This;
  *count = 0;
  *iids = NULL;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorGetRuntimeClassName(IwinShareIterator* This, IwinHString* name)
{
  (void)This;
  *name = NULL;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorGetTrustLevel(IwinShareIterator* This, int* level)
{
  (void)This;
  *level = 0;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorGetCurrent(IwinShareIterator* This, void** item)
{
  if (This->done)
  {
    *item = NULL;
    return E_BOUNDS;
  }
  This->item->lpVtbl->AddRef(This->item);
  *item = This->item;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorGetHasCurrent(IwinShareIterator* This, unsigned char* has)
{
  *has = This->done ? 0 : 1;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorMoveNext(IwinShareIterator* This, unsigned char* has)
{
  This->done = 1;
  *has = 0;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE winShareIteratorGetMany(IwinShareIterator* This, UINT32 capacity, void** items, UINT32* count)
{
  *count = 0;
  if (!This->done && capacity > 0)
  {
    This->item->lpVtbl->AddRef(This->item);
    items[0] = This->item;
    This->done = 1;
    *count = 1;
  }
  return S_OK;
}

static const IwinShareIteratorVtbl iwin_share_iterator_vtbl = {
  winShareIteratorQueryInterface,
  winShareIteratorAddRef,
  winShareIteratorRelease,
  winShareIteratorGetIids,
  winShareIteratorGetRuntimeClassName,
  winShareIteratorGetTrustLevel,
  winShareIteratorGetCurrent,
  winShareIteratorGetHasCurrent,
  winShareIteratorMoveNext,
  winShareIteratorGetMany
};

static HRESULT STDMETHODCALLTYPE winShareItemsFirst(IwinShareItems* This, void** iterator)
{
  IwinShareIterator* it = (IwinShareIterator*)calloc(1, sizeof(IwinShareIterator));
  it->lpVtbl = &iwin_share_iterator_vtbl;
  it->refcount = 1;
  it->item = This->item;
  it->item->lpVtbl->AddRef(it->item);
  *iterator = it;
  return S_OK;
}

static const IwinShareItemsVtbl iwin_share_items_vtbl = {
  winShareItemsQueryInterface,
  winShareItemsAddRef,
  winShareItemsRelease,
  winShareItemsGetIids,
  winShareItemsGetRuntimeClassName,
  winShareItemsGetTrustLevel,
  winShareItemsFirst
};

static int winShareLoad(void)
{
  static int loaded = 0;
  if (!loaded)
  {
    HMODULE lib = LoadLibraryA("combase.dll");
    loaded = 1;
    if (lib)
    {
      winRoInitialize = (PtrRoInitialize)(void*)GetProcAddress(lib, "RoInitialize");
      winRoUninitialize = (PtrRoUninitialize)(void*)GetProcAddress(lib, "RoUninitialize");
      winRoGetActivationFactory = (PtrRoGetActivationFactory)(void*)GetProcAddress(lib, "RoGetActivationFactory");
      winWindowsCreateString = (PtrWindowsCreateString)(void*)GetProcAddress(lib, "WindowsCreateString");
      winWindowsDeleteString = (PtrWindowsDeleteString)(void*)GetProcAddress(lib, "WindowsDeleteString");
    }
  }
  return winRoInitialize && winRoUninitialize && winRoGetActivationFactory && winWindowsCreateString && winWindowsDeleteString;
}

static IwinHString winShareString(const char* str)
{
  IwinHString hstr = NULL;
  int len;
  WCHAR* wstr;

  if (!str)
    return NULL;

  len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
  if (len <= 0)
    return NULL;

  wstr = (WCHAR*)malloc(len * sizeof(WCHAR));
  MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
  if (FAILED(winWindowsCreateString(wstr, (UINT32)(len - 1), &hstr)))
    hstr = NULL;
  free(wstr);
  return hstr;
}

static void* winShareFactory(const WCHAR* class_name, const GUID* iid)
{
  IwinHString name = NULL;
  void* factory = NULL;
  if (FAILED(winWindowsCreateString(class_name, (UINT32)wcslen(class_name), &name)))
    return NULL;
  if (FAILED(winRoGetActivationFactory(name, iid, &factory)))
    factory = NULL;
  winWindowsDeleteString(name);
  return factory;
}

static void winShareWait(volatile LONG* flag, LONG pending)
{
  while (*flag == pending)
  {
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
      MsgWaitForMultipleObjects(0, NULL, FALSE, 50, QS_ALLINPUT);
  }
}

static IwinUnknown* winShareGetStorageItem(const char* path)
{
  IwinStorageFileStatics* statics = (IwinStorageFileStatics*)winShareFactory(L"Windows.Storage.StorageFile", &iwin_IID_IStorageFileStatics);
  IwinAsyncOperation* op = NULL;
  IwinAsyncInfo* info = NULL;
  IwinUnknown* file = NULL;
  IwinUnknown* item = NULL;
  IwinHString hpath;
  int status = 0;

  if (!statics)
    return NULL;

  hpath = winShareString(path);
  if (hpath && SUCCEEDED(statics->lpVtbl->GetFileFromPathAsync(statics, hpath, (void**)&op)) && op &&
      SUCCEEDED(op->lpVtbl->QueryInterface(op, &iwin_IID_IAsyncInfo, (void**)&info)))
  {
    while (SUCCEEDED(info->lpVtbl->get_Status(info, &status)) && status == 0)
    {
      MSG msg;
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else
        MsgWaitForMultipleObjects(0, NULL, FALSE, 10, QS_ALLINPUT);
    }

    if (status == 1 && SUCCEEDED(op->lpVtbl->GetResults(op, (void**)&file)) && file)
    {
      if (FAILED(file->lpVtbl->QueryInterface(file, &iwin_IID_IStorageItem, (void**)&item)))
        item = NULL;
      file->lpVtbl->Release(file);
    }
    info->lpVtbl->Release(info);
  }

  if (op) op->lpVtbl->Release(op);
  if (hpath) winWindowsDeleteString(hpath);
  statics->lpVtbl->Release(statics);
  return item;
}

static void winShareChosen(IwinShareState* state, void* sender, void* args)
{
  (void)sender; (void)args;
  InterlockedExchange(&state->status, 1);
}

static void winShareCanceled(IwinShareState* state, void* sender, void* args)
{
  (void)sender; (void)args;
  InterlockedCompareExchange(&state->status, 0, 2);
}

static void winShareDataRequested(IwinShareState* state, void* sender, void* args)
{
  IwinDataRequestedEventArgs* event_args = (IwinDataRequestedEventArgs*)args;
  IwinDataRequest* request = NULL;
  IwinDataPackage* package = NULL;
  IwinDataPackagePropertySet* properties = NULL;
  IwinDataPackageEvent* package4 = NULL;
  INT64 token;

  (void)sender;

  if (FAILED(event_args->lpVtbl->get_Request(event_args, (void**)&request)) || !request)
  {
    InterlockedExchange(&state->status, -1);
    return;
  }

  if (SUCCEEDED(request->lpVtbl->get_Data(request, (void**)&package)) && package)
  {
    if (SUCCEEDED(package->lpVtbl->get_Properties(package, (void**)&properties)) && properties)
    {
      properties->lpVtbl->put_Title(properties, state->title);
      properties->lpVtbl->Release(properties);
    }

    if (state->text)
      package->lpVtbl->SetText(package, state->text);

    if (state->uri)
    {
      IwinDataPackage2* package2 = NULL;
      if (SUCCEEDED(package->lpVtbl->QueryInterface(package, &iwin_IID_IDataPackage2, (void**)&package2)))
      {
        package2->lpVtbl->SetWebLink(package2, state->uri);
        package2->lpVtbl->Release(package2);
      }
      else
        package->lpVtbl->SetUri(package, state->uri);
    }

    if (state->item)
    {
      IwinShareItems* items = (IwinShareItems*)calloc(1, sizeof(IwinShareItems));
      items->lpVtbl = &iwin_share_items_vtbl;
      items->refcount = 1;
      items->item = state->item;
      state->item->lpVtbl->AddRef(state->item);
      package->lpVtbl->SetStorageItemsReadOnly(package, items);
      items->lpVtbl->Release(items);
    }

    if (SUCCEEDED(package->lpVtbl->QueryInterface(package, &iwin_IID_IDataPackage4, (void**)&package4)))
    {
      package4->lpVtbl->add_Event(package4, state->canceled, &token);
      package4->lpVtbl->Release(package4);
      state->wait_cancel = 1;
    }

    package->lpVtbl->Release(package);
  }

  request->lpVtbl->Release(request);

  if (!state->wait_cancel)
    InterlockedExchange(&state->status, 1);
}

static int winShareDlgPopup(Ihandle* ih, int x, int y)
{
  const char* title = iupAttribGet(ih, "TITLE");
  const char* text = iupAttribGet(ih, "TEXT");
  const char* url = iupAttribGet(ih, "URL");
  const char* file = iupAttribGet(ih, "FILE");
  HWND hwnd = GetActiveWindow();
  IwinDataTransferManagerInterop* interop = NULL;
  IwinDataTransferManager* manager = NULL;
  IwinUnknown* requested = NULL;
  IwinUnknown* chosen = NULL;
  IwinShareState* state;
  INT64 token = 0, chosen_token = 0;
  HRESULT ro_init;
  int status = -1;

  iupAttribSetInt(ih, "_IUPDLG_X", x);
  iupAttribSetInt(ih, "_IUPDLG_Y", y);

  if (!winShareLoad() || !hwnd || (!text && !url && !file))
  {
    iupAttribSet(ih, "STATUS", "-1");
    return IUP_ERROR;
  }

  ro_init = winRoInitialize(0);

  state = (IwinShareState*)calloc(1, sizeof(IwinShareState));
  state->refcount = 1;
  state->status = 2;

  if (!title)
    title = file ? file : (url ? url : text);
  state->title = winShareString(title);
  state->text = winShareString(text);

  if (url)
  {
    IwinUriFactory* factory = (IwinUriFactory*)winShareFactory(L"Windows.Foundation.Uri", &iwin_IID_IUriRuntimeClassFactory);
    if (factory)
    {
      IwinHString hurl = winShareString(url);
      if (hurl && FAILED(factory->lpVtbl->CreateUri(factory, hurl, &state->uri)))
        state->uri = NULL;
      if (hurl) winWindowsDeleteString(hurl);
      factory->lpVtbl->Release(factory);
    }
  }

  if (file)
    state->item = winShareGetStorageItem(file);

  if ((file && !state->item) || (url && !state->uri && !text && !file))
    goto done;

  state->canceled = winShareHandlerNew(&iwin_IID_ShareCanceledHandler, state, winShareCanceled);
  requested = winShareHandlerNew(&iwin_IID_DataRequestedHandler, state, winShareDataRequested);
  chosen = winShareHandlerNew(&iwin_IID_TargetChosenHandler, state, winShareChosen);

  interop = (IwinDataTransferManagerInterop*)winShareFactory(L"Windows.ApplicationModel.DataTransfer.DataTransferManager", &iwin_IID_IDataTransferManagerInterop);
  if (!interop ||
      FAILED(interop->lpVtbl->GetForWindow(interop, hwnd, &iwin_IID_IDataTransferManager, (void**)&manager)) || !manager ||
      FAILED(manager->lpVtbl->add_DataRequested(manager, requested, &token)))
    goto done;

  if (FAILED(manager->lpVtbl->add_TargetApplicationChosen(manager, chosen, &chosen_token)))
    chosen_token = 0;

  if (SUCCEEDED(interop->lpVtbl->ShowShareUIForWindow(interop, hwnd)))
  {
    winShareWait(&state->status, 2);
    status = (int)state->status;
  }

  if (chosen_token)
    manager->lpVtbl->remove_TargetApplicationChosen(manager, chosen_token);
  manager->lpVtbl->remove_DataRequested(manager, token);

done:
  if (manager) manager->lpVtbl->Release(manager);
  if (interop) interop->lpVtbl->Release(interop);
  if (requested) requested->lpVtbl->Release(requested);
  if (chosen) chosen->lpVtbl->Release(chosen);
  winShareStateRelease(state);

  if (SUCCEEDED(ro_init))
    winRoUninitialize();

  iupAttribSet(ih, "STATUS", status == 1 ? "1" : (status == 0 ? "0" : "-1"));
  return IUP_NOERROR;
}

IUP_SDK_API void iupdrvShareDlgInitClass(Iclass* ic)
{
  ic->DlgPopup = winShareDlgPopup;
}
