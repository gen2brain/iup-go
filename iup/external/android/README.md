# IUP for Android

This directory is the Android-side Gradle project that pairs with the IUP C driver and the Go bindings.

Two independent build flows are supported:

1. **Go flow (primary)** - an IUP app is a Go program compiled as `-buildmode=c-shared`, producing a single `.so` that Gradle picks up from `jniLibs/`.
2. **C flow (optional)** - Gradle drives CMake via `externalNativeBuild` and builds the stock IUP C library (`libiup.so`) alongside the Java bridge. Enable with `-Piup.buildC`.

## Project layout

```
iup/external/android/
├── build.gradle         root build script (SDK levels, ABI defaults)
├── settings.gradle      library + iupapp modules
├── library/             AAR: IupLaunchActivity, IupActivity, *Helper Java classes
└── iupapp/              example APK consuming the library
```

## Prerequisites

* Android SDK installed
* Android NDK (r23 or newer)
* JDK 17+ (required by Android Gradle Plugin 9.x)
* Go 1.21 or newer (for the Go flow)

### Environment

Standard names. Gradle and the NDK honor these:

```sh
export ANDROID_SDK_ROOT=/opt/android-sdk    # preferred
export ANDROID_HOME=$ANDROID_SDK_ROOT       # legacy alias; still accepted
export ANDROID_NDK_HOME=/opt/android-ndk
# or let Gradle auto-find the NDK under $ANDROID_SDK_ROOT/ndk/<version>
```

For the Go flow, put the NDK's clang wrappers on `PATH`:

```sh
export PATH=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
```

The wrappers are named `<triple><api>-clang`, e.g. `aarch64-linux-android22-clang`.

## Go flow

Use the same `main()` on desktop and Android. The iup-go `init()` opens IUP before yours runs; `iup.EntryPoint(main)` in your own `init()` registers
`main` as the callback `IupLaunchActivity` fires after `System.loadLibrary()`. On desktop `EntryPoint` is a no-op and the Go runtime calls `main` directly.

```go
// main.go
package main

import "github.com/gen2brain/iup-go/iup"

func init() { iup.EntryPoint(main) }

func main() {
    iup.Open()
    defer iup.Close()           // no-op on Android: UI keeps running

    dlg := iup.Dialog(iup.Button("Hello, Android!"))
    dlg.SetAttribute("TITLE", "IUP-Go")
    iup.Show(dlg)

    iup.MainLoop()              // no-op on Android: Java owns the loop
}
```

`iup.Open()` returns `NOERROR` on the second call, so the same source compiles and runs on desktop unchanged.

**Heads-up on Android lifecycle:** `iup.MainLoop()` is a no-op on Android, so code after it runs immediately, not at app shutdown.
Do NOT `iup.Destroy()` timers, threads, or dialogs after `MainLoop()` on Android. Gate such a cleanup with `if iup.GetGlobal("DRIVER") != "Android" { ... }`
or skip it entirely (the Activity handles teardown).

Build a c-shared `.so` per ABI you care about:

```sh
CGO_ENABLED=1 \
CC=aarch64-linux-android22-clang \
CXX=aarch64-linux-android22-clang++ \
GOOS=android GOARCH=arm64 \
  go build -buildmode=c-shared \
    -ldflags='-s -w -extldflags=-Wl,-soname,libmyapp.so' \
    -o libmyapp.so .
```

* `-s -w` strips the symbol table and DWARF info. Saves ~2 MB per ABI. Drop it during development so Android Studio's lldb can step through the native library.
* `-extldflags=-Wl,-soname,libmyapp.so` embeds a `DT_SONAME` matching the filename. Android's dynamic linker uses `SONAME` to deduplicate loaded
  libraries; without it, API 24+ emits warnings and some load/dlopen flows behave unexpectedly. Go's `-buildmode=c-shared` does not set it by default.

Drop the result into the Gradle app project:

```
iupapp/src/main/jniLibs/arm64-v8a/libmyapp.so
```

Subclass `IupLaunchActivity` on the Java side to declare the library and the
entry-point `.so`:

```java
public class MyIupLaunchActivity extends IupLaunchActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "myapp" };
    }

    @Override
    public String getEntryPointLibraryName() {
        return "libmyapp.so";
    }
}
```

Then build the APK:

```sh
cd iup/external/android
./gradlew :iupapp:assembleDebug
```

Repeat the Go build for additional ABIs (`armv7a-linux-androideabi21-clang` for `armeabi-v7a`, `i686-linux-android21-clang` for `x86`,
`x86_64-linux-android21-clang` for `x86_64`) and drop each into the matching `jniLibs/<abi>/` subdirectory.

### One-shot helper: `build-android.sh`

For iterating on an examples end-to-end, the script in this directory wraps the Go c-shared build, `jniLibs` placement, Gradle invocation, and optional
`adb install` / logcat / screenshot capture:

```sh
./build-android.sh -f -l -i -s -n iupapp ../../../examples/mobile_sample 
```

`-n <name>` must match `MyIupLaunchActivity.getLibraries()` / `getEntryPointLibraryName()` (`iupapp` for the bundled example app).
Run `./build-android.sh -h` for the full flag list.

### Release signing

Pass `-r` for `assembleRelease`. Signing is wired through Gradle: env vars below are forwarded as `-PandroidKeystore=...` properties consumed by `iupapp/build.gradle`'s `signingConfigs.release`.

```sh
export ANDROID_KEYSTORE=/path/to/release.jks
export ANDROID_KEYSTORE_PASS=...
export ANDROID_KEY_ALIAS=...
export ANDROID_KEY_PASS=...   # optional, defaults to ANDROID_KEYSTORE_PASS

./build-android.sh -r /path/to/examples/myapp
```

Without those env vars, `-r` still produces a release APK but signed with the Gradle debug key. Use `-t :iupapp:bundleRelease` for an `.aab` instead.

## C flow (optional)

If you want Gradle to build `libiup.so` from the IUP C sources (for a pure-C Android consumer), invoke with:

```sh
./gradlew :iupapp:assembleDebug -Piup.buildC
```

This activates `externalNativeBuild` in `library/build.gradle`, which points at `../CMakeLists.txt` with `-DIUP_BACKEND=android`.
The recipe is in `../cmake/IUPDriverAndroid.cmake`.

## ABI filters

`build.gradle` reads `abiFilters` from `local.properties` to let you narrow the target ABI set during development:

```
# iup/external/android/local.properties
abiFilters=arm64-v8a
```

Unset, the default is all four ABIs (`armeabi-v7a arm64-v8a x86 x86_64`).

## Permissions

The library's `AndroidManifest.xml` declares **no** `<uses-permission>` entries. IUP features that need Android permissions list them below; the consumer app declares only what its code actually uses.

| IUP feature                                                | Permission                              | Notes                                                                                                                                                                  |
|------------------------------------------------------------|-----------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `IupNotify` with `STYLE="NOTIFICATION"`                    | `android.permission.POST_NOTIFICATIONS` | API 33+ runtime permission. On earlier APIs the declaration is harmless. Without it, `iupdrvNotifyShow` fires `ERROR_CB` with `"Notification permission denied"`.      |
| `IupNotify` with `STYLE="TOAST"`                           | none                                    | No permission needed.                                                                                                                                                  |
| Network-using examples (`postmessage`, web controls, etc.) | `android.permission.INTERNET`           | Standard Android requirement.                                                                                                                                          |
| `IupFileDlg` (SAF pickers)                                 | none                                    | No permission needed; SAF grants per-document access automatically. `takePersistableUriPermission` is called on the returned URI so recent-files work across sessions. |
| `IupHelp` / `IupExecute` opening URLs                      | none                                    | Covered by `<queries>` entries in the library manifest (merged into the app).                                                                                          |

Add permissions to your app's `AndroidManifest.xml`, i.e.:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <uses-permission android:name="android.permission.INTERNET"/>
    <uses-permission android:name="android.permission.POST_NOTIFICATIONS"/>
    ...
</manifest>
```

The bundled `iupapp/` test declares both because the shipped examples exercise both features.
