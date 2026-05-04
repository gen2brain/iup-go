# IUP for iOS

This directory pairs the iOS (CocoaTouch) C driver with build helper and app-bundle templates.

Two independent build flows are supported:

1. **Go flow on macOS (native)** - Apple's clang + codesign, no third-party tooling.
2. **Go flow on Linux (cross-compile)** - osxcross + zsign + go-ios.

## Project layout

```
iup/external/ios/
├── build-ios.sh          native macOS build/sign/install (Apple toolchain)
├── build-ios-cross.sh    end-to-end Linux cross-compile pipeline
├── build-framework.sh    native macOS XCFramework producer
├── iupapp/               app-bundle template (Info.plist, entitlements)
└── library/              IUP.xcframework metadata (Info.plist.in, modulemap)
```

## Prerequisites

### macOS

* Xcode 14+ (for iOS 15 deployment target)
* Xcode Command Line Tools (`xcode-select --install`)
* Go 1.21+

### Linux (cross-compile)

* osxcross with iPhoneOS.sdk and iPhoneSimulator.sdk under `/opt/osxcross/target/SDK/`
* `arm64-apple-ios-clang` and `x86_64-apple-ios-simulator-clang` [wrapper scripts](https://gist.github.com/gen2brain/490dec4a39e57aa0fd59c108f3120023) on PATH (under `/opt/osxcross/target/bin/`)
* `zsign` for ad-hoc signing
* `go-ios` for install/launch/log capture

iOS 17+ requires a one-time persistent tunnel daemon for go-ios's developer services:

```sh
sudo ios tunnel start &
```

## Go flow

Use the same `main()` on desktop and iOS. The iup-go `init()` opens IUP before yours runs; `iup.EntryPoint(main)` registers `main` as the callback `IupAppDelegate` fires after `application:didFinishLaunchingWithOptions:`.
On desktop `EntryPoint` is a no-op and the Go runtime calls `main` directly.

```go
// main.go
package main

import "github.com/gen2brain/iup-go/iup"

func init() { iup.EntryPoint(main) }

func main() {
    iup.Open()
    defer iup.Close()           // no-op on iOS: UIApplicationMain keeps running

    dlg := iup.Dialog(iup.Button("Hello, iOS!"))
    dlg.SetAttribute("TITLE", "IUP-Go")
    iup.Show(dlg)

    iup.MainLoop()              // calls UIApplicationMain; never returns on iOS
}
```

`iup.Open()` returns `NOERROR` on the second call, so the same source compiles and runs on desktop unchanged.

**Heads-up on iOS lifecycle:** code after `iup.MainLoop()` is unreachable on iOS because `UIApplicationMain` does not return. Do NOT `iup.Destroy()` timers, threads, or dialogs after `MainLoop()`. Gate such cleanup with `if iup.GetGlobal("DRIVER") != "CocoaTouch" { ... }` or skip it (UIKit handles teardown). Same caveat as Android.

### Building the Go binary

**macOS (native):**

```sh
SDK=$(xcrun -sdk iphoneos --show-sdk-path)
CGO_ENABLED=1 GOOS=ios GOARCH=arm64 \
  CC=$(xcrun -sdk iphoneos -find clang) \
  CGO_CFLAGS="-isysroot $SDK -target arm64-apple-ios15.0" \
  CGO_LDFLAGS="-isysroot $SDK -target arm64-apple-ios15.0" \
  go build -trimpath -ldflags='-s -w' -o myapp .
```

**Linux (cross-compile):**

```sh
CGO_ENABLED=1 GOOS=ios GOARCH=arm64 \
  CC=arm64-apple-ios-clang CXX=arm64-apple-ios-clang++ \
  go build -trimpath -ldflags='-s -w' -o myapp .
```

The `arm64-apple-ios-clang` wrapper handles the iOS triple, sysroot, and ld64 plumbing that `xcrun` does on macOS.

For the iOS Simulator, use `iphonesimulator` SDK on macOS (with `-target <host-arch>-apple-ios15.0-simulator`) or the `x86_64-apple-ios-simulator-clang` wrapper on Linux, and set `GOARCH=amd64` (or arm64 on Apple Silicon).

### Bundling, signing, installing

The Go binary IS the iOS executable; wrap it in a `Payload/<name>.app/` bundle with an `Info.plist`:

```sh
mkdir -p Payload/myapp.app
mv myapp Payload/myapp.app/
APP_NAME=myapp BUNDLE_ID=com.example.myapp APP_VERSION=1.0 APP_BUILD=1 MIN_IOS=15.0 \
  envsubst < iup/external/ios/iupapp/Info.plist > Payload/myapp.app/Info.plist
```

**macOS** uses `codesign` + `xcrun simctl/devicectl`:

```sh
# Device:
cp ~/dev.mobileprovision Payload/myapp.app/embedded.mobileprovision
codesign --force --sign "Apple Development: Name (TEAMID)" \
  --entitlements iup/external/ios/iupapp/entitlements.plist Payload/myapp.app
zip -qry myapp.ipa Payload
xcrun devicectl device install app --device <udid> myapp.ipa
xcrun devicectl device process launch --device <udid> com.example.myapp

# Simulator (no real signing needed):
codesign --force --sign - Payload/myapp.app
xcrun simctl install booted Payload/myapp.app
xcrun simctl launch booted com.example.myapp
```

**Linux** uses `zsign` + `go-ios`:

```sh
zsign -k ~/dev.p12 -m ~/dev.mobileprovision \
  -b com.example.myapp -o myapp.ipa Payload/myapp.app
go-ios install --path=myapp.ipa
go-ios launch com.example.myapp --kill-existing
```

### One-shot helpers

For iterating on an examples end-to-end, two scripts in this directory chain the build, bundle, sign, and install steps above:

```sh
# macOS native:
./build-ios.sh -i -l ../../../examples/mobile_sample

# Linux cross-compile:
./build-ios-cross.sh -f -l -i -s -D 60 ../../../examples/mobile_sample
```

`-S` on either script targets the iOS Simulator. Run `-h` for the full flag list.

Signing env (skip these and the scripts produce an unsigned `.ipa` for inspection):

```sh
# Both scripts:
export IOS_PROFILE=~/dev.mobileprovision   # path to .mobileprovision
export IOS_ENTITLEMENTS=...                # optional entitlements override

# Linux only (zsign reads the cert from a .p12):
export IOS_CERT_P12=~/dev.p12
export IOS_CERT_PASS=...                   # empty if no password

# macOS only (codesign reads the cert from the keychain):
export IOS_SIGN_IDENTITY="Apple Development: Your Name (TEAMID)"   # optional; auto-picks first valid identity when unset
export IOS_DEVICE_UDID=...                                         # optional; default targets first paired device
```

`build-ios-cross.sh -S` outputs `build/<name>-simulator.zip` with DWARF preserved. Transfer to a Mac:

```sh
unzip myapp-simulator.zip
xcrun simctl install booted myapp.app
xcrun simctl launch --wait-for-debugger booted com.example.myapp
lldb -n myapp    # or attach via Xcode
```

## Library distribution as XCFramework

C / Swift / Objective-C apps can consume IUP via `IUP.xcframework`, produced on macOS only:

```sh
./build-framework.sh            # base IUP only
./build-framework.sh -F         # with optional features (GL, Web, Ctrl, Plot)
./build-framework.sh -c -F      # clean rebuild
./build-framework.sh -F -s -    # ad-hoc sign the resulting xcframework
```

Output: `iup/external/build/dist/IUP.xcframework`. The script drives three CMake presets (`cocoatouch-framework-device`, `cocoatouch-framework-simulator-arm64`, `cocoatouch-framework-simulator-x86_64`), `lipo`'s the two simulator slices into one fat sim framework, and runs `xcodebuild -create-xcframework`.

Each per-slice `IUP.framework` carries the full module map so consumers can `#import <IUP/iup.h>` (Objective-C / C) or `import IUP` (Swift).

## Caveats

* iOS 15.0 is the deployment floor (same hardware as iOS 14, lets us use `UIButton.Configuration` and `UISheetPresentationController` unconditionally).
* `@available(...)` is not allowed in the driver source (so that `osxcross` can work without specific Darwin `compiler-rt` bits). The driver uses `NSProcessInfo.operatingSystemVersion` runtime checks instead.
