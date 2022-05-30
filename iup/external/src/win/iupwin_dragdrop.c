/** \file
 * \brief Windows Drag&Drop Functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>              
#include <stdlib.h>
#include <string.h>             

#include <windows.h>
#include <commctrl.h>
#include <ole2.h>
#include <shlobj.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_object.h"
#include "iup_childtree.h"
#include "iup_key.h"
#include "iup_str.h"
#include "iup_class.h"
#include "iup_attrib.h"
#include "iup_focus.h"
#include "iup_image.h"
#include "iup_dialog.h"
#include "iup_drvinfo.h"
#include "iup_drv.h"
#include "iup_array.h"

#include "iupwin_drv.h"
#include "iupwin_handle.h"
#include "iupwin_brush.h"
#include "iupwin_info.h"
#include "iupwin_str.h"


/* From the OLE Drag and Drop Tutorial at Catch22
   Thanks to James Brown and Davide Chiodi.
   http://www.catch22.net/tuts/ole-drag-and-drop
*/

#if !defined(__GNUC__) && !defined(__BORLANDC__)
#define USE_SHCREATESTDENUMFMTETC
#endif

#ifndef USE_SHCREATESTDENUMFMTETC

typedef struct _IwinEnumFORMATETC
{
  IEnumFORMATETC ief;
  LONG lRefCount;
  ULONG nIndex;
  ULONG nNumFormats;
  FORMATETC* pFormatEtc;
} IwinEnumFORMATETC;

typedef struct _IwinEnumFORMATETCVtbl
{
  BEGIN_INTERFACE
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinEnumFORMATETC* pThis, REFIID riid, void **ppvObject);
  ULONG   (STDMETHODCALLTYPE *AddRef)(IwinEnumFORMATETC* pThis);
  ULONG   (STDMETHODCALLTYPE *Release)(IwinEnumFORMATETC* pThis);
  HRESULT (STDMETHODCALLTYPE *Next)(IwinEnumFORMATETC* pThis, ULONG nCelt, FORMATETC *rgelt, ULONG *pCeltFetched);
  HRESULT (STDMETHODCALLTYPE *Skip)(IwinEnumFORMATETC* pThis, ULONG nCelt);
  HRESULT (STDMETHODCALLTYPE *Reset)(IwinEnumFORMATETC* pThis);
  HRESULT (STDMETHODCALLTYPE *Clone)(IwinEnumFORMATETC* pThis, IwinEnumFORMATETC **ppenum);
  END_INTERFACE
} IwinEnumFORMATETCVtbl;

static void winFormatEtcCopy(FORMATETC *dest, FORMATETC *source)
{
  /* copy the source FORMATETC into dest */
  *dest = *source;
  
  if(source->ptd)
  {
    /* allocate memory for the DVTARGETDEVICE if necessary */
    dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));

    /* copy the contents of the source DVTARGETDEVICE into dest->ptd */
    *(dest->ptd) = *(source->ptd);
  }
}

static HRESULT STDMETHODCALLTYPE IwinEnumFORMATETC_QueryInterface(IwinEnumFORMATETC *pThis, REFIID riid, LPVOID *ppvObject)
{
  if(IsEqualGUID (riid, &IID_IUnknown) ||
     IsEqualGUID (riid, &IID_IEnumFORMATETC))
  {
    InterlockedIncrement(&pThis->lRefCount);
    *ppvObject = pThis;
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE IwinEnumFORMATETC_AddRef (IwinEnumFORMATETC* pThis)
{
  return (ULONG)InterlockedIncrement(&pThis->lRefCount);
}

static ULONG STDMETHODCALLTYPE IwinEnumFORMATETC_Release (IwinEnumFORMATETC* pThis)
{
  ULONG nCount = (ULONG)InterlockedDecrement(&pThis->lRefCount);
  if(nCount == 0)
  {
    ULONG i;
    for(i = 0; i < pThis->nNumFormats; i++)
    {
      if (pThis->pFormatEtc[i].ptd)
        CoTaskMemFree(pThis->pFormatEtc[i].ptd);
    }
    free(pThis->pFormatEtc);
    free(pThis);
    return 0;
  }
  return nCount;
}

static HRESULT STDMETHODCALLTYPE IwinEnumFORMATETC_Next (IwinEnumFORMATETC* pThis, ULONG nCelt, LPFORMATETC pFormatEtc, ULONG *pCeltFetched)
{
  ULONG nCeltCopied  = 0;

  if(nCelt == 0 || pFormatEtc == 0)
    return E_INVALIDARG;

  while(pThis->nIndex < pThis->nNumFormats && nCeltCopied < nCelt)
  {
    winFormatEtcCopy((struct tagFORMATETC*)&pFormatEtc[nCeltCopied], &pThis->pFormatEtc[pThis->nIndex]);
    nCeltCopied++;
    pThis->nIndex++;
  }

  if (pCeltFetched != 0) 
    *pCeltFetched = nCeltCopied;

  return (nCeltCopied == nCelt) ? S_OK : S_FALSE;
}

static HRESULT STDMETHODCALLTYPE IwinEnumFORMATETC_Skip (IwinEnumFORMATETC *pThis, ULONG nCelt)
{
  pThis->nIndex += nCelt;
  return (pThis->nIndex <= pThis->nNumFormats) ? S_OK : S_FALSE;
}

static HRESULT STDMETHODCALLTYPE IwinEnumFORMATETC_Reset (IwinEnumFORMATETC* pThis)
{
  pThis->nIndex = 0;
  return S_OK;
}

static IwinEnumFORMATETC* winCreateEnumFORMATETC(ULONG nNumFormats, FORMATETC *pFormatEtc);

static HRESULT STDMETHODCALLTYPE IwinEnumFORMATETC_Clone (IwinEnumFORMATETC*  pThis, IwinEnumFORMATETC **ppEnumFormatEtc)
{
  *ppEnumFormatEtc = winCreateEnumFORMATETC(pThis->nNumFormats, pThis->pFormatEtc);
  (*ppEnumFormatEtc)->nIndex = pThis->nIndex;
  return S_OK;
}

static IwinEnumFORMATETC* winCreateEnumFORMATETC(ULONG nNumFormats, FORMATETC *pFormatEtc)
{
  ULONG i;
  IwinEnumFORMATETC *pEnumFormatEtc;
  static IwinEnumFORMATETCVtbl ief_vtbl = {
    IwinEnumFORMATETC_QueryInterface,
    IwinEnumFORMATETC_AddRef,
    IwinEnumFORMATETC_Release,
    IwinEnumFORMATETC_Next,
    IwinEnumFORMATETC_Skip,
    IwinEnumFORMATETC_Reset,
    IwinEnumFORMATETC_Clone
  };

  pEnumFormatEtc = malloc(sizeof(IwinEnumFORMATETC));

  pEnumFormatEtc->ief.lpVtbl = (IEnumFORMATETCVtbl*)&ief_vtbl;
  pEnumFormatEtc->lRefCount = 1;

  pEnumFormatEtc->nIndex = 0;
  pEnumFormatEtc->nNumFormats = nNumFormats;
  pEnumFormatEtc->pFormatEtc = malloc(sizeof(FORMATETC)*nNumFormats);
  
  for(i = 0; i < nNumFormats; i++)
    winFormatEtcCopy(&pEnumFormatEtc->pFormatEtc[i], &pFormatEtc[i]);

  return pEnumFormatEtc;
}
#endif


/******************************************************************************/


typedef struct _IwinDropSource
{
  IDropSource ids;
  LONG lRefCount;
  Ihandle* ih;
} IwinDropSource;

typedef struct _IwinDropSourceVtbl
{
  BEGIN_INTERFACE
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinDropSource* pThis, REFIID riid, void **ppvObject);
  ULONG   (STDMETHODCALLTYPE *AddRef)(IwinDropSource* pThis);
  ULONG   (STDMETHODCALLTYPE *Release)(IwinDropSource* pThis);
  HRESULT (STDMETHODCALLTYPE *QueryContinueDrag)(IwinDropSource* pThis, BOOL fEscapePressed, DWORD dwKeyState);
  HRESULT (STDMETHODCALLTYPE *GiveFeedback)(IwinDropSource* pThis, DWORD dwEffect);
  END_INTERFACE
} IwinDropSourceVtbl;

static HRESULT STDMETHODCALLTYPE IwinDropSource_QueryInterface(IwinDropSource* pThis, REFIID riid, LPVOID *ppvObject)
{
  if(IsEqualGUID(riid, &IID_IUnknown) ||
     IsEqualGUID(riid, &IID_IDropSource))
  {
    InterlockedIncrement(&pThis->lRefCount);
    *ppvObject = pThis;
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE IwinDropSource_AddRef(IwinDropSource* pThis)
{
  return (ULONG)InterlockedIncrement(&pThis->lRefCount);
}

static ULONG STDMETHODCALLTYPE IwinDropSource_Release(IwinDropSource* pThis)
{
  ULONG nCount = (ULONG)InterlockedDecrement(&pThis->lRefCount);
  if(nCount == 0)
  {
    free(pThis);
    return 0;
  }
  return nCount;
}

static HRESULT STDMETHODCALLTYPE IwinDropSource_QueryContinueDrag(IwinDropSource* pThis, BOOL fEscapePressed, DWORD dwKeyState)
{
  /* if the Escape key has been pressed since the last call, cancel the drop */
  if (fEscapePressed)
    return DRAGDROP_S_CANCEL;

  /* if the LeftMouse button has been released, then do the drop! */
  if (!(dwKeyState & MK_LBUTTON))
    return DRAGDROP_S_DROP;

  (void)pThis;
  /* continue with the drag-drop */
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE IwinDropSource_GiveFeedback(IwinDropSource* pThis, DWORD dwEffect)
{
  char* value = iupAttribGet(pThis->ih, "DRAGCURSOR");
  if (value)
  {
    HCURSOR hCur = NULL;

    if (dwEffect & DROPEFFECT_COPY)
    {
      char* copy = iupAttribGet(pThis->ih, "DRAGCURSORCOPY");
      if (copy) 
        hCur = iupwinGetCursor(pThis->ih, copy);
    }

    if (!hCur)
      hCur = iupwinGetCursor(pThis->ih, value);

    if (hCur)
    {
      SetCursor(hCur);
      return S_OK;
    }
  }
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

static IDropSource* winCreateDropSource(Ihandle* ih)
{
  static IwinDropSourceVtbl ids_vtbl = {
    IwinDropSource_QueryInterface,
    IwinDropSource_AddRef,
    IwinDropSource_Release,
    IwinDropSource_QueryContinueDrag,
    IwinDropSource_GiveFeedback};

  IwinDropSource* pDropSource = malloc(sizeof(IwinDropSource));

  pDropSource->ids.lpVtbl = (IDropSourceVtbl*)&ids_vtbl;
  pDropSource->lRefCount = 1;
  pDropSource->ih = ih;

  return (IDropSource*)pDropSource;
}

static void winDestroyDropSource(IDropSource* pSrc)
{
  pSrc->lpVtbl->Release(pSrc);
}


/**********************************************************************************/


typedef struct _IwinDataObject
{
  IDataObject ido;
  LONG lRefCount;
  ULONG nNumFormats;
  FORMATETC* pFormatEtc;
  Ihandle* ih;
} IwinDataObject;

typedef struct _IwinDataObjectVtbl
{
  BEGIN_INTERFACE
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinDataObject* pThis, REFIID riid, void **ppvObject);
  ULONG   (STDMETHODCALLTYPE *AddRef)(IwinDataObject* pThis);
  ULONG   (STDMETHODCALLTYPE *Release)(IwinDataObject* pThis);
  HRESULT (STDMETHODCALLTYPE *GetData)(IwinDataObject* pThis, FORMATETC *pFormatEtcIn, STGMEDIUM *pStgMedium);
  HRESULT (STDMETHODCALLTYPE *GetDataHere)(IwinDataObject* pThis, FORMATETC *pFormatEtc, STGMEDIUM *pStgMedium);
  HRESULT (STDMETHODCALLTYPE *QueryGetData)(IwinDataObject* pThis, FORMATETC *pFormatEtc);
  HRESULT (STDMETHODCALLTYPE *GetCanonicalFormatEtc)(IwinDataObject* pThis, FORMATETC *pFormatEtcIn, FORMATETC *pFormatEtcOut);
  HRESULT (STDMETHODCALLTYPE *SetData)(IwinDataObject* pThis, FORMATETC *pFormatEtc, STGMEDIUM *pStgMedium, BOOL fRelease);
  HRESULT (STDMETHODCALLTYPE *EnumFormatEtc)(IwinDataObject* pThis, DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
  HRESULT (STDMETHODCALLTYPE *DAdvise)(IwinDataObject* pThis, FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
  HRESULT (STDMETHODCALLTYPE *DUnadvise)(IwinDataObject* pThis, DWORD dwConnection);
  HRESULT (STDMETHODCALLTYPE *EnumDAdvise)(IwinDataObject* pThis, IEnumSTATDATA **ppEnumAdvise);
  END_INTERFACE
} IwinDataObjectVtbl;

static ULONG winDataObjectLookupFormatEtc(IwinDataObject* pThis, FORMATETC *pFormatEtc)
{
  ULONG i;
  
  for(i = 0; i < pThis->nNumFormats; i++)
  {
    if((pFormatEtc->tymed & pThis->pFormatEtc[i].tymed) && 
       pFormatEtc->cfFormat == pThis->pFormatEtc[i].cfFormat &&
       pFormatEtc->dwAspect == pThis->pFormatEtc[i].dwAspect)
      return (int)i;
  }

  return (ULONG)-1;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_QueryInterface(IwinDataObject* pThis, REFIID riid, LPVOID *ppvObject)
{
  if(IsEqualGUID(riid, &IID_IUnknown) ||
     IsEqualGUID(riid, &IID_IDataObject))
  {
    InterlockedIncrement(&pThis->lRefCount);
    *ppvObject = pThis;
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE IwinDataObject_AddRef(IwinDataObject* pThis)
{
  return (ULONG)InterlockedIncrement(&pThis->lRefCount);
}

static ULONG STDMETHODCALLTYPE IwinDataObject_Release(IwinDataObject* pThis)
{
  ULONG nCount = (ULONG)InterlockedDecrement(&pThis->lRefCount);
  if(nCount == 0)
  {
    free(pThis->pFormatEtc);
    free(pThis);
    return 0;
  }
  return nCount;
}

static void winGetClipboardFormatName(CLIPFORMAT cf, TCHAR* name, int len);

static HRESULT STDMETHODCALLTYPE IwinDataObject_GetData(IwinDataObject* pThis, LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMedium)
{
  IFns cbDragDataSize;
  IFnsVi cbDragData;
  int size;
  void *pData;
  TCHAR type[256];

  ULONG nIndex = winDataObjectLookupFormatEtc(pThis, pFormatEtc);
  if(nIndex == (ULONG)-1)
    return DV_E_FORMATETC;

  pStgMedium->tymed = TYMED_HGLOBAL;
  pStgMedium->pUnkForRelease  = NULL;

  winGetClipboardFormatName(pFormatEtc->cfFormat, type, 256);

  cbDragDataSize = (IFns)IupGetCallback(pThis->ih, "DRAGDATASIZE_CB");
  if (!cbDragDataSize)
    return STG_E_MEDIUMFULL;

  size = cbDragDataSize(pThis->ih, iupwinStrFromSystem(type));
  if (size <= 0)
    return STG_E_MEDIUMFULL;

  pStgMedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
  if(!pStgMedium->hGlobal)
    return STG_E_MEDIUMFULL;

  pData = GlobalLock(pStgMedium->hGlobal);

  /* fill data */
  cbDragData = (IFnsVi)IupGetCallback(pThis->ih, "DRAGDATA_CB");
  if (cbDragData)
    cbDragData(pThis->ih, iupwinStrFromSystem(type), pData, size);

  GlobalUnlock(pStgMedium->hGlobal);

  return S_OK;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_GetDataHere(IwinDataObject* pThis, LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMedium)
{
  (void)pThis;
  (void)pFormatEtc;
  (void)pStgMedium;
  return DV_E_FORMATETC;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_QueryGetData(IwinDataObject* pThis, LPFORMATETC pFormatEtc)
{
  /*  Called to see if the IDataObject supports the specified format of data */
  return winDataObjectLookupFormatEtc(pThis, pFormatEtc) == -1? DV_E_FORMATETC: S_OK;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_GetCanonicalFormatEtc(IwinDataObject* pThis, LPFORMATETC pFormatEtcIn, LPFORMATETC pFormatEtcOut)
{
  (void)pThis;
  (void)pFormatEtcIn;
  /* Apparently we have to set this field to NULL even though we don't do anything else */
  pFormatEtcOut->ptd = NULL;
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_SetData(IwinDataObject* pThis, LPFORMATETC pFormatEtc, LPSTGMEDIUM pStgMedium, BOOL fRelease)
{
  (void)pThis;
  (void)pFormatEtc;
  (void)pStgMedium;
  (void)fRelease;
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_EnumFormatEtc(IwinDataObject* pThis, DWORD dwDirection, LPENUMFORMATETC *ppEnumFormatEtc)
{
  if(dwDirection != DATADIR_GET)
    return E_NOTIMPL;

#ifdef USE_SHCREATESTDENUMFMTETC
  SHCreateStdEnumFmtEtc(pThis->nNumFormats, pThis->pFormatEtc, ppEnumFormatEtc);
#else
  *ppEnumFormatEtc = (LPENUMFORMATETC)winCreateEnumFORMATETC(pThis->nNumFormats, pThis->pFormatEtc);
#endif
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_DAdvise(IwinDataObject* pThis, LPFORMATETC pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD *pdwConnection)
{
  (void)pThis;
  (void)pFormatetc;
  (void)advf;
  (void)pAdvSink;
  (void)pdwConnection;
  return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_Dunadvise(IwinDataObject* pThis, DWORD dwConnection)
{
  (void)pThis;
  (void)dwConnection;
  return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT STDMETHODCALLTYPE IwinDataObject_EnumDAdvise(IwinDataObject* pThis, LPENUMSTATDATA *ppEnumAdvise)
{
  (void)pThis;
  (void)ppEnumAdvise;
  return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObject* winCreateDataObject(CLIPFORMAT *pClipFormat, ULONG nNumFormats, Ihandle* ih)
{
  IwinDataObject* pDataObject;
  ULONG i;
  static IwinDataObjectVtbl ido_vtbl = {
    IwinDataObject_QueryInterface,
    IwinDataObject_AddRef,
    IwinDataObject_Release,
    IwinDataObject_GetData,
    IwinDataObject_GetDataHere,
    IwinDataObject_QueryGetData,
    IwinDataObject_GetCanonicalFormatEtc,
    IwinDataObject_SetData,
    IwinDataObject_EnumFormatEtc,
    IwinDataObject_DAdvise,
    IwinDataObject_Dunadvise,
    IwinDataObject_EnumDAdvise};

  pDataObject = malloc(sizeof(IwinDataObject));

  pDataObject->ido.lpVtbl = (IDataObjectVtbl*)&ido_vtbl;
  pDataObject->lRefCount  = 1;

  pDataObject->nNumFormats = nNumFormats;
  pDataObject->pFormatEtc = malloc(nNumFormats*sizeof(FORMATETC));
  pDataObject->ih = ih;

  for(i = 0; i < nNumFormats; i++)
  {
    pDataObject->pFormatEtc[i].cfFormat = pClipFormat[i];
    pDataObject->pFormatEtc[i].dwAspect = DVASPECT_CONTENT;
    pDataObject->pFormatEtc[i].ptd = NULL;
    pDataObject->pFormatEtc[i].lindex = -1;
    pDataObject->pFormatEtc[i].tymed = TYMED_HGLOBAL;
  }

  return (IDataObject*)pDataObject;
}

static void winDestroyDataObject(IDataObject* pObj)
{
  pObj->lpVtbl->Release(pObj);
}


/**********************************************************************************/


typedef struct _IwinDropTarget
{
  IDropTarget idt;
  LONG lRefCount;
  BOOL fAllowDrop;
  ULONG nNumFormats;
  CLIPFORMAT* pClipFormat;
  Ihandle* ih;
} IwinDropTarget;

typedef struct _IwinDropTargetVtbl
{
  BEGIN_INTERFACE
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(IwinDropTarget* pThis, REFIID riid, void **ppvObject);
  ULONG   (STDMETHODCALLTYPE *AddRef)(IwinDropTarget* pThis);
  ULONG   (STDMETHODCALLTYPE *Release)(IwinDropTarget* pThis);
  HRESULT (STDMETHODCALLTYPE *DragEnter)(IwinDropTarget* pThis, IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
  HRESULT (STDMETHODCALLTYPE *DragOver)(IwinDropTarget* pThis, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
  HRESULT (STDMETHODCALLTYPE *DragLeave)(IwinDropTarget* pThis);
  HRESULT (STDMETHODCALLTYPE *Drop)(IwinDropTarget* pThis, IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect);
  END_INTERFACE
} IwinDropTargetVtbl;

static BOOL winQueryDataObject(IwinDropTarget* pDropTarget, IDataObject *pDataObject)
{
  ULONG i;
  FORMATETC fmtetc = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  
  /* If there are more than one format, 
     the first one accepted will be used */
  for(i = 0; i < pDropTarget->nNumFormats; i++)
  {
    fmtetc.cfFormat = pDropTarget->pClipFormat[i];

    if(pDataObject->lpVtbl->QueryGetData(pDataObject, &fmtetc) == S_OK)
      return TRUE;
  }

  return FALSE;
}

static DWORD winGetDropEffect(DWORD dwKeyState, DWORD dwOKEffect)
{
  DWORD dwEffect = DROPEFFECT_NONE;

  /* the standard interpretation:
     no modifier -- Default Drop
     CTRL        -- DROPEFFECT_COPY
     SHIFT       -- DROPEFFECT_MOVE
     CTRL-SHIFT  -- DROPEFFECT_LINK (unsupported, defaults to copy) */

  if (dwKeyState & MK_CONTROL)
    dwEffect = DROPEFFECT_COPY;
  else if(dwKeyState & MK_SHIFT)
    dwEffect = DROPEFFECT_MOVE;

  if (dwEffect == DROPEFFECT_NONE)  /* no modifier */
  {
    /* Try in order: MOVE, COPY */
    if(dwOKEffect & DROPEFFECT_MOVE)
      dwEffect = DROPEFFECT_MOVE;
    else if(dwOKEffect & DROPEFFECT_COPY)
      dwEffect = DROPEFFECT_COPY;
  }

  if (!(dwEffect & dwOKEffect))
    return DROPEFFECT_NONE;

  return dwEffect;
}

static HRESULT STDMETHODCALLTYPE IwinDropTarget_QueryInterface(IwinDropTarget* pThis, REFIID riid, LPVOID *ppvObject)
{
  if(IsEqualGUID(riid, &IID_IUnknown) ||
     IsEqualGUID(riid, &IID_IDropTarget))
  {
    InterlockedIncrement(&pThis->lRefCount);
    *ppvObject = pThis;
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE IwinDropTarget_AddRef(IwinDropTarget* pThis)
{
  return (ULONG)InterlockedIncrement(&pThis->lRefCount);
}

static ULONG STDMETHODCALLTYPE IwinDropTarget_Release(IwinDropTarget* pThis)
{
  ULONG nCount = (ULONG)InterlockedDecrement(&pThis->lRefCount);
  if(nCount == 0)
  {
    free(pThis->pClipFormat);
    free(pThis);
    return 0;
  }
  return nCount;
}

static HRESULT STDMETHODCALLTYPE IwinDropTarget_DragEnter(IwinDropTarget* pThis, IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
  pThis->fAllowDrop = winQueryDataObject(pThis, pDataObject);
  if (pThis->fAllowDrop)
    *pdwEffect = winGetDropEffect(dwKeyState, *pdwEffect);
  else
    *pdwEffect = DROPEFFECT_NONE;
  (void)pt;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE IwinDropTarget_DragOver(IwinDropTarget* pThis, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
  if (pThis->fAllowDrop)
  {
    IFniis cbDropMotion = (IFniis)IupGetCallback(pThis->ih, "DROPMOTION_CB");

    *pdwEffect = winGetDropEffect(dwKeyState, *pdwEffect);

    if(cbDropMotion)
    {
      char status[IUPKEY_STATUS_SIZE] = IUPKEY_STATUS_INIT;
      int x = pt.x, y = pt.y;
      iupdrvScreenToClient(pThis->ih, &x, &y);
      iupwinButtonKeySetStatus((WORD)dwKeyState, status, 0);

      cbDropMotion(pThis->ih, x, y, status);
    }
  }
  else
    *pdwEffect = DROPEFFECT_NONE;
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE IwinDropTarget_DragLeave(IwinDropTarget* pThis)
{
  (void)pThis;
  return S_OK;
}

static void winCallDropDataCB(Ihandle* ih, CLIPFORMAT cf, HGLOBAL hData, int x, int y)
{
  IFnsViii cbDropData = (IFnsViii)IupGetCallback((Ihandle*)ih, "DROPDATA_CB");
  if(cbDropData)
  {
    void* targetData = NULL;
    TCHAR type[256];
    SIZE_T size;

    iupdrvScreenToClient(ih, &x, &y);

    targetData = GlobalLock(hData);
    size = GlobalSize(hData);
    if(size == 0 || !targetData)
      return;

    winGetClipboardFormatName(cf, type, 256);

    cbDropData(ih, iupwinStrFromSystem(type), targetData, (int)size, x, y);

    GlobalUnlock(hData);
  }
}

static HRESULT STDMETHODCALLTYPE IwinDropTarget_Drop(IwinDropTarget* pThis, IDataObject *pDataObject, DWORD dwKeyState, POINTL pt, DWORD *pdwEffect)
{
  FORMATETC fmtetc = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  ULONG i;

  if (!pThis->fAllowDrop)
    return S_OK;

  *pdwEffect = winGetDropEffect(dwKeyState, *pdwEffect);
  if (*pdwEffect == DROPEFFECT_NONE)
    return S_OK;

  /* If there are more than one format, 
     the first one accepted will be used */
  for(i = 0; i < pThis->nNumFormats; i++)
  {
    fmtetc.cfFormat = pThis->pClipFormat[i];

    if (pDataObject->lpVtbl->QueryGetData(pDataObject, &fmtetc) == S_OK)
    {
      STGMEDIUM medium;

      pDataObject->lpVtbl->GetData(pDataObject, &fmtetc, &medium);

      winCallDropDataCB(pThis->ih, fmtetc.cfFormat, medium.hGlobal, (int)pt.x, (int)pt.y);

      ReleaseStgMedium(&medium);

      return S_OK;
    }
  }

  return S_OK;
}

static IwinDropTarget* winCreateDropTarget(CLIPFORMAT *pClipFormat, ULONG nNumFormats, Ihandle* ih)
{
  ULONG i;
  static IwinDropTargetVtbl idt_vtbl = {
    IwinDropTarget_QueryInterface,
    IwinDropTarget_AddRef,
    IwinDropTarget_Release,
    IwinDropTarget_DragEnter,
    IwinDropTarget_DragOver,
    IwinDropTarget_DragLeave,
    IwinDropTarget_Drop};

  IwinDropTarget* pDropTarget = malloc(sizeof(IwinDropTarget));

  pDropTarget->idt.lpVtbl = (IDropTargetVtbl*)&idt_vtbl;
  pDropTarget->lRefCount = 1;

  pDropTarget->fAllowDrop = FALSE;
  pDropTarget->nNumFormats = nNumFormats;
  pDropTarget->pClipFormat = malloc(nNumFormats*sizeof(CLIPFORMAT));
  pDropTarget->ih = ih;

  for(i = 0; i < nNumFormats; i++)
    pDropTarget->pClipFormat[i] = pClipFormat[i];

  return pDropTarget;
}

static void winDestroyDropTarget(IwinDropTarget* pDropTarget)
{
  ((IDropTarget*)pDropTarget)->lpVtbl->Release((IDropTarget*)pDropTarget);

  free(pDropTarget->pClipFormat);
  free(pDropTarget);
}


/******************************************************************************************/

static void winGetClipboardFormatName(CLIPFORMAT cf, TCHAR* name, int len)
{
  if (cf == CF_TEXT)
    lstrcpy(name, TEXT("TEXT"));
  else if (cf == CF_BITMAP)
    lstrcpy(name, TEXT("BITMAP"));
  else if (cf == CF_METAFILEPICT)
    lstrcpy(name, TEXT("METAFILEPICT"));
  else if (cf == CF_TIFF)
    lstrcpy(name, TEXT("TIFF"));
  else if (cf == CF_DIB)
    lstrcpy(name, TEXT("DIB"));
  else if (cf == CF_WAVE)
    lstrcpy(name, TEXT("WAVE"));
  else if (cf == CF_UNICODETEXT)
    lstrcpy(name, TEXT("UNICODETEXT"));
  else if (cf == CF_ENHMETAFILE)
    lstrcpy(name, TEXT("ENHMETAFILE"));
  else
    GetClipboardFormatName(cf, name, len);
}

static CLIPFORMAT winRegisterClipboardFormat(const char* name)
{
  if (iupStrEqual(name, "TEXT"))
    return CF_TEXT;
  if (iupStrEqual(name, "BITMAP"))
    return CF_BITMAP;
  if (iupStrEqual(name, "METAFILEPICT"))
    return CF_METAFILEPICT;
  if (iupStrEqual(name, "TIFF"))
    return CF_TIFF;
  if (iupStrEqual(name, "DIB"))
    return CF_DIB;
  if (iupStrEqual(name, "WAVE"))
    return CF_WAVE;
  if (iupStrEqual(name, "UNICODETEXT"))
    return CF_UNICODETEXT;
  if (iupStrEqual(name, "ENHMETAFILE"))
    return CF_ENHMETAFILE;

  return (CLIPFORMAT)RegisterClipboardFormat(iupwinStrToSystem(name));
}

static IwinDropTarget* winRegisterDrop(Ihandle *ih)
{
  Iarray* dropList = (Iarray*)iupAttribGet(ih, "_IUPWIN_DROP_TYPES");
  int i, j, count = iupArrayCount(dropList);
  char** dropListData = (char**)iupArrayGetData(dropList);
  CLIPFORMAT* cfList = malloc(count * sizeof(CLIPFORMAT));
  IwinDropTarget* pDropTarget = NULL;

  j = 0;
  for(i = 0; i < count; i++)
  {
    CLIPFORMAT f = (CLIPFORMAT)winRegisterClipboardFormat(dropListData[i]);
    if (f)
    {
      cfList[j] = f;
      j++;
    }
  }

  if (j)
    pDropTarget = winCreateDropTarget(cfList, (ULONG)j, ih);

  free(cfList);
  return pDropTarget;
}

static int winRegisterProcessDrag(Ihandle *ih)
{
  IDataObject *pObj;
  IDropSource *pSrc;
  Iarray* dragList = (Iarray*)iupAttribGet(ih, "_IUPWIN_DRAG_TYPES");
  int i, j, dragListCount;
  char **dragListData;
  CLIPFORMAT *cfList;
  DWORD dwEffect = 0, dwOKEffect;
  IFns cbDragDataSize = (IFns)IupGetCallback(ih, "DRAGDATASIZE_CB");
  IFnsVi cbDragData = (IFnsVi)IupGetCallback(ih, "DRAGDATA_CB");

  if (!dragList || !cbDragDataSize || !cbDragData)
    return -1;

  dragListCount = iupArrayCount(dragList);
  dragListData = (char**)iupArrayGetData(dragList);

  cfList = malloc(dragListCount * sizeof(CLIPFORMAT));

  /* Register all the drag types. */
  j = 0;
  for(i = 0; i < dragListCount; i++)
  {
    CLIPFORMAT f = (CLIPFORMAT)winRegisterClipboardFormat(dragListData[i]);
    if (f)
    {
      cfList[j] = f;
      j++;
    }
  }

  pSrc = winCreateDropSource(ih);
  pObj = winCreateDataObject(cfList, (ULONG)j, ih);

  /* Process drag, this will stop util drag is done or canceled. */
  dwOKEffect = iupAttribGetBoolean(ih, "DRAGSOURCEMOVE")? DROPEFFECT_MOVE|DROPEFFECT_COPY: DROPEFFECT_COPY;
  DoDragDrop(pObj, pSrc, dwOKEffect, &dwEffect);

  winDestroyDataObject(pObj);
  winDestroyDropSource(pSrc);
  free(cfList);

  if (dwEffect == DROPEFFECT_MOVE)
    return 1;
  else if (dwEffect == DROPEFFECT_COPY)
    return 0;
  else 
    return -1;
}

static int winDragStart(Ihandle* ih, int x, int y)
{
  IFnii cbDragBegin = (IFnii)IupGetCallback(ih, "DRAGBEGIN_CB");
  if (cbDragBegin)
  {
    IFni cbDragEnd;
    int remove;
    iupdrvScreenToClient(ih, &x, &y);
    if (cbDragBegin(ih, x, y) == IUP_IGNORE)
      return 0;

    remove = winRegisterProcessDrag(ih);

    cbDragEnd = (IFni)IupGetCallback(ih, "DRAGEND_CB");
    if (cbDragEnd)
      cbDragEnd(ih, remove);

    return 1;
  }

  return 0;
}

int iupwinDragDetectStart(Ihandle* ih)
{
  POINT pt;
  GetCursorPos(&pt);

  if (DragDetect(ih->handle, pt))
    return winDragStart(ih, pt.x, pt.y);

  return 0;
}

static Iarray* winCreateTypesList(const char* value)
{
  Iarray *newList = iupArrayCreate(10, sizeof(char*));
  char** newListData;
  char valueCopy[256];
  char valueTemp1[256];
  char valueTemp2[256];
  int i = 0;

  strcpy(valueCopy, value);
  while (iupStrToStrStr(valueCopy, valueTemp1, valueTemp2, ',') > 0)
  {
    newListData = (char**)iupArrayInc(newList);
    newListData[i] = iupStrDup(valueTemp1);
    i++;

    if (iupStrEqualNoCase(valueTemp2, valueTemp1))
      break;

    strcpy(valueCopy, valueTemp2);
  }

  if (i == 0)
  {
    iupArrayDestroy(newList);
    return NULL;
  }

  return newList;
}

static void winDestroyTypesList(Iarray *list)
{
  int i, count = iupArrayCount(list);
  char** listData = (char**)iupArrayGetData(list);
  for (i = 0; i<count; i++)
    free(listData[i]);
  iupArrayDestroy(list);
}

static int winSetDropTypesAttrib(Ihandle* ih, const char* value)
{
  Iarray* drop_types_list = (Iarray*)iupAttribGet(ih, "_IUPWIN_DROP_TYPES");
  if (drop_types_list)
  {
    winDestroyTypesList(drop_types_list);
    iupAttribSet(ih, "_IUPWIN_DROP_TYPES", NULL);
  }

  if(!value)
    return 0;

  drop_types_list = winCreateTypesList(value);
  if (drop_types_list)
    iupAttribSet(ih, "_IUPWIN_DROP_TYPES", (char*)drop_types_list);

  return 1;
}

static void winInitOle(void)
{
  if (!IupGetGlobal("_IUPWIN_OLEINITIALIZE"))
  {
    OleInitialize(NULL);
    IupSetGlobal("_IUPWIN_OLEINITIALIZE", "1");
  }
}

static int winSetDropTargetAttrib(Ihandle* ih, const char* value)
{
  IwinDropTarget* pDropTarget;
  Iarray* dropList = (Iarray*)iupAttribGet(ih, "_IUPWIN_DROP_TYPES");
  if (!dropList)
    return 0;

  winInitOle();

  pDropTarget = (IwinDropTarget*)iupAttribGet(ih, "_IUPWIN_DROPTARGET");
  if (pDropTarget)
  {
    RevokeDragDrop(ih->handle);

    CoLockObjectExternal((LPUNKNOWN)pDropTarget, FALSE, TRUE);
    iupAttribSet(ih, "_IUPWIN_DROPTARGET", NULL);

    winDestroyDropTarget(pDropTarget);
  }

  if (iupStrBoolean(value))
  {
    pDropTarget = winRegisterDrop(ih);

    CoLockObjectExternal((LPUNKNOWN)pDropTarget, TRUE, FALSE);
    RegisterDragDrop(ih->handle, (IDropTarget*)pDropTarget);
    iupAttribSet(ih, "_IUPWIN_DROPTARGET", (char*)pDropTarget);
  }

  return 1;
}

static int winSetDragTypesAttrib(Ihandle* ih, const char* value)
{
  Iarray* drag_types_list = (Iarray*)iupAttribGet(ih, "_IUPWIN_DRAG_TYPES");
  if (drag_types_list)
  {
    winDestroyTypesList(drag_types_list);
    iupAttribSet(ih, "_IUPWIN_DRAG_TYPES", NULL);
  }

  if(!value)
    return 0;

  drag_types_list = winCreateTypesList(value);
  if (drag_types_list)
    iupAttribSet(ih, "_IUPWIN_DRAG_TYPES", (char*)drag_types_list);

  return 1;
}

static int winSetDragSourceAttrib(Ihandle* ih, const char* value)
{
  (void)ih;
  if (iupStrBoolean(value))
    winInitOle();
  return 1;
}

static int winSetDragStartAttrib(Ihandle* ih, const char* value)
{
  int x, y;

  if (iupStrToIntInt(value, &x, &y, ',') == 2)
    winDragStart(ih, x, y);

  return 0;
}


/******************************************************************************************/


static int winSetDropFilesTargetAttrib(Ihandle* ih, const char* value)
{
  if (iupStrBoolean(value))
    DragAcceptFiles(ih->handle, TRUE);
  else
    DragAcceptFiles(ih->handle, FALSE);
  return 1;
}

void iupwinDropFiles(HDROP hDrop, Ihandle *ih)
{
  /* called for a WM_DROPFILES */
  TCHAR* filename;
  char* str;
  int i, numFiles, numchar, ret;
  POINT point;

  IFnsiii cb = (IFnsiii)IupGetCallback(ih, "DROPFILES_CB");
  if (!cb) return; 

  numFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

  DragQueryPoint(hDrop, &point);  
  for (i = 0; i < numFiles; i++)
  {
    numchar = DragQueryFile(hDrop, i, NULL, 0);
    filename = (TCHAR*)malloc((numchar+1)*sizeof(TCHAR)); 
    if (!filename)
      break;

    DragQueryFile(hDrop, i, filename, numchar+1);

    str = iupwinStrFromSystemFilename(filename);
    memcpy(filename, str, strlen(str) + 1);
    str = (char*)filename;

    ret = cb(ih, str, numFiles-i-1, (int) point.x, (int) point.y); 

    free(filename);

    if (ret == IUP_IGNORE)
      break;
  }
  DragFinish(hDrop);
}


/******************************************************************************************/

void iupwinDestroyDragDrop(Ihandle* ih)
{
  IwinDropTarget* pDropTarget;

  Iarray* list = (Iarray*)iupAttribGet(ih, "_IUPWIN_DRAG_TYPES");
  if (list)
    winDestroyTypesList(list);

  list = (Iarray*)iupAttribGet(ih, "_IUPWIN_DROP_TYPES");
  if (list)
    winDestroyTypesList(list);

  pDropTarget = (IwinDropTarget*)iupAttribGet(ih, "_IUPWIN_DROPTARGET");
  if (pDropTarget)
  {
    CoLockObjectExternal((LPUNKNOWN)pDropTarget, FALSE, TRUE);
    iupAttribSet(ih, "_IUPWIN_DROPTARGET", NULL);

    winDestroyDropTarget(pDropTarget);
  }
}

void iupdrvRegisterDragDropAttrib(Iclass* ic)
{
  iupClassRegisterCallback(ic, "DROPFILES_CB", "siii");

  iupClassRegisterCallback(ic, "DRAGBEGIN_CB", "ii");
  iupClassRegisterCallback(ic, "DRAGDATASIZE_CB", "s");
  iupClassRegisterCallback(ic, "DRAGDATA_CB", "sVi");
  iupClassRegisterCallback(ic, "DRAGEND_CB", "i");
  iupClassRegisterCallback(ic, "DROPDATA_CB", "sViii");
  iupClassRegisterCallback(ic, "DROPMOTION_CB", "iis");

  iupClassRegisterAttribute(ic, "DRAGTYPES",  NULL, winSetDragTypesAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTYPES",  NULL, winSetDropTypesAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCE", NULL, winSetDragSourceAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPTARGET", NULL, winSetDropTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSOURCEMOVE", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSOR", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGCURSORCOPY", NULL, NULL, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DRAGSTART", NULL, winSetDragStartAttrib, NULL, NULL, IUPAF_NO_INHERIT);  /* Windows Only */

  iupClassRegisterAttribute(ic, "DRAGDROP", NULL, winSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DROPFILESTARGET", NULL, winSetDropFilesTargetAttrib, NULL, NULL, IUPAF_NO_INHERIT);
}
