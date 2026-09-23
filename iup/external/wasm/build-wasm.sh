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
# Usage: build-wasm.sh [-s] [-l] [-k SELECTOR] [-y TEXT] [-K SEQ] [-c] [-f] [-O] [-t] [-T TAGS] [-h] APP
#   APP          a Go example directory, a *.go file, or a *.c/*.cpp app.
#   -s           screenshot to /tmp/screenshot.png
#   -l           console log to /tmp/wasm_console.txt
#   -k SELECTOR  click SELECTOR after load, then screenshot _after.png
#   -y TEXT      type TEXT into SELECTOR (or first input), then screenshot _after.png
#                (IUP_WAIT=ms overrides the 20s wait for heavy/slow-loading apps)
#   -K SEQ       scripted sequence, steps split by ';', each "cmd:arg":
#                click:SEL[##x,y] / dblclick:SEL[##x,y] (x,y clicks inside the element),
#                type:TEXT (ASCII; uses
#                insertText, fires no keydown),
#                rawkey:CHAR (trusted keydown carrying any character, incl. non-ASCII),
#                press:KEY (named keys/chords, e.g. Enter, ArrowUp, Control+c),
#                kpkey:CODE[##KEY[##TEXT]] (a keypad key: dispatches CODE with
#                location 3, e.g. kpkey:Numpad7##Home or kpkey:Numpad7##7##7),
#                wait:MS, reload[:MS], shot (writes _stepN.png), drag:SRC##TGT[##x,y],
#                mdrag:SEL##dx,dy,
#                tap:SEL[##x,y[##holdms]], swipe:SEL##dx,dy, pinch:SEL##scale,
#                rotate:SEL##degrees (touch is enabled for the
#                whole run as soon as one of these appears)
#   -c           wipe the build dir before building; if APP omitted, exit
#   -f           force-rebuild the emcc C module + full Go rebuild (go build -a)
#                (the C module is otherwise reused; also use after -T/-O changes)
#   -O           optimized/release build (emcc -O2, Go -ldflags='-s -w', wasm-opt)
#   -t           compile the Go program with TinyGo (smaller app.wasm)
#   -T TAGS      build tags (comma/space separated), e.g. "gl"; selects the
#                optional subsystems (gl, web, ctrl, plot, media) in both the emcc module
#                and the Go program, mirroring the desktop -tags convention.
#   -m           build only the IUP module (build/iup.js + iup.wasm), no APP needed
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
MODULE_ONLY=0
while getopts "slk:y:K:cT:Otfmh" opt; do
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
    m) MODULE_ONLY=1 ;;
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

if [ $# -lt 1 ] && [ "$MODULE_ONLY" = 0 ]; then
  echo "error: APP required" >&2
  usage 1
fi
APP="${1:-}"

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
if has_tag gl; then
  CORE="$CORE $EXTERNAL/srcgl/iup_glcanvas.c $EXTERNAL/srcgl/iup_glcanvas_wasm.c"
  GLFLAGS="-sMAX_WEBGL_VERSION=2 -sMIN_WEBGL_VERSION=1"
fi

if has_tag web; then
  CORE="$CORE $EXTERNAL/srcweb/iup_webbrowser.c $EXTERNAL/srcweb/iupwasm_webbrowser.c"
fi

if has_tag ctrl; then
  CFLAGS="$CFLAGS -I$EXTERNAL/srcctrl"
  CORE="$CORE $EXTERNAL/srcctrl/*.c $EXTERNAL/srcctrl/matrix/*.c $EXTERNAL/srcctrl/matrixex/*.c"
fi

if has_tag media; then
  CFLAGS="$CFLAGS -I$EXTERNAL/srcmedia -isystem $EXTERNAL/srcmedia/bundled"
  CORE="$CORE $EXTERNAL/srcmedia/iup_media.c $EXTERNAL/srcmedia/iup_audio.c $EXTERNAL/srcmedia/iup_miniaudio.c $EXTERNAL/srcmedia/iup_camera.c $EXTERNAL/srcmedia/iup_microphone.c $EXTERNAL/srcmedia/iupwasm_audio.c $EXTERNAL/srcmedia/iupwasm_camera.c $EXTERNAL/srcmedia/iupwasm_microphone.c"
fi

if has_tag plot; then
  CFLAGS="$CFLAGS -I$EXTERNAL/srcplot"
  CORE="$CORE $EXTERNAL/srcplot/*.cpp"
fi

wasm_exports() {
  local gofiles defined
  gofiles=$(cd "$REPO/iup" && GOOS=js GOARCH=wasm go list -e -tags "$TAGS" -f '{{range .GoFiles}}{{$.Dir}}/{{.}} {{end}}' .)
  [ -n "$gofiles" ] || return 1
  defined=$(grep -hoE '^[A-Za-z_][A-Za-z0-9_ *]*[ *]((Iup|iupwasm)[A-Za-z0-9_]*)[ ]*\([^;{]*(\{.*)?$' $CORE "$REPO/iup/wasm_bridge.c" |
    sed -E 's/^[^(]*[ *]((Iup|iupwasm)[A-Za-z0-9_]*)[ ]*\(.*/\1/' | sort -u)
  { grep -ohE "\"(Iup|iupwasm)[A-Za-z0-9_]*\"" $gofiles
    grep -ohE "['\"_](Iup|iupwasm)[A-Za-z0-9_]*['\"(]" "$HERE"/web/*.js
  } | sed -E "s/^['\"_]//; s/['\"(]$//" | sort -u | comm -12 - <(echo "$defined") |
    sed 's/^/_/' | paste -sd, - | sed 's/^/_malloc,_free,/'
}

if [ "$FORCE" = 1 ] || [ ! -f "$BUILD/iup.js" ]; then MODULE_FRESH=1; else MODULE_FRESH=0; fi

case "$APP" in
  *.c | *.cpp)
    if [ "$MODULE_FRESH" = 1 ]; then
      echo ">>> emcc (C/C++ app, runs on a Web Worker): $APP"
      emcc $CFLAGS $GLFLAGS \
        -pthread -sPTHREAD_POOL_SIZE=4 \
        -sMODULARIZE=1 -sEXPORT_NAME=createIupModule -sINVOKE_RUN=0 \
        -sERROR_ON_UNDEFINED_SYMBOLS=1 -sNO_EXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sEMULATE_FUNCTION_POINTER_CASTS=1 \
        -sEXPORTED_FUNCTIONS=_main,_malloc,_free \
        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,callMain,UTF8ToString,stringToUTF8,lengthBytesUTF8,setValue,getValue,HEAPU8,FS,IDBFS \
        -lidbfs.js \
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
    EXPORTS=$(wasm_exports)
    RUNTIME="ccall,cwrap,UTF8ToString,stringToUTF8,lengthBytesUTF8,setValue,getValue,HEAPU8,FS,IDBFS"
    if [ "$MODULE_FRESH" = 1 ]; then
      echo ">>> emcc (Go module): IUP library"
      emcc $CFLAGS $GLFLAGS \
        -sMODULARIZE=1 -sEXPORT_NAME=createIupModule \
        -sNO_EXIT_RUNTIME=1 -sALLOW_MEMORY_GROWTH=1 -sEMULATE_FUNCTION_POINTER_CASTS=1 \
        -sEXPORTED_FUNCTIONS="$EXPORTS" \
        -sEXPORTED_RUNTIME_METHODS="$RUNTIME" \
        -lidbfs.js \
        --pre-js "$PREJS" \
        $CORE "$REPO/iup/wasm_bridge.c" -o "$BUILD/iup.js"
    else
      echo ">>> reuse build/iup.js (-f to rebuild)"
    fi

    if [ "$MODULE_ONLY" = 1 ]; then
      echo "OK: built $BUILD/iup.js"
      exit 0
    fi

    if [ "$TINYGO" = 1 ]; then
      echo ">>> tinygo build (js/wasm): $APP"
      TGROOT=$(tinygo env TINYGOROOT 2>/dev/null || echo /opt/tinygo)
      [ "$OPT" = 1 ] && TGFLAGS="-no-debug" || TGFLAGS=""
      # tinygo 0.41 caps at Go 1.26; drop the pin once it accepts the system toolchain
      (cd "$APP" && GOTOOLCHAIN=${GOTOOLCHAIN:-go1.26.3} tinygo build -target wasm $TGFLAGS -tags "$TAGS" -o "$BUILD/app.wasm" .)
      cp "$TGROOT/targets/wasm_exec.js" "$BUILD/wasm_exec.js"
    else
      echo ">>> go build (js/wasm): $APP"
      [ "$FORCE" = 1 ] && GOFORCE="-a" || GOFORCE=""
      # pinned: js/wasm has no cgo, and an exported CGO_ENABLED=1 makes Go reject wasm_bridge.c
      (cd "$APP" && CGO_ENABLED=0 GOOS=js GOARCH=wasm go build $GOFORCE -tags "$TAGS" ${GOLD:+-ldflags="$GOLD"} -o "$BUILD/app.wasm" .)

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
