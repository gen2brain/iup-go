## iupkg

Builds and packages an IUP-Go program for distribution: a Windows executable with icon, version info and manifest,
a macOS `.app`, a Linux tarball, `.deb` or `.rpm`, an Android `.apk`, an iOS `.ipa`, a WebAssembly site or a Haiku `.hpkg`.

Every format is written by iupkg itself, including the signatures, so a Go toolchain is all a host needs.
cross-compiling C code and installing on devices need the platform tools named below.

```sh
go install github.com/gen2brain/iup-go/cmd/iupkg@latest
```

### Quick start

Run it from the module of the program you package:

```sh
cd examples
iupkg package --os linux ./sample                      # sample-1.0.0-linux-amd64.tar.gz
iupkg package --os windows --arch amd64 ./sample       # sample.exe
iupkg package --os darwin --arch universal ./sample    # sample.app + zip
iupkg package --os android --install ./mobile_sample   # mobile_sample.apk, installed with adb
iupkg package --os js ./sample && iupkg serve sample   # http://localhost:8000/
```

### Usage

```
iupkg package [flags] [package]
iupkg sign [flags] <file>
iupkg serve [--addr localhost:8000] <directory>
iupkg staple <app>
```

`package` is the main package to build, `.` by default. Output goes to `--out` (default: the current directory). `iupkg help` prints the flag list.

 | Flag                                                 | Default             | Meaning                                                                                                                     |
 |------------------------------------------------------|---------------------|-----------------------------------------------------------------------------------------------------------------------------|
 | `--os`                                               | host                | `windows`, `darwin`, `linux`, `android`, `ios`, `js` (or `wasm`), `haiku`                                                   |
 | `--arch`                                             | host                | target architecture; `universal` on darwin, a comma list on android, `arm64` on android and ios by default                  |
 | `--out`                                              | `.`                 | output directory                                                                                                            |
 | `--name`                                             | executable name     | application name shown to the user                                                                                          |
 | `--id`                                               | `com.example.<exe>` | application identifier, the `APPID` the program sets; linux: the desktop file name, haiku: the application signature        |
 | `--version`                                          | `1.0.0`             | version string                                                                                                              |
 | `--build`                                            | `1`                 | build number: `versionCode`, `CFBundleVersion`, last Windows version field, deb/rpm/hpkg revision                           |
 | `--icon`                                             | the IUP icon        | square PNG, 1024 px recommended                                                                                             |
 | `--tags`                                             |                     | Go build tags (`gl,ctrl,web,...`)                                                                                           |
 | `--ldflags`                                          |                     | extra linker flags                                                                                                          |
 | `--cgo`                                              |                     | build with cgo (default: `go env CGO_ENABLED` for the host platform, off when cross-compiling, always on with a driver tag) |
 | `--release`                                          |                     | `-trimpath -ldflags "-s -w"`                                                                                                |
 | `--console`                                          |                     | windows: console application                                                                                                |
 | `--format`                                           | `targz`             | linux: comma list of `targz`, `deb`, `rpm`                                                                                  |
 | `--category`                                         | `Utility`           | linux: desktop entry categories                                                                                             |
 | `--vendor`                                           | application name    | Windows company name, Debian maintainer (`Name <email>`), RPM and Haiku vendor                                              |
 | `--copyright`                                        |                     | Windows and Haiku copyright line                                                                                            |
 | `--license`                                          | `Unknown`           | rpm and haiku license name                                                                                                  |
 | `--permissions`                                      |                     | comma list of `camera`, `microphone`, `location`, `notifications`, `internet`                                               |
 | `--sign`                                             |                     | signing identity, see Signing                                                                                               |
 | `--signer`                                           | `iupkg`             | `iupkg`; darwin, ios: `codesign` (the default on a Mac); linux: `gpg`                                                       |
 | `--timestamp`                                        | `true`              | add a trusted timestamp when signing with a certificate                                                                     |
 | `--timestamp-url`                                    | DigiCert            | windows: RFC 3161 server                                                                                                    |
 | `--install`                                          |                     | android, ios: install on the connected device                                                                               |
 | `--simulator`                                        |                     | ios: build for the simulator                                                                                                |
 | `--minos`                                            | `15.0`              | ios: minimum iOS version                                                                                                    |
 | `--profile`                                          |                     | ios: provisioning profile                                                                                                   |
 | `--notary-key`, `--notary-key-id`, `--notary-issuer` |                     | darwin: App Store Connect API key, enables notarization                                                                     |

Without `--cgo` the program is built with `CGO_ENABLED=0` and the purego backend, so the desktop targets build from any host with only Go installed.
With `--cgo` the IUP C library is compiled into the executable; cross builds then need the matching C toolchain in `CC` and `CXX`.
Android, iOS and Haiku are always cgo builds.

### Targets

#### Windows

`<exe>.exe` with the icon, version resource (product name, version, company from `--vendor`, copyright) and the IUP application manifest embedded.
Built as a GUI application unless `--console`. A cross build with `--cgo` or a driver tag needs llvm-mingw or MinGW-w64 in `CC`.

```sh
iupkg package --os windows --arch amd64 --name "My App" --version 1.2.0 --icon icon.png ./cmd/myapp
iupkg package --os windows --tags winui --release ./cmd/myapp                  # on a Windows host
```

#### macOS

`<Name>.app` and a zip of it. `--arch universal` builds amd64 and arm64 and merges them. A purego build carries the IUP libraries in `Contents/Frameworks`; a cgo build (the default on a Mac) has them linked in.
`LSMinimumSystemVersion` comes from the executable. Camera, microphone and location usage descriptions and entitlements are added for the listed `--permissions`. Without `--sign` the bundle is signed ad-hoc.

```sh
iupkg package --os darwin --arch universal --name "My App" --id com.example.myapp ./cmd/myapp
```

#### Linux

`<exe>-<version>-linux-<arch>.tar.gz` with `bin/<exe>`, a `.desktop` file, hicolor icons from 16 to 512 px and a `Makefile` (`install`, `uninstall`, `user-install`, `user-uninstall`, `PREFIX`, `DESTDIR`).
The desktop file and icons are named after the executable, which is the app id GTK, Qt and FLTK report when the program does not set `APPID`; with `--id` they take that name.

```sh
iupkg package --os linux --arch arm64 --release ./cmd/myapp
tar xzf myapp-1.0.0-linux-arm64.tar.gz && make -C myapp-1.0.0 user-install
```

`--format deb,rpm` writes `<name>_<version>-<build>_<arch>.deb` and
`<name>-<version>-<build>.<arch>.rpm` instead, installing under `/usr`. With `--sign` all three are signed with an OpenPGP key, see Signing.

```sh
iupkg package --os linux --format deb,rpm --vendor "Jane Doe <jane@example.com>" --license MIT ./cmd/myapp
```

#### Android

`<exe>.apk`, built from the template APK shipped in the iup module. Only the Android NDK is needed: set `ANDROID_NDK_HOME`, or install it under `$ANDROID_SDK_ROOT/ndk`.
The Go program is compiled as a shared library per ABI; the manifest, launcher icons and native libraries are written into the template, then the APK is aligned and signed (v1 and v2).
No permission is requested unless listed. `--install` runs `adb install`.

```sh
iupkg package --os android --arch arm64,amd64 --name "My App" --id com.example.myapp --permissions camera,internet --install ./cmd/myapp
```

Programs that need their own Java code or manifest entries use the Gradle project in
`iup/external/android` instead.

#### iOS

`<exe>.ipa` (or `<exe>-simulator.zip` with `--simulator`) with the icons, `Info.plist` and the provisioning profile embedded.
On a Mac the build uses the Xcode toolchain; elsewhere it needs osxcross with the iOS SDK (`arm64-apple-ios-clang` on `PATH`, or `CC`). `--sign` and `--profile` are both required for a device build;
the entitlements come from the profile. `--install` uses `devicectl` on a Mac and [go-ios](https://github.com/danielpaulus/go-ios) elsewhere.

```sh
iupkg package --os ios --name "My App" --id com.example.myapp --sign dev.p12 --profile dev.mobileprovision --install ./cmd/myapp
```

#### WebAssembly

A directory `<exe>/` ready to serve: `index.html`, the worker and DOM bridge scripts, the IUP module (`iup.js`, `iup.wasm`, prebuilt with every subsystem and shipped in the iup module), the program as `app.wasm` and Go's `wasm_exec.js`.
The page needs the `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` headers for `SharedArrayBuffer`; `iupkg serve` is a static server that sets them.

```sh
iupkg package --os js --name "My App" --release ./cmd/myapp
iupkg serve myapp
```

#### Haiku

`<name>-<version>-<build>-x86_64.hpkg` holding `apps/<Name>/<exe>` with the application signature, type and bitmap icons as file attributes, a Deskbar menu entry and the `.PackageInfo`.
Copy it into `/boot/home/config/packages` and it is active; remove it to uninstall. `--id` sets the signature (default `application/x-vnd.iup-Application`, the driver's default `APPID`);
`--version` must be `major[.minor[.micro]]` and `--build` is the package revision.

The Go used is `GOHAIKU` if set, else `go`. Cross-building needs the Haiku Go port and the `haiku-x86_64-cc` wrapper on `PATH` (or `CC`/`CXX`).

```sh
GOHAIKU=/opt/haiku-go/bin/go iupkg package --os haiku --name "My App" --version 1.0 ./cmd/myapp
```

### Signing

`--sign` takes a `.p12`/`.pfx` file (password in `IUPKG_P12_PASSWORD`, empty by default) or a PEM file holding the private key, the certificate and any intermediate certificates.
With `--signer codesign` it is a keychain identity instead. Signatures carry a trusted timestamp unless `--timestamp=false`.

#### Windows

Authenticode: SHA-256, the full certificate chain from the file, an RFC 3161 countersignature from `--timestamp-url`.
Windows only trusts certificates from a CA in Microsoft's root program; a self-signed one produces a valid signature that SmartScreen still warns about.

```sh
IUPKG_P12_PASSWORD=secret iupkg package --os windows --arch amd64 --sign codesign.p12 ./cmd/myapp
```

#### macOS

The certificate must be Apple-issued (Developer ID Application for distribution outside the App Store; Apple Development for testing).
The bundle gets the hardened runtime, entitlements from `--permissions`, sealed resources and nested dylibs, and a timestamp from Apple's service.

```sh
iupkg package --os darwin --arch universal --sign devid.p12 ./cmd/myapp
```

Notarization needs a Developer ID certificate and an App Store Connect API key (`AuthKey_<id>.p8` with its issuer id).
iupkg uploads the zipped bundle, waits for Apple's verdict (minutes to half an hour; a rejection prints Apple's log) and staples the ticket.
`iupkg staple <app>` fetches and staples the ticket for a bundle notarized earlier.

```sh
iupkg package --os darwin --arch universal --sign devid.p12 --notary-key AuthKey_ABC123DEFG.p8 --notary-issuer 69a6de7f-... ./cmd/myapp
```

On a Mac `--signer codesign` is the default and uses `codesign`, `notarytool` and `stapler` with a keychain identity in `--sign`;
`--signer iupkg` selects the built-in signer there too. Apple's `security import` rejects `.p12` files written with current OpenSSL defaults; re-export them with `openssl pkcs12 -export -legacy` for the keychain.

#### iOS

The certificate must match the provisioning profile. On a Mac `codesign` is the default signer.

```sh
iupkg package --os ios --sign dev.p12 --profile dev.mobileprovision --install ./cmd/myapp
```

#### Linux

OpenPGP, SHA-256. `--sign` takes a secret key exported with `gpg --export-secret-keys`, armored or binary, holding exactly one key (passphrase in `IUPKG_GPG_PASSPHRASE`, empty by default).
With `--signer gpg` the `gpg` program signs instead and `--sign` is any key it knows (an id, a fingerprint or an email), which covers keys held by the agent or a smartcard.

- `.tar.gz`: a detached armored signature `<archive>.asc`, checked with `gpg --verify`.
- `.rpm`: the header signature and the legacy header and payload signature, checked with `rpmkeys --checksig` after `rpmkeys --import` of the public key.
- `.deb`: a debsigs origin signature (the `_gpgorigin` member), checked with `debsig-verify`. dpkg and apt do not check it; they trust the signed repository a package comes from, which iupkg does not make.

```sh
gpg --armor --export-secret-keys jane@example.com > release.asc
IUPKG_GPG_PASSPHRASE=secret iupkg package --os linux --format targz,deb,rpm --sign release.asc ./cmd/myapp
iupkg package --os linux --format rpm --signer gpg --sign jane@example.com ./cmd/myapp
```

### Signing existing files

`iupkg sign` signs a file made elsewhere, in place, with the same signers: a Windows `.exe` or `.dll`, a macOS or iOS `.app` directory, an `.ipa`, an `.apk`, a `.deb`, an `.rpm`, or a bare Mach-O executable or dylib.
It takes `--sign`, `--signer` (`gpg`), `--profile` (iOS), `--entitlements` (macOS), `--timestamp`, `--timestamp-url` and the `--notary-*` flags; a macOS bundle with `--notary-key` is notarized and stapled after signing.

```sh
iupkg sign --sign codesign.p12 MyApp.exe
iupkg sign --sign devid.p12 --notary-key AuthKey_ABC123DEFG.p8 --notary-issuer 69a6de7f-... "My App.app"
iupkg sign --sign dev.p12 --profile dev.mobileprovision MyApp.ipa
iupkg sign MyApp.apk                                  # debug key
iupkg sign --sign release.asc myapp-1.0.0-1.x86_64.rpm
iupkg sign --signer gpg --sign jane@example.com myapp-1.0.0-haiku.hpkg   # writes the .hpkg.asc
```

With an OpenPGP key (`--signer gpg`, or a key file named `.asc`, `.gpg` or `.pgp`) a `.deb` or `.rpm` is re-signed in place, replacing any signature it had, and every other file gets a detached armored `<file>.asc` next to it. RPM v6 packages are refused.

#### Android

Without `--sign` a debug key is generated once and kept in the user cache directory. For release, convert a Java keystore once:

```sh
keytool -importkeystore -srckeystore release.jks -destkeystore release.p12 -deststoretype PKCS12
IUPKG_P12_PASSWORD=secret iupkg package --os android --sign release.p12 --release ./cmd/myapp
```
