/** \file
 * \brief Video engines, threads and library loading
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPVIDEO_OS_H
#define __IUPVIDEO_OS_H

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>

typedef SRWLOCK ivideo_mutex_t;
typedef CONDITION_VARIABLE ivideo_cond_t;
typedef HANDLE ivideo_thread_t;

#define IVIDEO_THREAD(name, arg) static unsigned __stdcall name(void* arg)
#define IVIDEO_THREAD_END return 0

static inline int ivideoMutexInit(ivideo_mutex_t* mutex) { InitializeSRWLock(mutex); return 1; }
static inline void ivideoMutexDestroy(ivideo_mutex_t* mutex) { (void)mutex; }
static inline void ivideoMutexLock(ivideo_mutex_t* mutex) { AcquireSRWLockExclusive(mutex); }
static inline void ivideoMutexUnlock(ivideo_mutex_t* mutex) { ReleaseSRWLockExclusive(mutex); }
static inline int ivideoCondInit(ivideo_cond_t* cond) { InitializeConditionVariable(cond); return 1; }
static inline void ivideoCondDestroy(ivideo_cond_t* cond) { (void)cond; }
static inline void ivideoCondWait(ivideo_cond_t* cond, ivideo_mutex_t* mutex) { SleepConditionVariableSRW(cond, mutex, INFINITE, 0); }
static inline void ivideoCondSignal(ivideo_cond_t* cond) { WakeConditionVariable(cond); }

static inline int ivideoThreadStart(ivideo_thread_t* thread, unsigned (__stdcall* proc)(void*), void* arg)
{
  *thread = (HANDLE)_beginthreadex(NULL, 0, proc, arg, 0, NULL);
  return *thread != NULL;
}

static inline void ivideoThreadJoin(ivideo_thread_t thread)
{
  WaitForSingleObject(thread, INFINITE);
  CloseHandle(thread);
}

static inline void* ivideoAlignedAlloc(size_t alignment, size_t size)
{
  return _aligned_malloc(size, alignment);
}

static inline void ivideoAlignedFree(void* memory)
{
  _aligned_free(memory);
}

static inline void* ivideoLibraryOpen(const char* name)
{
  return (void*)LoadLibraryA(name);
}

static inline void* ivideoLibraryOpenIn(const wchar_t* dir, const char* name)
{
  wchar_t path[MAX_PATH];
  size_t length = wcslen(dir);
  if (length == 0 || length + strlen(name) + 2 > MAX_PATH)
    return NULL;
  memcpy(path, dir, length * sizeof(wchar_t));
  if (path[length - 1] != L'\\')
    path[length++] = L'\\';
  MultiByteToWideChar(CP_UTF8, 0, name, -1, path + length, (int)(MAX_PATH - length));
  return (void*)LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
}

static inline void* ivideoLibrarySymbol(void* library, const char* name)
{
  return (void*)GetProcAddress((HMODULE)library, name);
}

static inline void ivideoLibraryClose(void* library)
{
  FreeLibrary((HMODULE)library);
}

static inline int ivideoRegistryDir(HKEY root, const wchar_t* key, const wchar_t* value, wchar_t* dir, DWORD size)
{
  DWORD bytes = size * sizeof(wchar_t);
  return RegGetValueW(root, key, value, RRF_RT_REG_SZ, NULL, dir, &bytes) == ERROR_SUCCESS && dir[0];
}

static inline char* ivideoFullPath(const char* path)
{
  wchar_t wide[MAX_PATH], full[MAX_PATH];
  char* result;
  int length;
  if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, MAX_PATH))
    return NULL;
  if (!_wfullpath(full, wide, MAX_PATH) || GetFileAttributesW(full) == INVALID_FILE_ATTRIBUTES)
    return NULL;
  length = WideCharToMultiByte(CP_UTF8, 0, full, -1, NULL, 0, NULL, NULL);
  result = length > 0 ? (char*)malloc((size_t)length) : NULL;
  if (result)
    WideCharToMultiByte(CP_UTF8, 0, full, -1, result, length, NULL, NULL);
  return result;
}
#else
#include <dlfcn.h>
#include <pthread.h>

typedef pthread_mutex_t ivideo_mutex_t;
typedef pthread_cond_t ivideo_cond_t;
typedef pthread_t ivideo_thread_t;

#define IVIDEO_THREAD(name, arg) static void* name(void* arg)
#define IVIDEO_THREAD_END return NULL

static inline int ivideoMutexInit(ivideo_mutex_t* mutex) { return pthread_mutex_init(mutex, NULL) == 0; }
static inline void ivideoMutexDestroy(ivideo_mutex_t* mutex) { pthread_mutex_destroy(mutex); }
static inline void ivideoMutexLock(ivideo_mutex_t* mutex) { pthread_mutex_lock(mutex); }
static inline void ivideoMutexUnlock(ivideo_mutex_t* mutex) { pthread_mutex_unlock(mutex); }
static inline int ivideoCondInit(ivideo_cond_t* cond) { return pthread_cond_init(cond, NULL) == 0; }
static inline void ivideoCondDestroy(ivideo_cond_t* cond) { pthread_cond_destroy(cond); }
static inline void ivideoCondWait(ivideo_cond_t* cond, ivideo_mutex_t* mutex) { pthread_cond_wait(cond, mutex); }
static inline void ivideoCondSignal(ivideo_cond_t* cond) { pthread_cond_signal(cond); }

static inline int ivideoThreadStart(ivideo_thread_t* thread, void* (*proc)(void*), void* arg)
{
  return pthread_create(thread, NULL, proc, arg) == 0;
}

static inline void ivideoThreadJoin(ivideo_thread_t thread)
{
  pthread_join(thread, NULL);
}

static inline void* ivideoAlignedAlloc(size_t alignment, size_t size)
{
  void* memory;
  return posix_memalign(&memory, alignment, size) == 0 ? memory : NULL;
}

static inline void ivideoAlignedFree(void* memory)
{
  free(memory);
}

static inline void* ivideoLibraryOpen(const char* name)
{
  return dlopen(name, RTLD_NOW | RTLD_LOCAL);
}

static inline void* ivideoLibrarySymbol(void* library, const char* name)
{
  return dlsym(library, name);
}

static inline void ivideoLibraryClose(void* library)
{
  dlclose(library);
}

static inline char* ivideoFullPath(const char* path)
{
  return realpath(path, NULL);
}
#endif

#endif
