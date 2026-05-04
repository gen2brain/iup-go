/** \file
 * \brief Thread / Mutex (iOS UIKit)
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "iup.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_thread.h"


typedef struct
{
	pthread_t tid;
	Ihandle* ih;
	Icallback cb;
	char* name;
} IupCocoaTouchThread;

static void* cocoaTouchThreadTrampoline(void* arg)
{
	IupCocoaTouchThread* t = (IupCocoaTouchThread*)arg;
	if (!t) return NULL;

	/* Darwin pthread_setname_np takes only one arg, current thread */
	if (t->name) pthread_setname_np(t->name);

	@autoreleasepool {
		if (t->cb && t->ih) t->cb(t->ih);
	}
	return NULL;
}

IUP_SDK_API void* iupdrvThreadStart(Ihandle* ih)
{
	Icallback cb = IupGetCallback(ih, "THREAD_CB");
	if (!cb) return NULL;

	IupCocoaTouchThread* t = (IupCocoaTouchThread*)calloc(1, sizeof(IupCocoaTouchThread));
	if (!t) return NULL;
	t->ih = ih;
	t->cb = cb;

	const char* name = iupAttribGet(ih, "THREADNAME");
	if (name) t->name = strdup(name);

	if (pthread_create(&t->tid, NULL, cocoaTouchThreadTrampoline, t) != 0)
	{
		free(t->name);
		free(t);
		return NULL;
	}
	return t;
}

IUP_SDK_API void iupdrvThreadJoin(void* handle)
{
	if (!handle) return;
	IupCocoaTouchThread* t = (IupCocoaTouchThread*)handle;
	pthread_join(t->tid, NULL);
}

IUP_SDK_API void iupdrvThreadDestroy(void* handle)
{
	if (!handle) return;
	IupCocoaTouchThread* t = (IupCocoaTouchThread*)handle;
	pthread_detach(t->tid);
	free(t->name);
	free(t);
}

IUP_SDK_API void iupdrvThreadYield(void)
{
	sched_yield();
}

IUP_SDK_API int iupdrvThreadIsCurrent(void* handle)
{
	if (!handle) return 0;
	IupCocoaTouchThread* t = (IupCocoaTouchThread*)handle;
	return pthread_equal(t->tid, pthread_self()) ? 1 : 0;
}

IUP_SDK_API void iupdrvThreadExit(int code)
{
	pthread_exit((void*)(intptr_t)code);
}

IUP_SDK_API void* iupdrvMutexCreate(void)
{
	pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	if (!m) return NULL;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(m, &attr);
	pthread_mutexattr_destroy(&attr);
	return m;
}

IUP_SDK_API void iupdrvMutexLock(void* handle)
{
	if (handle) pthread_mutex_lock((pthread_mutex_t*)handle);
}

IUP_SDK_API void iupdrvMutexUnlock(void* handle)
{
	if (handle) pthread_mutex_unlock((pthread_mutex_t*)handle);
}

IUP_SDK_API void iupdrvMutexDestroy(void* handle)
{
	if (!handle) return;
	pthread_mutex_destroy((pthread_mutex_t*)handle);
	free(handle);
}
