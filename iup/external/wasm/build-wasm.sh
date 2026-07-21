#!/bin/sh
# Builds an IUP app for WebAssembly and optionally loads it in headless Chrome
# to capture a screenshot and the console log.
#
# Two modes, chosen by the app argument:
#   *.c | *.cpp  C/C++ app: IUP and the app are emcc-built together into
#                build/iup.js, which auto-runs main().
#   dir | *.go   Go app: the IUP core+driver is emcc-built as a MODULARIZE
#                library (build/iup.js), the Go program is compiled to
#                build/app.wasm (GOOS=js, no cgo), and a loader page wires them.
#
# Usage: build-wasm.sh [-s] [-l] [-k SELECTOR] [-c] [-f] [-O] [-t] [-T TAGS] [-h] APP
#   APP          a Go example directory, a *.go file, or a *.c/*.cpp app.
#   -s           screenshot to /tmp/screenshot.png
#   -l           console log to /tmp/wasm_console.txt
#   -k SELECTOR  click SELECTOR after load, then screenshot _after.png
#   -y TEXT      type TEXT into SELECTOR (or first input), then screenshot _after.png
#                (IUP_WAIT=ms overrides the 20s wait for heavy/slow-loading apps)
#   -c           wipe the build dir before building; if APP omitted, exit
#   -f           force-rebuild the emcc C module + full Go rebuild (go build -a)
#                (the C module is otherwise reused; also use after -T/-O changes)
#   -O           optimized/release build (emcc -O2, Go -ldflags='-s -w', wasm-opt)
#   -t           compile the Go program with TinyGo (smaller app.wasm)
#   -T TAGS      build tags (comma/space separated), e.g. "gl"; selects the
#                optional subsystems (gl = IupGLCanvas) in both the emcc module
#                and the Go program, mirroring the desktop -tags convention.
#   -h           show this help
#
# Requires EMSDK at /opt/emsdk (or EMSDK exported). -s/-l/-k need node +
# playwright-core + a system Chrome or Chromium (IUP_BROWSER overrides).

set -e

HERE=$(cd "$(dirname "$0")" && pwd)
EXTERNAL=$(cd "$HERE/.." && pwd)
REPO=$(cd "$EXTERNAL/../.." && pwd)
SRC="$EXTERNAL/src"
INCLUDE="$EXTERNAL/include"
BUILD="$HERE/build"

usage() {
  # Print the leading comment block (skip the shebang), stripping the '# '.
  awk 'NR==1{next} /^[^#]/{exit} {sub(/^# ?/,""); print}' "$0"
  exit "${1:-0}"
}

case "${1:-}" in -h|--help) usage 0 ;; esac

SHOT=0
LOG=0
CLICK=""
TYPE=""
KEYS=""
CLEAN=0
TAGS=""
OPT=0
TINYGO=0
FORCE=0
while getopts "slk:y:K:cT:Otfh" opt; do
  case "$opt" in
    s) SHOT=1 ;;
    l) LOG=1 ;;
    k) CLICK="$OPTARG" ;;
    y) TYPE="$OPTARG" ;;
    K) KEYS="$OPTARG" ;;
    c) CLEAN=1 ;;
    T) TAGS="$OPTARG" ;;
    O) OPT=1 ;;
    t) TINYGO=1 ; PATH="/opt/tinygo/bin:$PATH" ;;
    f) FORCE=1 ;;
    h) usage 0 ;;
    *) usage 1 ;;
  esac
done
shift $((OPTIND - 1))

TAGS_NORM=" $(echo "$TAGS" | tr ',' ' ') "
has_tag() { case "$TAGS_NORM" in *" $1 "*) return 0 ;; *) return 1 ;; esac; }

if [ "$CLEAN" = 1 ]; then
  echo ">>> clean: $BUILD"
  rm -rf "$BUILD"
  [ $# -eq 0 ] && exit 0
fi

if [ $# -lt 1 ]; then
  echo "error: APP required" >&2
  usage 1
fi
APP="$1"

EMSDK_DIR="${EMSDK:-/opt/emsdk}"
. "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1 || true

mkdir -p "$BUILD"
rm -f "$BUILD"/app.wasm

PREJS="$HERE/web/iupwasm_dom.js"
if [ "$OPT" = 1 ]; then COPT="-O2"; GOLD="-s -w"; else COPT="-O0 -g"; GOLD=""; fi
CFLAGS="$COPT -I$INCLUDE -I$SRC -I$SRC/wasm -DIUP_WASM"
# wasm ships its own native iupwasm_datepick.c, so exclude the core composite
CORE_IUP=$(ls "$SRC"/iup_*.c | grep -v '/iup_datepick\.c$')
CORE="$SRC/iup.c $CORE_IUP $SRC/wasm/iupwasm_*.c"

GLFLAGS=""
GL_EXPORTS=""
if has_tag gl; then
  CORE="$CORE $EXTERNAL/srcgl/iup_glcanvas.c $EXTERNAL/srcgl/iup_glcanvas_wasm.c"
  GLFLAGS="-sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=1"
  GL_EXPORTS=",_IupGLCanvasOpen,_IupGLCanvas,_IupGLBackgroundBox,_IupGLMakeCurrent,_IupGLIsCurrent,_IupGLSwapBuffers,_IupGLPalette,_IupGLUseFont,_IupGLWait,_iupwasmGLDomId"
fi

WEB_EXPORTS=""
if has_tag web; then
  CORE="$CORE $EXTERNAL/srcweb/iup_webbrowser.c $EXTERNAL/srcweb/iupwasm_webbrowser.c"
  WEB_EXPORTS=",_IupWebBrowserOpen,_IupWebBrowser"
fi

CTRL_EXPORTS=""
if has_tag ctrl; then
  CFLAGS="$CFLAGS -I$EXTERNAL/srcctrl"
  CORE="$CORE $EXTERNAL/srcctrl/*.c $EXTERNAL/srcctrl/matrix/*.c $EXTERNAL/srcctrl/matrixex/*.c"
  CTRL_EXPORTS=",_IupControlsOpen,_IupCells,_IupMatrix,_IupMatrixList,_IupMatrixEx,_IupFlatButton,_IupFlatLabel,_IupFlatToggle,_IupFlatFrame,_IupFlatList,_IupFlatTree,_IupFlatVal,_IupFlatTabs,_IupFlatScrollBox,_IupDropButton,_IupGauge"
fi

PLOT_EXPORTS=""
if has_tag plot; then
  CFLAGS="$CFLAGS -I$EXTERNAL/srcplot"
  CORE="$CORE $EXTERNAL/srcplot/*.cpp"
  PLOT_EXPORTS=",_IupPlotOpen,_IupPlot,_IupPlotBegin,_IupPlotAdd,_IupPlotAddStr,_IupPlotAddSamples,_IupPlotAddStrSamples,_IupPlotInsert,_IupPlotInsertStr,_IupPlotInsertSamples,_IupPlotInsertStrSamples,_IupPlotAddSegment,_IupPlotInsertSegment,_IupPlotEnd,_IupPlotLoadData,_IupPlotSetSample,_IupPlotSetSampleStr,_IupPlotSetSampleSelection,_IupPlotSetSampleExtra,_IupPlotGetSample,_IupPlotGetSampleStr,_IupPlotGetSampleSelection,_IupPlotGetSampleExtra,_IupPlotTransform,_IupPlotTransformTo,_IupPlotFindSample,_IupPlotFindSegment"
fi

if [ "$FORCE" = 1 ] || [ ! -f "$BUILD/iup.js" ]; then MODULE_FRESH=1; else MODULE_FRESH=0; fi

case "$APP" in
  *.c | *.cpp)
    if [ "$MODULE_FRESH" = 1 ]; then
      echo ">>> emcc (C/C++ app, runs on a Web Worker): $APP"
      emcc $CFLAGS $GLFLAGS \
        -pthread -sPTHREAD_POOL_SIZE=4 \
        -sMODULARIZE=1 -sEXPORT_NAME=createIupModule -sINVOKE_RUN=0 \
        -sERROR_ON_UNDEFINED_SYMBOLS=1 -sNO_EXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sEMULATE_FUNCTION_POINTER_CASTS=1 \
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,callMain,UTF8ToString,stringToUTF8,lengthBytesUTF8,setValue,getValue,HEAPU8 \
        --pre-js "$PREJS" \
        $CORE "$APP" -o "$BUILD/iup.js"
    else
      echo ">>> reuse build/iup.js (-f to rebuild)"
    fi
    cp "$HERE/web/worker.js" "$BUILD/worker.js"
    cp "$PREJS" "$BUILD/iupwasm_dom.js"
    cp "$HERE/web/index.html" "$BUILD/index.html"
    ;;
  *)
    EXPORTS="_IupOpen,_IupClose,_IupMainLoop,_IupShow,_IupShowXY,_IupPopup,_IupLabel,_IupButton,_IupToggle,_IupText,_IupMultiLine,_IupList,_IupTabs,_IupVal,_IupProgressBar,_IupTimer,_IupCanvas,_IupCalendar,_IupTable,_IupScrollbar,_IupPopover,_IupTree,_IupSetAttributeId,_IupSetStrAttributeId,_IupGetAttributeId,_IupSetAttributeId2,_IupSetStrAttributeId2,_IupGetAttributeId2,_IupRadio,_IupDestroy,_IupGetGlobal,_IupGetChild,_IupGetFloat,_IupImage,_IupImageRGB,_IupImageRGBA,_IupImageFromHandle,_IupDialog,_IupFrame,_IupFill,_iupwasmTabs0,_IupAppend,_IupGetParent,_IupGetChildPos,_IupGetHandle,_IupGetName,_IupGetInt,_IupSetAttribute,_IupSetStrAttribute,_IupSetAttributeHandle,_IupGetAttribute,_IupSetHandle,_IupSetAttributes,_IupSetGlobal,_IupSetStrGlobal,_IupMessage,_IupMessageError,_IupMessageAlarm,_IupAlarm,_IupNotify,_IupClipboard,_IupHelp,_IupFontDlg,_IupSubmenu,_IupMenuItem,_IupSeparator,_IupMenuSeparator,_iupwasmMenu0,_IupDrawBegin,_IupDrawEnd,_IupDrawGetSize,_IupDrawParentBackground,_IupDrawLine,_IupDrawRectangle,_IupDrawArc,_IupDrawEllipse,_IupDrawPolygon,_IupDrawPixel,_IupDrawRoundedRectangle,_IupDrawBezier,_IupDrawQuadraticBezier,_IupDrawText,_IupDrawImage,_IupDrawSelectRect,_IupDrawFocusRect,_IupDrawSetClipRect,_IupDrawSetClipRoundedRect,_IupDrawResetClip,_IupDrawLinearGradient,_IupDrawRadialGradient,_IupDrawLinearGradientStops,_IupDrawRadialGradientStops,_IupDrawGetTextSize,_IupDrawGetTextMetrics,_iupwasmVbox0,_iupwasmHbox0,_IupSetCallback,_iupwasmGoSetCallback,_iupwasmGoSetIdle,_iupwasmListReorder,_iupwasmDndTransfer,_iupwasmGetParamv,_iupwasmThemeChanged,_IupFileDlg,_IupMessageDlg,_IupColorDlg,_IupColorBrowser,_IupProgressDlg,_IupGetFile,_IupGetColor,_IupGetText,_IupListDialog,_malloc,_free$GL_EXPORTS$WEB_EXPORTS"
    EXPORTS="$EXPORTS,_IupAnimatedLabel,_IupBackgroundBox,_IupClassMatch,_IupConfig,_IupConfigDialogClosed,_IupConfigDialogShow,_IupConfigGetVariableDouble,_IupConfigGetVariableDoubleDef,_IupConfigGetVariableDoubleId,_IupConfigGetVariableDoubleIdDef,_IupConfigGetVariableInt,_IupConfigGetVariableIntDef,_IupConfigGetVariableIntId,_IupConfigGetVariableIntIdDef,_IupConfigGetVariableStr,_IupConfigGetVariableStrDef,_IupConfigGetVariableStrId,_IupConfigGetVariableStrIdDef,_IupConfigLoad,_IupConfigRecentInit,_IupConfigRecentUpdate,_IupConfigSave,_IupConfigSetListVariable,_IupConfigSetVariableDouble,_IupConfigSetVariableDoubleId,_IupConfigSetVariableInt,_IupConfigSetVariableIntId,_IupConfigSetVariableStr,_IupConfigSetVariableStrId,_IupConvertXYToPos,_IupCopyClassAttributes,_IupCreate,_IupDatePick,_IupDetach,_IupDetachBox,_IupDrawGetClipRect,_IupDrawGetImage,_IupDrawGetImageInfo,_IupDrawGetSvg,_IupExecute,_IupExecuteWait,_IupExitLoop,_IupExpander,_IupFlush,_IupGetAllAttributes,_IupGetAllClasses,_IupGetAllDialogs,_IupGetAllFunctions,_IupGetAllGlobals,_IupGetAllNames,_IupGetAttributeHandle,_IupGetAttributeHandleId,_IupGetAttributeHandleId2,_IupGetAttributes,_IupGetBrother,_IupGetCallback,_IupGetChildCount,_IupGetClassAttributeInfo,_IupGetClassAttributes,_IupGetClassCallbackFormat,_IupGetClassCallbacks,_IupGetClassConstructor,_IupGetClassInfo,_IupGetClassName,_IupGetClassType,_IupGetDialog,_IupGetDialogChild,_IupGetDouble,_IupGetDoubleId,_IupGetDoubleId2,_IupGetFloatId,_IupGetFloatId2,_IupGetFocus,_IupGetFunction,_IupGetGlobalInfo,_IupGetIntId,_IupGetIntId2,_IupGetIntInt,_IupGetLanguage,_IupGetLanguageString,_IupGetNextChild,_IupGetRGB,_IupGetRGBA,_IupGetRGBId,_IupGetRGBId2,_IupHide,_IupImageGetHandle,_IupImageSave,_IupImageSaveToBuffer,_IupInsert,_IupLink,_IupLog,_IupLoopStep,_IupLoopStepWait,_IupMainLoopLevel,_IupMap,_IupNextField,_IupParam,_IupPlayInput,_IupPostMessage,_IupPreviousField,_IupRecordInput,_IupRedraw,_IupRefresh,_IupRefreshChildren,_IupReparent,_IupResetAttribute,_IupSaveClassAttributes,_IupSbox,_IupScrollBox,_IupSetAttributeHandleId,_IupSetAttributeHandleId2,_IupSetClassDefaultAttribute,_IupSetFocus,_IupSetLanguage,_IupSetLanguagePack,_IupSetRGB,_IupSetRGBA,_IupSetRGBId,_IupSetRGBId2,_IupSpace,_IupSpin,_IupSpinbox,_IupSplit,_IupStringCompare,_IupTextConvertLinColToPos,_IupTextConvertPosToLinCol,_IupThread,_IupTray,_IupTreeGetId,_IupTreeGetUserId,_IupTreeSetAttributeHandle,_IupTreeSetUserId,_IupUnmap,_IupUpdate,_IupUpdateChildren,_IupUser,_IupVersion,_IupVersionDate,_IupVersionNumber,_IupVersionShow,_IupCbox,_IupZbox,_IupGridBox,_IupMultiBox,_IupNormalizer,_IupParamBox,_IupDial,_IupColorbar,_IupElementPropertiesDialog,_IupClassInfoDialog,_IupGlobalsDialog$CTRL_EXPORTS$PLOT_EXPORTS"
    RUNTIME="ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,setValue,getValue,HEAPU8"
    if [ "$MODULE_FRESH" = 1 ]; then
      echo ">>> emcc (Go module): IUP library"
      emcc $CFLAGS $GLFLAGS \
        -sMODULARIZE=1 -sEXPORT_NAME=createIupModule \
        -sNO_EXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sEMULATE_FUNCTION_POINTER_CASTS=1 \
        -sEXPORTED_FUNCTIONS="$EXPORTS" \
        -sEXPORTED_RUNTIME_METHODS="$RUNTIME" \
        --pre-js "$PREJS" \
        $CORE "$REPO/iup/wasm_bridge.c" -o "$BUILD/iup.js"
    else
      echo ">>> reuse build/iup.js (-f to rebuild)"
    fi

    if [ "$TINYGO" = 1 ]; then
      echo ">>> tinygo build (js/wasm): $APP"
      TGROOT=$(tinygo env TINYGOROOT 2>/dev/null || echo /opt/tinygo)
      [ "$OPT" = 1 ] && TGFLAGS="-no-debug" || TGFLAGS=""
      (cd "$APP" && tinygo build -target wasm $TGFLAGS -tags "$TAGS" -o "$BUILD/app.wasm" .)
      cp "$TGROOT/targets/wasm_exec.js" "$BUILD/wasm_exec.js"
    else
      echo ">>> go build (js/wasm): $APP"
      [ "$FORCE" = 1 ] && GOFORCE="-a" || GOFORCE=""
      (cd "$APP" && GOOS=js GOARCH=wasm go build $GOFORCE -tags "$TAGS" ${GOLD:+-ldflags="$GOLD"} -o "$BUILD/app.wasm" .)

      # Go's wasm uses bulk-memory/sign-ext/etc; enable them or wasm-opt rejects the module
      if [ "$OPT" = 1 ]; then
        WASMOPT="$EMSDK_DIR/upstream/bin/wasm-opt"
        [ -x "$WASMOPT" ] || WASMOPT=$(command -v wasm-opt || true)
        if [ -n "$WASMOPT" ] && [ -x "$WASMOPT" ]; then
          echo ">>> wasm-opt -O2 (Go app.wasm)"
          "$WASMOPT" -O2 --enable-bulk-memory --enable-bulk-memory-opt --enable-sign-ext \
            --enable-nontrapping-float-to-int --enable-mutable-globals \
            "$BUILD/app.wasm" -o "$BUILD/app.wasm"
        fi
      fi

      WASM_EXEC=$(go env GOROOT)/lib/wasm/wasm_exec.js
      [ -f "$WASM_EXEC" ] || WASM_EXEC=$(go env GOROOT)/misc/wasm/wasm_exec.js
      cp "$WASM_EXEC" "$BUILD/wasm_exec.js"
    fi
    cp "$HERE/web/worker.js" "$BUILD/worker.js"
    cp "$PREJS" "$BUILD/iupwasm_dom.js"
    cp "$HERE/web/index.html" "$BUILD/index.html"
    ;;
esac

echo "OK: built $BUILD"

if [ "$SHOT" = 1 ] || [ "$LOG" = 1 ] || [ -n "$CLICK" ] || [ -n "$TYPE" ] || [ -n "$KEYS" ]; then
  SHOTFILE="/tmp/screenshot.png"
  LOGFILE="/tmp/wasm_console.txt"
  echo ">>> headless Chrome: loading build (screenshot $SHOTFILE)"
  NODE_PATH="$(npm root -g)" node "$HERE/run-browser.js" "$BUILD" "$SHOTFILE" "$LOGFILE" "$CLICK" "$TYPE" "$KEYS"
  [ "$LOG" = 1 ] && { echo "--- $LOGFILE ---"; cat "$LOGFILE"; }
fi
