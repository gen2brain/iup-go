#!/usr/bin/env bash
#
# build-ios.sh - native macOS iOS builder for IUP-Go.
#
# Wraps Apple's xcrun toolchain (clang, codesign, simctl, devicectl) to compile
# a Go program for arm64 iOS device or x86_64/arm64 iOS Simulator, sign it,
# package it, and optionally install/launch on the device or Simulator.
#
# Linux cross-compile uses build-ios-cross.sh (osxcross + zsign + go-ios).
# This script intentionally has no third-party dependencies on macOS.
#
# Usage:
#   build-ios.sh [-A IOS_MIN] [-T TAGS] [-n NAME] [-b BUNDLE_ID] [-V VERSION]
#                [-B BUILD] [-D SECS] [-f] [-c] [-i] [-l] [-s] [-S] SOURCE_DIR
#
# Options:
#   -A MIN     iOS deployment target (default: 15.0)
#   -T TAGS    Go build tags
#   -n NAME    app/executable name (default: basename of SOURCE_DIR)
#   -b ID      bundle identifier (default: com.example.<name>)
#   -V VER     CFBundleShortVersionString (default: 1.0)
#   -B BUILD   CFBundleVersion (default: 1)
#   -D SECS    log capture duration in seconds (default: 3, simulator only)
#   -f         force full Go rebuild (passes -a to go build, needed after any change to ObjC)
#   -c         wipe build/ before building; if SOURCE_DIR omitted, exit
#   -i         install onto running Simulator (-S) or first paired device.
#   -l         install + launch + capture os_log to /tmp/iossyslog.txt (simulator only).
#   -s         install + screenshot to /tmp/screenshot.png (simulator only).
#   -S         build for iOS Simulator instead of device. Picks Mac's host
#              arch (arm64 on Apple Silicon, x86_64 on Intel). DWARF preserved.
#   -h         show this help
#
# Device-build signing (skipped if IOS_SIGN_IDENTITY unset):
#   IOS_SIGN_IDENTITY   codesign identity, e.g. "Apple Development: Name (TEAMID)"
#                       or omit to auto-pick first valid identity from keychain
#   IOS_PROFILE         path to .mobileprovision; copied to embedded.mobileprovision
#   IOS_ENTITLEMENTS    optional entitlements .plist
#   IOS_DEVICE_UDID     target a specific device for -i; default: first paired
#
# Examples:
#   build-ios.sh -S -i -l -n iupapp ../../../examples/mobile_hello
#   IOS_SIGN_IDENTITY="Apple Development: ..." \
#     build-ios.sh -i -n iupapp ../../../examples/mobile_hello

set -euo pipefail

if [ "$(uname)" != "Darwin" ]; then
	echo "error: this script targets macOS. On Linux use build-ios-cross.sh" >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE_DIR="${SCRIPT_DIR}/iupapp"
BUILD_DIR="${SCRIPT_DIR}/build"

MIN_IOS="15.0"
TAGS=""
NAME=""
BUNDLE_ID=""
APP_VERSION="1.0"
APP_BUILD="1"
FORCE=0
CLEAN=0
INSTALL=0
LAUNCH=0
SCREENSHOT=0
SIMULATOR=0

SCREENSHOT_FILE="/tmp/screenshot.png"
LOGCAT_FILE="/tmp/iossyslog.txt"
LOG_DURATION=3

usage() {
	awk 'NR==1{next} /^[^#]/{exit} {sub(/^# ?/,""); print}' "$0"
	exit "${1:-0}"
}

while getopts "A:T:n:b:V:B:D:fcilsSh" opt; do
	case "$opt" in
		A) MIN_IOS="$OPTARG" ;;
		T) TAGS="$OPTARG" ;;
		n) NAME="$OPTARG" ;;
		b) BUNDLE_ID="$OPTARG" ;;
		V) APP_VERSION="$OPTARG" ;;
		B) APP_BUILD="$OPTARG" ;;
		D) LOG_DURATION="$OPTARG" ;;
		f) FORCE=1 ;;
		c) CLEAN=1 ;;
		i) INSTALL=1 ;;
		l) LAUNCH=1; INSTALL=1 ;;
		s) SCREENSHOT=1; INSTALL=1 ;;
		S) SIMULATOR=1 ;;
		h) usage 0 ;;
		*) usage 1 ;;
	esac
done
shift $((OPTIND-1))

SOURCE_DIR="${1:-}"

if [ "$CLEAN" -eq 1 ]; then
	rm -rf "$BUILD_DIR"
	[ -z "$SOURCE_DIR" ] && exit 0
fi

if [ -z "$SOURCE_DIR" ]; then
	echo "error: SOURCE_DIR required" >&2
	usage 1
fi
SOURCE_DIR="$(cd "$SOURCE_DIR" && pwd)"

[ -z "$NAME" ] && NAME="$(basename "$SOURCE_DIR")"
[ -z "$BUNDLE_ID" ] && BUNDLE_ID="com.example.${NAME}"

APP_DIR="${BUILD_DIR}/Payload/${NAME}.app"
IPA="${BUILD_DIR}/${NAME}.ipa"

if [ "$SIMULATOR" -eq 1 ]; then
	SDK="iphonesimulator"
	HOST_ARCH="$(uname -m)"
	[ "$HOST_ARCH" = "arm64" ] && GOARCH=arm64 || GOARCH=amd64
	TARGET="${HOST_ARCH}-apple-ios${MIN_IOS}-simulator"
else
	SDK="iphoneos"
	GOARCH=arm64
	TARGET="arm64-apple-ios${MIN_IOS}"
fi

SDKPATH="$(xcrun -sdk "$SDK" --show-sdk-path)"
CC="$(xcrun -sdk "$SDK" -find clang)"
CXX="$(xcrun -sdk "$SDK" -find clang++)"

mkdir -p "$BUILD_DIR" "$APP_DIR"

echo ">>> go build $SOURCE_DIR -> $APP_DIR/$NAME ($TARGET)"
GO_FLAGS=(-trimpath)
[ "$SIMULATOR" -eq 0 ] && GO_FLAGS+=(-ldflags='-s -w')
[ "$FORCE" -eq 1 ] && GO_FLAGS+=(-a)
[ -n "$TAGS" ] && GO_FLAGS+=(-tags "$TAGS")

(
	cd "$SOURCE_DIR"
	CGO_ENABLED=1 \
	GOOS=ios GOARCH="$GOARCH" \
	CC="$CC" CXX="$CXX" \
	CGO_CFLAGS="-isysroot $SDKPATH -target $TARGET" \
	CGO_LDFLAGS="-isysroot $SDKPATH -target $TARGET" \
	go build "${GO_FLAGS[@]}" -o "$APP_DIR/$NAME" .
)

echo ">>> assembling $APP_DIR"
export APP_NAME="$NAME" EXEC_NAME="$NAME" BUNDLE_ID APP_VERSION APP_BUILD MIN_IOS
/usr/bin/envsubst < "$TEMPLATE_DIR/Info.plist" > "$APP_DIR/Info.plist" 2>/dev/null \
	|| { command -v envsubst >/dev/null || { echo "error: envsubst not found (brew install gettext)" >&2; exit 1; }; envsubst < "$TEMPLATE_DIR/Info.plist" > "$APP_DIR/Info.plist"; }
for icon in "$TEMPLATE_DIR"/Icon-*.png; do
	[ -f "$icon" ] && cp "$icon" "$APP_DIR/"
done

if [ "$SIMULATOR" -eq 1 ]; then
	# Simulator: ad-hoc sign so the runtime accepts the bundle. No profile needed.
	codesign --force --sign - --timestamp=none "$APP_DIR" >/dev/null 2>&1 || true
	echo ">>> simulator bundle ready: $APP_DIR"
else
	if [ -z "${IOS_SIGN_IDENTITY:-}" ]; then
		IOS_SIGN_IDENTITY="$(security find-identity -v -p codesigning 2>/dev/null \
			| awk -F'"' '/Apple Development|iPhone Developer|Apple Distribution/{print $2; exit}')"
	fi
	if [ -n "${IOS_SIGN_IDENTITY:-}" ]; then
		echo ">>> codesign with: $IOS_SIGN_IDENTITY"
		[ -n "${IOS_PROFILE:-}" ] && cp "$IOS_PROFILE" "$APP_DIR/embedded.mobileprovision"
		ENT_FLAG=()
		[ -n "${IOS_ENTITLEMENTS:-}" ] && ENT_FLAG=(--entitlements "$IOS_ENTITLEMENTS")
		codesign --force --sign "$IOS_SIGN_IDENTITY" --timestamp=none "${ENT_FLAG[@]}" "$APP_DIR"
		( cd "$BUILD_DIR" && rm -f "$IPA" && zip -qry "$IPA" Payload )
		echo ">>> signed $IPA"
	else
		echo ">>> no codesign identity (set IOS_SIGN_IDENTITY); skipping sign + install"
		( cd "$BUILD_DIR" && rm -f "$IPA" && zip -qry "$IPA" Payload )
		echo ">>> unsigned $IPA produced for inspection"
		INSTALL=0
	fi
fi

if [ "$INSTALL" -eq 1 ]; then
	if [ "$SIMULATOR" -eq 1 ]; then
		# Auto-boot the first available simulator device if none is booted.
		BOOTED="$(xcrun simctl list devices booted -j 2>/dev/null | grep -o '"udid"[^,]*' | head -1 || true)"
		if [ -z "$BOOTED" ]; then
			DEV="$(xcrun simctl list devices available -j | python3 -c 'import json,sys; d=json.load(sys.stdin); [print(x["udid"]) for k,v in d["devices"].items() for x in v if "iPhone" in x.get("name","")][:1]' | head -1)"
			[ -n "$DEV" ] && xcrun simctl boot "$DEV" >/dev/null 2>&1 || true
			open -a Simulator >/dev/null 2>&1 || true
			sleep 2
		fi
		echo ">>> simctl install booted $APP_DIR"
		xcrun simctl install booted "$APP_DIR"
		if [ "$LAUNCH" -eq 1 ]; then
			cleanup_log() {
				if [ -n "${LOG_PID:-}" ]; then
					kill "$LOG_PID" 2>/dev/null || true
					wait "$LOG_PID" 2>/dev/null || true
				fi
			}
			trap cleanup_log EXIT INT TERM
			: > "$LOGCAT_FILE"
			echo ">>> capturing $LOGCAT_FILE (${LOG_DURATION}s, filter: process == \"$NAME\")"
			xcrun simctl spawn booted log stream --predicate "process == \"$NAME\"" > "$LOGCAT_FILE" 2>&1 &
			LOG_PID=$!
			sleep 1
			echo ">>> simctl launch booted $BUNDLE_ID"
			xcrun simctl launch --terminate-running-process booted "$BUNDLE_ID" >/dev/null || true
			sleep "$LOG_DURATION"
			cleanup_log
			trap - EXIT INT TERM
		fi
		if [ "$SCREENSHOT" -eq 1 ]; then
			echo ">>> simctl io booted screenshot $SCREENSHOT_FILE"
			xcrun simctl io booted screenshot "$SCREENSHOT_FILE"
		fi
	else
		DEV_FLAG=()
		[ -n "${IOS_DEVICE_UDID:-}" ] && DEV_FLAG=(--device "$IOS_DEVICE_UDID")
		echo ">>> devicectl device install app $IPA"
		xcrun devicectl device install app "${DEV_FLAG[@]}" "$IPA"
		if [ "$LAUNCH" -eq 1 ]; then
			echo ">>> devicectl device process launch $BUNDLE_ID"
			xcrun devicectl device process launch "${DEV_FLAG[@]}" "$BUNDLE_ID" || true
			echo "    (device console: open Console.app, filter by process name '$NAME')"
		fi
		[ "$SCREENSHOT" -eq 1 ] && echo "    (device screenshot: use Xcode > Window > Devices and Simulators)"
	fi
fi

echo ">>> done"
