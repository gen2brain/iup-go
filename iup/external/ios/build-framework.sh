#!/usr/bin/env bash
#
# build-framework.sh - assembles IUP.xcframework on macOS.
#
# Drives three CMake presets (iOS device + two simulator slices), lipo's the
# simulator slices into one fat .framework, then runs xcodebuild
# -create-xcframework to assemble the final IUP.xcframework.
#
# Output: <external>/build/dist/IUP.xcframework
#
# Usage:
#   build-framework.sh [-F] [-c] [-s IDENTITY]
#
# Options:
#   -F           include optional features (GL, Web, Ctrl, Plot)
#   -c           wipe build/cocoatouch-framework-* and dist/ before building
#   -s IDENTITY  codesign the resulting xcframework with the given identity;
#                pass "-" for ad-hoc signing
#   -h           show this help

set -euo pipefail

if [ "$(uname)" != "Darwin" ]; then
	echo "error: this script targets macOS." >&2
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXTERNAL_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${EXTERNAL_DIR}/build"
DIST_DIR="${BUILD_DIR}/dist"

FULL=0
CLEAN=0
SIGN_IDENTITY=""

usage() {
	awk 'NR==1{next} /^[^#]/{exit} {sub(/^# ?/,""); print}' "$0"
	exit "${1:-0}"
}

while getopts "Fcs:h" opt; do
	case "$opt" in
		F) FULL=1 ;;
		c) CLEAN=1 ;;
		s) SIGN_IDENTITY="$OPTARG" ;;
		h) usage 0 ;;
		*) usage 1 ;;
	esac
done

if [ "$FULL" -eq 1 ]; then
	PRESET_DEVICE="cocoatouch-framework-device-full"
	PRESET_SIM_ARM64="cocoatouch-framework-simulator-arm64-full"
	PRESET_SIM_X86_64="cocoatouch-framework-simulator-x86_64-full"
else
	PRESET_DEVICE="cocoatouch-framework-device"
	PRESET_SIM_ARM64="cocoatouch-framework-simulator-arm64"
	PRESET_SIM_X86_64="cocoatouch-framework-simulator-x86_64"
fi

if [ "$CLEAN" -eq 1 ]; then
	rm -rf \
		"${BUILD_DIR}/${PRESET_DEVICE}" \
		"${BUILD_DIR}/${PRESET_SIM_ARM64}" \
		"${BUILD_DIR}/${PRESET_SIM_X86_64}" \
		"${DIST_DIR}"
fi

build_preset() {
	local preset="$1"
	echo ">>> cmake --preset ${preset}"
	cmake -S "${EXTERNAL_DIR}" --preset "${preset}"
	echo ">>> cmake --build ${BUILD_DIR}/${preset}"
	cmake --build "${BUILD_DIR}/${preset}" --config Release
}

build_preset "${PRESET_DEVICE}"
build_preset "${PRESET_SIM_ARM64}"
build_preset "${PRESET_SIM_X86_64}"

DEVICE_FW="${BUILD_DIR}/${PRESET_DEVICE}/IUP.framework"
SIM_ARM64_FW="${BUILD_DIR}/${PRESET_SIM_ARM64}/IUP.framework"
SIM_X86_64_FW="${BUILD_DIR}/${PRESET_SIM_X86_64}/IUP.framework"

for fw in "${DEVICE_FW}" "${SIM_ARM64_FW}" "${SIM_X86_64_FW}"; do
	if [ ! -d "${fw}" ]; then
		echo "error: missing ${fw}" >&2
		exit 1
	fi
done

# Assemble the fat simulator framework: copy the arm64 bundle then lipo-merge
# the x86_64 binary into it. Frameworks staged by CMake are flat (no Versions
# symlinks on iOS) so a plain copy + binary swap is enough.
SIM_FW="${BUILD_DIR}/sim-fat/IUP.framework"
echo ">>> lipo simulator slices -> ${SIM_FW}"
rm -rf "${SIM_FW}"
mkdir -p "$(dirname "${SIM_FW}")"
cp -R "${SIM_ARM64_FW}" "${SIM_FW}"
lipo -create \
	"${SIM_ARM64_FW}/IUP" \
	"${SIM_X86_64_FW}/IUP" \
	-output "${SIM_FW}/IUP"

mkdir -p "${DIST_DIR}"
XCFRAMEWORK="${DIST_DIR}/IUP.xcframework"
rm -rf "${XCFRAMEWORK}"

echo ">>> xcodebuild -create-xcframework -> ${XCFRAMEWORK}"
xcodebuild -create-xcframework \
	-framework "${DEVICE_FW}" \
	-framework "${SIM_FW}" \
	-output "${XCFRAMEWORK}"

if [ -n "${SIGN_IDENTITY}" ]; then
	echo ">>> codesign --sign ${SIGN_IDENTITY} ${XCFRAMEWORK}"
	codesign --force --sign "${SIGN_IDENTITY}" --timestamp=none "${XCFRAMEWORK}"
fi

echo ">>> done: ${XCFRAMEWORK}"
