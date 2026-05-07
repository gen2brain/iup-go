## IupGetGlobalInfo

Returns the registered metadata of a global attribute: flag bits and the set of drivers that expose it.

### Parameters/Return

    int IupGetGlobalInfo(const char* name, int* flags, int* drivers);

**name**: global attribute name.\
**flags**: receives a bitwise OR of `IUPGF_*` flags. NULL to ignore.\
**drivers**: receives a bitwise OR of `IUPDRV_*` driver bits indicating which drivers implement this global. NULL to ignore.

**Returns:** 1 if the name is registered, 0 if it is not (output zero-filled), -1 if `name` is NULL.

### Flags

| Flag | Value | Meaning |
|---|---|---|
| IUPGF_NORMAL | 0 | normal read/write string global |
| IUPGF_READONLY | 1 | cannot be set |
| IUPGF_POINTER | 2 | value is an opaque pointer (returned by IupGetGlobal as a string of bytes) |

### Drivers

| Bit | Value | Driver |
|---|---|---|
| IUPDRV_WIN | 1 | Win32 |
| IUPDRV_MOTIF | 2 | Motif |
| IUPDRV_GTK | 4 | GTK |
| IUPDRV_COCOA | 8 | Cocoa (macOS / GNUstep) |
| IUPDRV_QT | 16 | Qt |
| IUPDRV_GTK4 | 32 | GTK4 |
| IUPDRV_EFL | 64 | EFL |
| IUPDRV_WINUI | 128 | WinUI 3 |
| IUPDRV_FLTK | 256 | FLTK |
| IUPDRV_ANDROID | 512 | Android |
| IUPDRV_COCOATOUCH | 1024 | iOS (CocoaTouch) |

A binding can compare `drivers` against the bit for the current driver (matched from `IupGetGlobal("DRIVER")`) to know whether a global is supported on this build.

### See Also

[IupGetAllGlobals](iup_getallglobals.md), [IupSetGlobal](iup_setglobal.md), [IupGetGlobal](iup_getglobal.md)
