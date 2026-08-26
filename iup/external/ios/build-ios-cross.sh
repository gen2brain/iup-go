#!/usr/bin/env bash
#
# build-ios-cross.sh - build, sign, install, and trace an IUP-Go iOS app via go-ios.
#
# Usage:
#   build-ios-cross.sh [-A IOS_MIN] [-T TAGS] [-n NAME] [-x EXEC] [-b BUNDLE_ID]
#                [-V VERSION] [-B BUILD] [-D SECS]
#                [-f] [-c] [-i] [-l] [-s] [-L] [-S] SOURCE_DIR
#
# Options:
#   -A MIN     iOS deployment target (default: 15.0)
#   -T TAGS    Go build tags
#   -n NAME    user-visible app name (CFBundleDisplayName, default: IupApp)
#   -x EXEC    binary filename / bundle dir basename (default: iupapp)
#   -b ID      bundle identifier (default: com.example.<exec>)
#   -V VER     CFBundleShortVersionString (default: 1.0)
#   -B BUILD   CFBundleVersion (default: 1)
#   -D SECS    log capture duration in seconds (default: 3)
#   -f         force full Go rebuild (passes -a to go build, needed after any change to ObjC)
#   -c         wipe build/ before building; if SOURCE_DIR omitted, exit
#   -i         install signed .ipa
#   -l         install + relaunch + capture os_log lines starting with "[Iup "
#              to /tmp/iossyslog.txt. Implies -i.
#   -s         install + screenshot to /tmp/screenshot.png. Implies -i.
#   -L         capture-only: log + screenshot of the running app, no rebuild
#   -S         build for x86_64 iOS Simulator instead of arm64 device.
#              Produces build/<exec>-simulator.zip containing <exec>.app, with
#              DWARF symbols preserved. No signing, no install/log/screenshot.
#              Transfer to a Mac and run `xcrun simctl install booted <exec>.app`.
#   -h         show this help
#
# Signing env (device builds only; skipped if IOS_CERT_P12 unset):
#   IOS_CERT_P12 / IOS_CERT_PASS / IOS_PROFILE / IOS_ENTITLEMENTS
#
# One-time tunnel for iOS 17+ (needed by launch/screenshot/ostrace):
#   sudo go-ios tunnel start &
#
# Examples:
#   build-ios-cross.sh -i -l -s ../../../examples/mobile_hello
#   build-ios-cross.sh -n "Mobile Sample" -x mobilesample ../../../examples/mobile_sample
#   build-ios-cross.sh -L

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE_DIR="${SCRIPT_DIR}/iupapp"
BUILD_DIR="${SCRIPT_DIR}/build"

MIN_IOS="15.0"
TAGS=""
APP_NAME="IupApp"
EXEC_NAME="iupapp"
BUNDLE_ID=""
APP_VERSION="1.0"
APP_BUILD="1"
FORCE=0
CLEAN=0
INSTALL=0
LOGCAP=0
SCREENSHOT=0
CAPTURE_ONLY=0
SIMULATOR=0
LOG_DURATION=3

LOGCAT_FILE="/tmp/iossyslog.txt"
SCREENSHOT_FILE="/tmp/screenshot.png"
OSTRACE_PIDFILE="/tmp/iup-ios-ostrace.pid"

# an example ipa is a few MB; anything near this is a packaging runaway
IPA_MAX_BYTES=$((200 * 1024 * 1024))

usage() {
	awk 'NR==1{next} /^[^#]/{exit} {sub(/^# ?/,""); print}' "$0"
	exit "${1:-0}"
}

while getopts "A:T:n:x:b:V:B:D:fcilLsSh" opt; do
	case "$opt" in
		A) MIN_IOS="$OPTARG" ;;
		T) TAGS="$OPTARG" ;;
		n) APP_NAME="$OPTARG" ;;
		x) EXEC_NAME="$OPTARG" ;;
		b) BUNDLE_ID="$OPTARG" ;;
		V) APP_VERSION="$OPTARG" ;;
		B) APP_BUILD="$OPTARG" ;;
		D) LOG_DURATION="$OPTARG" ;;
		f) FORCE=1 ;;
		c) CLEAN=1 ;;
		i) INSTALL=1 ;;
		l) LOGCAP=1; INSTALL=1 ;;
		L) CAPTURE_ONLY=1 ;;
		s) SCREENSHOT=1; INSTALL=1 ;;
		S) SIMULATOR=1 ;;
		h) usage 0 ;;
		*) usage 1 ;;
	esac
done
shift $((OPTIND-1))

SOURCE_DIR="${1:-}"

if [ "$CLEAN" -eq 1 ]; then
	echo "==> clean: wiping $BUILD_DIR"
	rm -rf "$BUILD_DIR"
	if [ -z "$SOURCE_DIR" ]; then
		echo "==> done"
		exit 0
	fi
fi

if [ "$CAPTURE_ONLY" -eq 1 ]; then
	LOGCAP=1
	SCREENSHOT=1
	INSTALL=0
fi

if [ "$SIMULATOR" -eq 1 ]; then
	INSTALL=0
	LOGCAP=0
	SCREENSHOT=0
	CAPTURE_ONLY=0
fi

if [ "$CAPTURE_ONLY" -eq 0 ] && [ -z "$SOURCE_DIR" ]; then
	echo "error: SOURCE_DIR required" >&2
	usage 1
fi
[ -n "$SOURCE_DIR" ] && SOURCE_DIR="$(cd "$SOURCE_DIR" && pwd)"

[ -z "$BUNDLE_ID" ] && BUNDLE_ID="com.example.${EXEC_NAME}"

PKG_DIR="${BUILD_DIR}/pkg"
APP_DIR="${PKG_DIR}/Payload/${EXEC_NAME}.app"
IPA="${BUILD_DIR}/${EXEC_NAME}.ipa"
SIM_ZIP="${BUILD_DIR}/${EXEC_NAME}-simulator.zip"

if [ "$SIMULATOR" -eq 1 ]; then
	IOS_CC=x86_64-apple-ios-simulator-clang
	IOS_CXX=x86_64-apple-ios-simulator-clang++
	GO_GOARCH=amd64
else
	IOS_CC=arm64-apple-ios-clang
	IOS_CXX=arm64-apple-ios-clang++
	GO_GOARCH=arm64
fi

if [ "$CAPTURE_ONLY" -eq 0 ]; then
	command -v "$IOS_CC" >/dev/null \
		|| { echo "error: $IOS_CC not on PATH" >&2; exit 1; }

	rm -rf "$PKG_DIR"
	mkdir -p "$BUILD_DIR" "$APP_DIR"

	echo ">>> go build $SOURCE_DIR -> $APP_DIR/$EXEC_NAME"
	# Simulator builds keep DWARF for lldb/Xcode debugging; device builds strip.
	if [ "$SIMULATOR" -eq 1 ]; then
		GO_FLAGS=(-trimpath)
	else
		GO_FLAGS=(-trimpath -ldflags='-s -w')
	fi
	[ "$FORCE" -eq 1 ] && GO_FLAGS+=(-a)
	[ -n "$TAGS" ] && GO_FLAGS+=(-tags "$TAGS")

	(
		cd "$SOURCE_DIR"
		CGO_ENABLED=1 \
		GOOS=ios GOARCH="$GO_GOARCH" \
		CC="$IOS_CC" CXX="$IOS_CXX" \
		IOS_DEPLOYMENT_TARGET="$MIN_IOS" \
		go build "${GO_FLAGS[@]}" -o "$APP_DIR/$EXEC_NAME" .
	)

	echo ">>> Mach-O verification"
	aarch64-apple-darwin25.1-otool -l "$APP_DIR/$EXEC_NAME" \
		| awk '/LC_BUILD_VERSION/{f=1} f && /platform|minos|sdk/{print; n++} n==3{exit}'

	echo ">>> assembling $APP_DIR"
	export APP_NAME EXEC_NAME BUNDLE_ID APP_VERSION APP_BUILD MIN_IOS
	envsubst < "$TEMPLATE_DIR/Info.plist" > "$APP_DIR/Info.plist"
	for icon in "$TEMPLATE_DIR"/Icon-*.png; do
		[ -f "$icon" ] && cp "$icon" "$APP_DIR/"
	done

	if [ "$SIMULATOR" -eq 1 ]; then
		( cd "$PKG_DIR/Payload" && rm -f "$SIM_ZIP" && zip -qry "$SIM_ZIP" "${EXEC_NAME}.app" )
		echo ">>> simulator bundle: $SIM_ZIP"
		echo "    on Mac: unzip + xcrun simctl install booted ${EXEC_NAME}.app"
	else
		if [ -n "${IOS_CERT_P12:-}" ] && [ -n "${IOS_PROFILE:-}" ]; then
			echo ">>> signing with zsign"
			# Resolve user-supplied paths before the cd so they stay valid.
			ZS_P12="$(realpath "$IOS_CERT_P12")"
			ZS_PROFILE="$(realpath "$IOS_PROFILE")"
			ENT_FLAG=()
			[ -n "${IOS_ENTITLEMENTS:-}" ] && ENT_FLAG=(-e "$(realpath "$IOS_ENTITLEMENTS")")
			(
				cd "$PKG_DIR"
				zsign -f \
					-k "$ZS_P12" \
					${IOS_CERT_PASS:+-p "$IOS_CERT_PASS"} \
					-m "$ZS_PROFILE" \
					-b "$BUNDLE_ID" \
					"${ENT_FLAG[@]}" \
					"Payload/${EXEC_NAME}.app"
			)
		else
			echo ">>> IOS_CERT_P12/IOS_PROFILE unset; skipping sign + install"
			INSTALL=0
		fi

		# zsign -o archives its whole working directory, not just the bundle
		( cd "$PKG_DIR" && rm -f "$IPA" && zip -qry "$IPA" Payload )

		IPA_BYTES=$(stat -c %s "$IPA")
		if [ "$IPA_BYTES" -gt "$IPA_MAX_BYTES" ]; then
			echo "error: $IPA is $((IPA_BYTES / 1048576)) MB, refusing it" >&2
			exit 1
		fi
		echo ">>> $IPA ($((IPA_BYTES / 1024)) KB)"
	fi
fi

if [ "$INSTALL" -eq 1 ]; then
	[ -f "$IPA" ] || { echo "error: $IPA not built" >&2; exit 1; }
	echo ">>> go-ios install $IPA"
	if ! INSTALL_ERR="$(go-ios install --path="$IPA" 2>&1 >/dev/null)"; then
		echo "$INSTALL_ERR" >&2
		exit 1
	fi
fi

if [ "$LOGCAP" -eq 1 ]; then
	# Stream opens BEFORE launch so early IupLog traces are caught.
	# --match="[Iup " is the only filter; every IupLog call and cgo stdio
	# emits "[Iup <type>] ..." through os_log.
	# a stream left behind by an interrupted run keeps holding the device
	if [ -f "$OSTRACE_PIDFILE" ]; then
		STALE_PID="$(cat "$OSTRACE_PIDFILE")"
		if [ -n "$STALE_PID" ] && grep -qs go-ios "/proc/$STALE_PID/cmdline"; then
			kill "$STALE_PID" 2>/dev/null || true
		fi
		rm -f "$OSTRACE_PIDFILE"
	fi

	cleanup_log() {
		if [ -n "${LOG_PID:-}" ]; then
			kill "$LOG_PID" 2>/dev/null || true
			wait "$LOG_PID" 2>/dev/null || true
		fi
		rm -f "$OSTRACE_PIDFILE"
	}
	trap cleanup_log EXIT INT TERM

	if [ "$CAPTURE_ONLY" -eq 0 ]; then
		: > "$LOGCAT_FILE"
		echo ">>> capturing $LOGCAT_FILE (${LOG_DURATION}s, fresh, pre-launch)"
		go-ios ostrace --nojson --match="[Iup " 2>/dev/null > "$LOGCAT_FILE" &
		LOG_PID=$!
		echo "$LOG_PID" > "$OSTRACE_PIDFILE"
		sleep 1
		echo ">>> go-ios launch $BUNDLE_ID --kill-existing"
		go-ios launch "$BUNDLE_ID" --kill-existing >/dev/null 2>&1 || true
	else
		echo ">>> capturing $LOGCAT_FILE (${LOG_DURATION}s, append)"
		go-ios ostrace --nojson --match="[Iup " 2>/dev/null >> "$LOGCAT_FILE" &
		LOG_PID=$!
		echo "$LOG_PID" > "$OSTRACE_PIDFILE"
	fi
	sleep "$LOG_DURATION"
	cleanup_log
	trap - EXIT INT TERM
	echo ">>> $LOGCAT_FILE total $(wc -l <"$LOGCAT_FILE")"

	CRASH_DIR="${LOGCAT_FILE%.*}.crashes"
	rm -rf "$CRASH_DIR"; mkdir -p "$CRASH_DIR"
	TODAY=$(date +%Y-%m-%d)
	for pattern in "${EXEC_NAME}-${TODAY}-*.ips" "ExcUserFault_${EXEC_NAME}-${TODAY}-*.ips"; do
		go-ios crash cp "$pattern" "$CRASH_DIR" >/dev/null 2>&1 || true
	done
	pulled=$(find "$CRASH_DIR" -maxdepth 1 -name "*.ips" 2>/dev/null | wc -l)
	[ "$pulled" -gt 0 ] && echo ">>> pulled $pulled crash report(s) -> $CRASH_DIR"
fi

if [ "$SCREENSHOT" -eq 1 ]; then
	echo ">>> go-ios screenshot --output=$SCREENSHOT_FILE"
	go-ios screenshot --output="$SCREENSHOT_FILE" >/dev/null 2>&1
	command -v mogrify >/dev/null && mogrify -resize 50% "$SCREENSHOT_FILE"
fi

echo ">>> done"
