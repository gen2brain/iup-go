#!/usr/bin/env bash
#
# build-android.sh - build an IUP-Go Android app end-to-end.
#
# Compiles the Go main package in SOURCE_DIR as a c-shared library for each
# requested ABI, drops each .so into iupapp/src/main/jniLibs/<abi>/, runs
# Gradle to produce the APK, and optionally installs it via adb.
#
# Usage:
#   build-android.sh [-a ABI[,ABI...]] [-A API] [-n NAME] [-x EXEC]
#                    [-t TASK] [-T TAGS] [-i] SOURCE_DIR
#
# Options:
#   -a ABIS   comma-separated ABI list (default: arm64-v8a)
#             valid: arm64-v8a, armeabi-v7a, x86, x86_64
#   -A API    Android API level for the NDK toolchain (default: 22)
#   -n NAME   user-visible app label (string resource app_name, default: IupApp)
#   -x EXEC   library name without lib prefix / .so suffix (default: iupapp)
#             Must match MyIupLaunchActivity.getLibraries()/getEntryPointLibraryName()
#   -t TASK   Gradle task (default: :iupapp:assembleDebug)
#   -T TAGS   comma-separated Go build tags (passed as -tags to go build)
#   -f        force full Go rebuild (passes -a to go build, needed after any change to C)
#   -c        wipe Gradle build/ dirs and root .gradle/ cache before building; if SOURCE_DIR omitted, exit
#   -i        adb install -r the resulting APK after the build
#   -r        build release APK (assembleRelease). Signs with the env-supplied
#             keystore if ANDROID_KEYSTORE/_PASS/KEY_ALIAS are set; otherwise
#             falls back to the debug key (warns; not Play-ready).
#   -l        after install, force-stop, relaunch, wait 3s, dump filtered
#             logs (Iup + AndroidRuntime + native crash) to /tmp/logcat.txt.
#             Implies -i.
#   -s        after install (and -l's relaunch if set), grab a screencap to
#             /tmp/screenshot.png. Implies -i.
#   -L        capture-only: dump current logs + screenshot without build,
#             install, or relaunch. Use after interacting with the running
#             app to capture post-interaction state.
#   -h        show this help
#
# Environment (NDK lookup, first match wins):
#   ANDROID_NDK_HOME    path to NDK install (contains toolchains/)
#   ANDROID_NDK_ROOT    same, alternative name
#   ANDROID_SDK_ROOT    parent SDK; NDK auto-picked from $ANDROID_SDK_ROOT/ndk/<version>/
#
# Release signing env (-r only; consumed by Gradle signingConfigs.release):
#   ANDROID_KEYSTORE / ANDROID_KEYSTORE_PASS / ANDROID_KEY_ALIAS / ANDROID_KEY_PASS
#
# Examples:
#   # Default: arm64-v8a, API 22, debug APK, label "IupApp", lib libiupapp.so
#   build-android.sh ../../../examples/mobile_hello
#
#   # Multiple ABIs
#   build-android.sh -a arm64-v8a,x86_64 ../../../examples/mobile_hello
#
#   # Custom display label
#   build-android.sh -n "Mobile Hello" ../../../examples/mobile_hello
#
#   # Custom library name (must match MyIupLaunchActivity override)
#   build-android.sh -x myapp ../../../examples/mobile_hello
#
#   # Release build with install
#   build-android.sh -i -t :iupapp:assembleRelease ../../../examples/mobile_hello

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GRADLE_DIR="${SCRIPT_DIR}"
JNILIBS_DIR="${GRADLE_DIR}/iupapp/src/main/jniLibs"

ABIS="arm64-v8a"
API=22
APP_NAME="IupApp"
EXEC_NAME="iupapp"
GRADLE_TASK=":iupapp:assembleDebug"
TAGS=""
INSTALL=0
FORCE=0
CLEAN=0
LOGCAT=0
SCREENSHOT=0
LOGCAT_FILE="/tmp/logcat.txt"
SCREENSHOT_FILE="/tmp/screenshot.png"
PACKAGE="com.example.iupapp"
ACTIVITY="${PACKAGE}/.MyIupLaunchActivity"
LOGCAT_TAGS="Iup:* IupLog:* IupStdout:* IupStderr:* AndroidRuntime:E DEBUG:F libc:F"

usage() {
	# Print the leading comment block (skip the shebang), stripping the '# '.
	awk 'NR==1{next} /^[^#]/{exit} {sub(/^# ?/,""); print}' "$0"
	exit "${1:-0}"
}

CAPTURE_ONLY=0
RELEASE=0

while getopts "a:A:n:x:t:T:cfilLrsh" opt; do
	case "$opt" in
		a) ABIS="$OPTARG" ;;
		A) API="$OPTARG" ;;
		n) APP_NAME="$OPTARG" ;;
		x) EXEC_NAME="$OPTARG" ;;
		t) GRADLE_TASK="$OPTARG" ;;
		T) TAGS="$OPTARG" ;;
		c) CLEAN=1 ;;
		f) FORCE=1 ;;
		i) INSTALL=1 ;;
		l) LOGCAT=1; INSTALL=1 ;;
		L) CAPTURE_ONLY=1 ;;
		r) RELEASE=1; GRADLE_TASK=":iupapp:assembleRelease" ;;
		s) SCREENSHOT=1; INSTALL=1 ;;
		h) usage 0 ;;
		*) usage 1 ;;
	esac
done
shift $((OPTIND - 1))

if [[ "$CAPTURE_ONLY" -eq 1 ]]; then
	adb logcat -d -s $LOGCAT_TAGS > "$LOGCAT_FILE"
	echo "==> wrote $(wc -l < "$LOGCAT_FILE") lines to ${LOGCAT_FILE}"
	adb exec-out screencap -p > "$SCREENSHOT_FILE"
	if command -v mogrify >/dev/null; then
		mogrify -resize 50% "$SCREENSHOT_FILE"
	fi
	echo "==> wrote screenshot to ${SCREENSHOT_FILE}"
	exit 0
fi

if [[ "$CLEAN" -eq 1 ]]; then
	echo "==> clean: wiping ${GRADLE_DIR}/{,library/,iupapp/}build, ${GRADLE_DIR}/.gradle, and ${JNILIBS_DIR}"
	rm -rf "${GRADLE_DIR}/build" "${GRADLE_DIR}/library/build" "${GRADLE_DIR}/iupapp/build" "${GRADLE_DIR}/.gradle" "${JNILIBS_DIR}"
	# Exit here if the user only asked to clean.
	if [[ $# -lt 1 ]]; then
		echo "==> done"
		exit 0
	fi
fi

if [[ $# -lt 1 ]]; then
	echo "error: SOURCE_DIR required" >&2
	usage 1
fi
SOURCE_DIR="$(cd "$1" && pwd)"

# Locate the NDK. Standard paths:
#   $ANDROID_NDK_HOME / $ANDROID_NDK_ROOT   (must contain toolchains/)
#   $ANDROID_SDK_ROOT/ndk/<version>/        (sidecar install)
locate_ndk() {
	local candidates=()
	[[ -n "${ANDROID_NDK_HOME:-}" ]] && candidates+=("$ANDROID_NDK_HOME")
	[[ -n "${ANDROID_NDK_ROOT:-}" ]] && candidates+=("$ANDROID_NDK_ROOT")
	if [[ -n "${ANDROID_SDK_ROOT:-}" ]] && [[ -d "${ANDROID_SDK_ROOT}/ndk" ]]; then
		local v
		for v in $(ls -r "${ANDROID_SDK_ROOT}/ndk" 2>/dev/null); do
			candidates+=("${ANDROID_SDK_ROOT}/ndk/${v}")
		done
	fi
	local c
	for c in "${candidates[@]}"; do
		if [[ -d "${c}/toolchains/llvm/prebuilt/linux-x86_64/bin" ]]; then
			echo "$c"
			return 0
		fi
	done
	return 1
}

NDK_HOME="$(locate_ndk || true)"
if [[ -z "$NDK_HOME" ]]; then
	cat >&2 <<EOF
error: no Android NDK found.
  Set ANDROID_NDK_HOME (or ANDROID_NDK_ROOT) to an NDK install directory
  that contains 'toolchains/llvm/prebuilt/linux-x86_64/bin/'.
  Or install one under \$ANDROID_SDK_ROOT/ndk/<version>/.
EOF
	exit 1
fi
TOOLCHAIN="${NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64/bin"

abi_to_target() {
	case "$1" in
		arm64-v8a)   echo "arm64:aarch64-linux-android${API}" ;;
		armeabi-v7a) echo "arm:armv7a-linux-androideabi${API}" ;;
		x86)         echo "386:i686-linux-android${API}" ;;
		x86_64)      echo "amd64:x86_64-linux-android${API}" ;;
		*)           echo "error: unknown ABI '$1'" >&2; exit 1 ;;
	esac
}

IFS=',' read -ra ABI_LIST <<< "$ABIS"
for abi in "${ABI_LIST[@]}"; do
	pair="$(abi_to_target "$abi")"
	goarch="${pair%%:*}"
	triple="${pair#*:}"
	clang="${TOOLCHAIN}/${triple}-clang"
	clangxx="${TOOLCHAIN}/${triple}-clang++"
	out="${JNILIBS_DIR}/${abi}/lib${EXEC_NAME}.so"

	if [[ ! -x "$clang" ]]; then
		echo "error: no NDK clang for ABI '$abi' (looked for $clang)" >&2
		exit 1
	fi

	echo "==> go build [$abi] -> $out"
	mkdir -p "$(dirname "$out")"
	force_flag=""
	[[ "$FORCE" -eq 1 ]] && force_flag="-a"
	tags_flag=""
	[[ -n "$TAGS" ]] && tags_flag="-tags=${TAGS}"
	# Debug builds keep DWARF/symbols for lldb.
	strip_flags=""
	[[ "$RELEASE" -eq 1 ]] && strip_flags="-s -w "
	(
		cd "$SOURCE_DIR"
		# Stub pkg-config: NDK sysroot already provides target headers/libs.
		CGO_ENABLED=1 \
			CC="$clang" CXX="$clangxx" \
			GOOS=android GOARCH="$goarch" \
			PKG_CONFIG=true \
			go build $force_flag $tags_flag -buildmode=c-shared \
				-ldflags="${strip_flags}-extldflags=-Wl,-soname,lib${EXEC_NAME}.so" \
				-o "$out" .
	)
done

SIGN_PROPS=()
if [[ "$RELEASE" -eq 1 ]]; then
	if [[ -n "${ANDROID_KEYSTORE:-}" && -n "${ANDROID_KEYSTORE_PASS:-}" && -n "${ANDROID_KEY_ALIAS:-}" ]]; then
		echo "==> release signing: $ANDROID_KEYSTORE (alias $ANDROID_KEY_ALIAS)"
		SIGN_PROPS=(
			-PandroidKeystore="$(realpath "$ANDROID_KEYSTORE")"
			-PandroidKeystorePass="$ANDROID_KEYSTORE_PASS"
			-PandroidKeyAlias="$ANDROID_KEY_ALIAS"
			-PandroidKeyPass="${ANDROID_KEY_PASS:-$ANDROID_KEYSTORE_PASS}"
		)
	else
		echo "==> ANDROID_KEYSTORE/PASS/KEY_ALIAS unset; release APK will use the debug key (NOT Play-ready)"
	fi
fi

echo "==> gradle: ${GRADLE_TASK}"
(
	cd "$GRADLE_DIR"
	# GRADLE_OPTS is read by the gradlew launcher JVM; org.gradle.jvmargs
	# in gradle.properties only reaches the daemon, so the native-access
	# warning on JDK 24+ (from gradle's native-platform JAR) surfaces without
	# this flag.
	GRADLE_OPTS="--enable-native-access=ALL-UNNAMED" ./gradlew -PappName="$APP_NAME" "${SIGN_PROPS[@]}" "$GRADLE_TASK"
)

if [[ "$INSTALL" -eq 1 ]]; then
	if ! command -v adb >/dev/null; then
		echo "error: adb not in PATH" >&2
		exit 1
	fi
	# Extract the variant name from the task (e.g. ":iupapp:assembleDebug" ->
	# "debug") so we install the APK matching what we just built, not whatever
	# stale file find(1) returns first.
	variant="${GRADLE_TASK##*assemble}"
	variant="$(echo "$variant" | tr '[:upper:]' '[:lower:]')"
	APK_DIR="${GRADLE_DIR}/iupapp/build/outputs/apk/${variant}"
	APK="$(find "${APK_DIR}" -maxdepth 1 -name "*.apk" -type f -print -quit 2>/dev/null)"
	if [[ -z "$APK" ]]; then
		echo "error: no APK produced under ${APK_DIR}/" >&2
		exit 1
	fi
	echo "==> adb install -r $APK"
	adb install -r "$APK"
fi

if [[ "$LOGCAT" -eq 1 || "$SCREENSHOT" -eq 1 ]]; then
	echo "==> relaunch ${ACTIVITY}"
	adb shell am force-stop "$PACKAGE"
	adb logcat -c
	adb shell am start -n "$ACTIVITY" >/dev/null
	sleep 3

	if [[ "$LOGCAT" -eq 1 ]]; then
		adb logcat -d -s $LOGCAT_TAGS > "$LOGCAT_FILE"
		echo "==> wrote $(wc -l < "$LOGCAT_FILE") lines to ${LOGCAT_FILE}"
	fi
	if [[ "$SCREENSHOT" -eq 1 ]]; then
		adb exec-out screencap -p > "$SCREENSHOT_FILE"
		if command -v mogrify >/dev/null; then
			mogrify -resize 50% "$SCREENSHOT_FILE"
		fi
		echo "==> wrote screenshot to ${SCREENSHOT_FILE}"
	fi
fi

echo "==> done"
