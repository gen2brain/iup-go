/** \file
 * \brief Media Foundation timed text interfaces
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWIN_TIMEDTEXT_H
#define __IUPWIN_TIMEDTEXT_H

enum { IWINVIDEO_CUE_ACTIVE, IWINVIDEO_CUE_INACTIVE, IWINVIDEO_CUE_CLEAR };

struct IwinVideoTextTrack;
struct IwinVideoTextTrackList;
struct IwinVideoTextCueList;
struct IwinVideoTextCue;
struct IwinVideoTextLine;
struct IwinVideoTextStyle;
struct IwinVideoTextRegion;
struct IwinVideoTextBinary;
struct IwinVideoTextNotify;

struct IwinVideoText : IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE RegisterNotifications(IwinVideoTextNotify*) = 0;
  virtual HRESULT STDMETHODCALLTYPE SelectTrack(DWORD, BOOL) = 0;
  virtual HRESULT STDMETHODCALLTYPE AddDataSource(IMFByteStream*, LPCWSTR, LPCWSTR, int, BOOL, DWORD*) = 0;
  virtual HRESULT STDMETHODCALLTYPE AddDataSourceFromUrl(LPCWSTR, LPCWSTR, LPCWSTR, int, BOOL, DWORD*) = 0;
  virtual HRESULT STDMETHODCALLTYPE AddTrack(LPCWSTR, LPCWSTR, int, IwinVideoTextTrack**) = 0;
  virtual HRESULT STDMETHODCALLTYPE RemoveTrack(IwinVideoTextTrack*) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetCueTimeOffset(double*) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetCueTimeOffset(double) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetTracks(IwinVideoTextTrackList**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetActiveTracks(IwinVideoTextTrackList**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetTextTracks(IwinVideoTextTrackList**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetMetadataTracks(IwinVideoTextTrackList**) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetInBandEnabled(BOOL) = 0;
  virtual BOOL STDMETHODCALLTYPE IsInBandEnabled() = 0;
};

struct IwinVideoTextTrack : IUnknown
{
  virtual DWORD STDMETHODCALLTYPE GetId() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetLabel(LPWSTR*) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetLabel(LPCWSTR) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetLanguage(LPWSTR*) = 0;
  virtual int STDMETHODCALLTYPE GetTrackKind() = 0;
  virtual BOOL STDMETHODCALLTYPE IsInBand() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInBandMetadataTrackDispatchType(LPWSTR*) = 0;
  virtual BOOL STDMETHODCALLTYPE IsActive() = 0;
  virtual int STDMETHODCALLTYPE GetErrorCode() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetExtendedErrorCode() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetDataFormat(GUID*) = 0;
  virtual int STDMETHODCALLTYPE GetReadyState() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetCueList(IwinVideoTextCueList**) = 0;
};

struct IwinVideoTextTrackList : IUnknown
{
  virtual DWORD STDMETHODCALLTYPE GetLength() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetTrack(DWORD, IwinVideoTextTrack**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetTrackById(DWORD, IwinVideoTextTrack**) = 0;
};

struct IwinVideoTextCue : IUnknown
{
  virtual DWORD STDMETHODCALLTYPE GetId() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetOriginalId(LPWSTR*) = 0;
  virtual int STDMETHODCALLTYPE GetCueKind() = 0;
  virtual double STDMETHODCALLTYPE GetStartTime() = 0;
  virtual double STDMETHODCALLTYPE GetDuration() = 0;
  virtual DWORD STDMETHODCALLTYPE GetTrackId() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetData(IwinVideoTextBinary**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetRegion(IwinVideoTextRegion**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetStyle(IwinVideoTextStyle**) = 0;
  virtual DWORD STDMETHODCALLTYPE GetLineCount() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetLine(DWORD, IwinVideoTextLine**) = 0;
};

struct IwinVideoTextLine : IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE GetText(LPWSTR*) = 0;
  virtual DWORD STDMETHODCALLTYPE GetSubformattingCount() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetSubformatting(DWORD, DWORD*, DWORD*, IwinVideoTextStyle**) = 0;
};

struct IwinVideoTextCueList : IUnknown
{
  virtual DWORD STDMETHODCALLTYPE GetLength() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetCueByIndex(DWORD, IwinVideoTextCue**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetCueById(DWORD, IwinVideoTextCue**) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetCueByOriginalId(LPCWSTR, IwinVideoTextCue**) = 0;
};

struct IwinVideoTextNotify : IUnknown
{
  virtual void STDMETHODCALLTYPE TrackAdded(DWORD) = 0;
  virtual void STDMETHODCALLTYPE TrackRemoved(DWORD) = 0;
  virtual void STDMETHODCALLTYPE TrackSelected(DWORD, BOOL) = 0;
  virtual void STDMETHODCALLTYPE TrackReadyStateChanged(DWORD) = 0;
  virtual void STDMETHODCALLTYPE Error(int, HRESULT, DWORD) = 0;
  virtual void STDMETHODCALLTYPE Cue(int, double, IwinVideoTextCue*) = 0;
  virtual void STDMETHODCALLTYPE Reset() = 0;
};

static const GUID iwinVideoTextService = {0x805ea411, 0x92e0, 0x4e59, {0x9b, 0x6e, 0x5c, 0x7d, 0x79, 0x15, 0xe6, 0x4f}};
static const GUID iwinVideoTextIID = {0x1f2a94c9, 0xa3df, 0x430d, {0x9d, 0x0f, 0xac, 0xd8, 0x5d, 0xdc, 0x29, 0xaf}};
static const GUID iwinVideoTextNotifyIID = {0xdf6b87b6, 0xce12, 0x45db, {0xab, 0xa7, 0x43, 0x2f, 0xe0, 0x54, 0xe5, 0x7d}};

#endif
