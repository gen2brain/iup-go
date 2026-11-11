## IupThread

Creates a thread element in IUP, which is not associated to any interface element.
Creates and manages threads in a multithread environment.

It inherits from [IupUser](iup_user.md).

### Creation

    Ihandle* IupThread(void);

**Returns:** the identifier of the created element, or NULL if an error occurs.

### Attributes

**START** (write-only) (non-inheritable): starts the thread and calls the callback.
Can be YES only. The thread exits when the callback is terminated.

**EXIT** (write-only) (non-inheritable): exits the current thread. Value contains the exit code.

**ISCURRENT** (read-only) (non-inheritable): returns whether the started thread is the current thread.

**YIELD** (write-only) (non-inheritable): yields execution to another thread. Value is ignored.

**JOIN** (write-only) (non-inheritable): Waits until the thread finishes. Value is ignored.

**THREADNAME** (non-inheritable): name of the native thread, shown by debuggers and system tools. Must be set before START.
In Linux and Android, only the first 15 bytes are used.

**LOCK** (non-inheritable): uses a mutex to create a lock to allow access to shared data.
Can be YES or NO. When set to YES the mutex will be locked, when set to NO the mutex will be unlocked.
It does not depend on whether the thread is started or not.

### Callbacks

**THREAD_CB**: Action generated when the thread is started.
If this callback returns or does not exist, the thread is terminated.

    int function(Ihandle* ih);

**ih**: identifier of the element that activated the event.

In WebAssembly, Emscripten POSIX threads are used; under the Go binding the callback runs as a goroutine.
