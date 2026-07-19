//go:build !cgo && !js

package iup

import (
	"sync"

	"github.com/ebitengine/purego"
)

var (
	iupOpen          func(argc, argv uintptr) int32
	iupClose         func()
	iupMainLoop      func() int32
	iupExitLoop      func()
	iupFlush         func()
	iupLoopStep      func() int32
	iupMainLoopLevel func() int32
	iupVersion       func() string
	iupVersionDate   func() string
	iupVersionNumber func() int32

	iupSetStrGlobal  func(name, value string)
	iupSetGlobal     func(name string, value uintptr)
	iupGetGlobal     func(name string) string
	iupGetGlobalRaw  func(name string) uintptr
	iupStringCompare func(s1, s2 string, casesensitive, lexicographic int32) int32

	iupSetStrAttribute      func(ih uintptr, name, value string)
	iupSetAttribute         func(ih uintptr, name string, value uintptr)
	iupSetInt               func(ih uintptr, name string, v int32)
	iupSetFloat             func(ih uintptr, name string, v float32)
	iupSetDouble            func(ih uintptr, name string, v float64)
	iupSetRGB               func(ih uintptr, name string, r, g, b uint8)
	iupSetRGBA              func(ih uintptr, name string, r, g, b, a uint8)
	iupSetAttributes        func(ih uintptr, str string) uintptr
	iupResetAttribute       func(ih uintptr, name string)
	iupSetAttributeHandle   func(ih uintptr, name string, named uintptr)
	iupSetAttributeHandleId func(ih uintptr, name string, id int32, named uintptr)
	iupSetAttributeHandleI2 func(ih uintptr, name string, lin, col int32, named uintptr)
	iupSetAttributeId       func(ih uintptr, name string, id int32, value uintptr)
	iupSetStrAttributeId    func(ih uintptr, name string, id int32, value string)
	iupSetIntId             func(ih uintptr, name string, id int32, v int32)
	iupSetFloatId           func(ih uintptr, name string, id int32, v float32)
	iupSetDoubleId          func(ih uintptr, name string, id int32, v float64)
	iupSetRGBId             func(ih uintptr, name string, id int32, r, g, b uint8)
	iupSetAttributeId2      func(ih uintptr, name string, lin, col int32, value uintptr)
	iupSetStrAttributeId2   func(ih uintptr, name string, lin, col int32, value string)
	iupSetIntId2            func(ih uintptr, name string, lin, col int32, v int32)
	iupSetFloatId2          func(ih uintptr, name string, lin, col int32, v float32)
	iupSetDoubleId2         func(ih uintptr, name string, lin, col int32, v float64)
	iupSetRGBId2            func(ih uintptr, name string, lin, col int32, r, g, b uint8)

	iupGetAttribute         func(ih uintptr, name string) string
	iupGetAttributeRaw      func(ih uintptr, name string) uintptr
	iupGetAttributeId       func(ih uintptr, name string, id int32) string
	iupGetAttributeIdRaw    func(ih uintptr, name string, id int32) uintptr
	iupGetAttributeId2      func(ih uintptr, name string, lin, col int32) string
	iupGetAttributeId2Raw   func(ih uintptr, name string, lin, col int32) uintptr
	iupGetAllAttributes     func(ih uintptr, names *uintptr, n int32) int32
	iupGetAttributes        func(ih uintptr) string
	iupGetAttributeHandle   func(ih uintptr, name string) uintptr
	iupGetAttributeHandleId func(ih uintptr, name string, id int32) uintptr
	iupGetAttributeHandleI2 func(ih uintptr, name string, lin, col int32) uintptr
	iupGetInt               func(ih uintptr, name string) int32
	iupGetIntInt            func(ih uintptr, name string, i1, i2 *int32) int32
	iupGetFloat             func(ih uintptr, name string) float32
	iupGetDouble            func(ih uintptr, name string) float64
	iupGetRGB               func(ih uintptr, name string, r, g, b *uint8)
	iupGetRGBA              func(ih uintptr, name string, r, g, b, a *uint8)
	iupGetIntId             func(ih uintptr, name string, id int32) int32
	iupGetFloatId           func(ih uintptr, name string, id int32) float32
	iupGetDoubleId          func(ih uintptr, name string, id int32) float64
	iupGetRGBId             func(ih uintptr, name string, id int32, r, g, b *uint8)
	iupGetIntId2            func(ih uintptr, name string, lin, col int32) int32
	iupGetFloatId2          func(ih uintptr, name string, lin, col int32) float32
	iupGetDoubleId2         func(ih uintptr, name string, lin, col int32) float64
	iupGetRGBId2            func(ih uintptr, name string, lin, col int32, r, g, b *uint8)

	iupSetCallback func(ih uintptr, name string, cb uintptr) uintptr
	iupGetCallback func(ih uintptr, name string) uintptr
	iupSetFunction func(name string, cb uintptr) uintptr
	iupGetFunction func(name string) uintptr

	iupButton func(title *byte) uintptr
	iupLabel  func(title *byte) uintptr
	iupFill   func() uintptr
	iupVboxv  func(children []uintptr) uintptr
	iupHboxv  func(children []uintptr) uintptr
	iupDialog func(child uintptr) uintptr
	iupTimer  func() uintptr

	iupShow      func(ih uintptr) int32
	iupShowXY    func(ih uintptr, x, y int32) int32
	iupMap       func(ih uintptr) int32
	iupRefresh   func(ih uintptr)
	iupUpdate    func(ih uintptr)
	iupDestroy   func(ih uintptr)
	iupSetHandle func(name string, ih uintptr)
	iupGetHandle func(name string) uintptr

	iupAnimatedLabel          func(anim uintptr) uintptr
	iupCalendar               func() uintptr
	iupCanvas                 func() uintptr
	iupColorbar               func() uintptr
	iupColorBrowser           func() uintptr
	iupDatePick               func() uintptr
	iupDial                   func(orientation *byte) uintptr
	iupSeparator              func() uintptr
	iupLink                   func(url, title *byte) uintptr
	iupList                   func() uintptr
	iupProgressBar            func() uintptr
	iupSpin                   func() uintptr
	iupSpinbox                func(child uintptr) uintptr
	iupText                   func() uintptr
	iupMultiLine              func() uintptr
	iupToggle                 func(title *byte) uintptr
	iupTree                   func() uintptr
	iupTable                  func() uintptr
	iupVal                    func(t *byte) uintptr
	iupScrollbar              func(orientation *byte) uintptr
	iupPopover                func(child uintptr) uintptr
	iupTextConvertLinColToPos func(ih uintptr, lin, col int32, pos *int32)
	iupTextConvertPosToLinCol func(ih uintptr, pos int32, lin, col *int32)
	iupTreeSetUserId          func(ih uintptr, id int32, userid uintptr) int32
	iupTreeGetUserId          func(ih uintptr, id int32) uintptr
	iupTreeGetId              func(ih uintptr, userid uintptr) int32
	iupTreeSetAttributeHandle func(ih uintptr, name string, id int32, named uintptr)

	iupSpace         func() uintptr
	iupCboxv         func(children []uintptr) uintptr
	iupGridBoxv      func(children []uintptr) uintptr
	iupMultiBoxv     func(children []uintptr) uintptr
	iupZboxv         func(children []uintptr) uintptr
	iupRadio         func(child uintptr) uintptr
	iupNormalizerv   func(list []uintptr) uintptr
	iupFrame         func(child uintptr) uintptr
	iupTabsv         func(children []uintptr) uintptr
	iupBackgroundBox func(child uintptr) uintptr
	iupScrollBox     func(child uintptr) uintptr
	iupDetachBox     func(child uintptr) uintptr
	iupExpander      func(child uintptr) uintptr
	iupSbox          func(child uintptr) uintptr
	iupSplit         func(c1, c2 uintptr) uintptr

	iupPopup        func(ih uintptr, x, y int32) int32
	iupHide         func(ih uintptr) int32
	iupFileDlg      func() uintptr
	iupMessageDlg   func() uintptr
	iupColorDlg     func() uintptr
	iupFontDlg      func() uintptr
	iupProgressDlg  func() uintptr
	iupAlarm        func(title, msg, b1, b2, b3 *byte) int32
	iupGetColor     func(x, y int32, r, g, b *uint8) int32
	iupMessage      func(title, msg *byte)
	iupVersionShow  func()
	iupMessageError func(parent uintptr, msg *byte)
	iupMessageAlarm func(parent uintptr, title, msg, buttons *byte) int32
	iupGetText      func(title, buf *byte, maxsize int32) int32

	iupAppend         func(ih, child uintptr) uintptr
	iupDetach         func(child uintptr)
	iupInsert         func(ih, ref, child uintptr) uintptr
	iupReparent       func(ih, newParent, ref uintptr) int32
	iupGetParent      func(ih uintptr) uintptr
	iupGetChild       func(ih uintptr, pos int32) uintptr
	iupGetChildPos    func(ih, child uintptr) int32
	iupGetChildCount  func(ih uintptr) int32
	iupGetNextChild   func(ih, child uintptr) uintptr
	iupGetBrother     func(ih uintptr) uintptr
	iupGetDialog      func(ih uintptr) uintptr
	iupGetDialogChild func(ih uintptr, name string) uintptr

	iupRefreshChildren func(ih uintptr)
	iupUpdateChildren  func(ih uintptr)
	iupRedraw          func(ih uintptr, children int32)
	iupUnmap           func(ih uintptr)
	iupConvertXYToPos  func(ih uintptr, x, y int32) int32

	iupImage           func(w, h int32, pix []byte) uintptr
	iupImageRGB        func(w, h int32, pix []byte) uintptr
	iupImageRGBA       func(w, h int32, pix []byte) uintptr
	iupImageGetHandle  func(name string) uintptr
	iupImageFromHandle func(handle uintptr) uintptr
	iupMenuv           func(children []uintptr) uintptr
	iupMenuItem        func(title *byte) uintptr
	iupMenuSeparator   func() uintptr
	iupSubmenu         func(title *byte, child uintptr) uintptr
	iupNextField       func(ih uintptr) uintptr
	iupPreviousField   func(ih uintptr) uintptr
	iupGetFocus        func() uintptr
	iupSetFocus        func(ih uintptr) uintptr
	iupGetName         func(ih uintptr) string
	iupClipboard       func() uintptr
	iupThread          func() uintptr
	iupTray            func() uintptr
	iupGetFile         func(buf *byte) int32

	iupConfig                       func() uintptr
	iupConfigLoad                   func(ih uintptr) int32
	iupConfigSave                   func(ih uintptr) int32
	iupConfigSetVariableStr         func(ih uintptr, group, key string, value *byte)
	iupConfigSetVariableStrId       func(ih uintptr, group, key string, id int32, value *byte)
	iupConfigSetVariableInt         func(ih uintptr, group, key string, value int32)
	iupConfigSetVariableIntId       func(ih uintptr, group, key string, id, value int32)
	iupConfigSetVariableDouble      func(ih uintptr, group, key string, value float64)
	iupConfigSetVariableDoubleId    func(ih uintptr, group, key string, id int32, value float64)
	iupConfigGetVariableStr         func(ih uintptr, group, key string) string
	iupConfigGetVariableStrId       func(ih uintptr, group, key string, id int32) string
	iupConfigGetVariableInt         func(ih uintptr, group, key string) int32
	iupConfigGetVariableIntId       func(ih uintptr, group, key string, id int32) int32
	iupConfigGetVariableDouble      func(ih uintptr, group, key string) float64
	iupConfigGetVariableDoubleId    func(ih uintptr, group, key string, id int32) float64
	iupConfigGetVariableStrDef      func(ih uintptr, group, key, def string) string
	iupConfigGetVariableStrIdDef    func(ih uintptr, group, key string, id int32, def string) string
	iupConfigGetVariableIntDef      func(ih uintptr, group, key string, def int32) int32
	iupConfigGetVariableIntIdDef    func(ih uintptr, group, key string, id, def int32) int32
	iupConfigGetVariableDoubleDef   func(ih uintptr, group, key string, def float64) float64
	iupConfigGetVariableDoubleIdDef func(ih uintptr, group, key string, id int32, def float64) float64
	iupConfigSetListVariable        func(ih uintptr, group, key string, value *byte, add int32)
	iupConfigDialogShow             func(ih, dialog uintptr, name string)
	iupConfigDialogClosed           func(ih, dialog uintptr, name string)
	iupConfigRecentInit             func(ih, menuOrList uintptr, cb uintptr, maxRecent int32)
	iupConfigRecentUpdate           func(ih uintptr, filename string)

	iupGetAllClasses      func(names *uintptr, n int32) int32
	iupGetClassAttributes func(className string, names *uintptr, max int32) int32
	iupGetClassCallbacks  func(className string, names *uintptr, max int32) int32
	iupGetClassType       func(ih uintptr) string
	iupGetClassName       func(ih uintptr) string
	iupGetAllGlobals      func(names *uintptr, n int32) int32

	iupPostMessage func(ih uintptr, s string, i int32, d float64, p uintptr)

	iupDrawBegin              func(ih uintptr)
	iupDrawEnd                func(ih uintptr)
	iupDrawSetClipRect        func(ih uintptr, x1, y1, x2, y2 int32)
	iupDrawSetClipRoundedRect func(ih uintptr, x1, y1, x2, y2, corner int32)
	iupDrawResetClip          func(ih uintptr)
	iupDrawGetClipRect        func(ih uintptr, x1, y1, x2, y2 *int32)
	iupDrawParentBackground   func(ih uintptr)
	iupDrawLine               func(ih uintptr, x1, y1, x2, y2 int32)
	iupDrawRectangle          func(ih uintptr, x1, y1, x2, y2 int32)
	iupDrawArc                func(ih uintptr, x1, y1, x2, y2 int32, a1, a2 float64)
	iupDrawEllipse            func(ih uintptr, x1, y1, x2, y2 int32)
	iupDrawPolygon            func(ih uintptr, points []int32, count int32)
	iupDrawPixel              func(ih uintptr, x, y int32)
	iupDrawRoundedRectangle   func(ih uintptr, x1, y1, x2, y2, corner int32)
	iupDrawText               func(ih uintptr, str string, length, x, y, w, h int32)
	iupDrawImage              func(ih uintptr, name string, x, y, w, h int32)
	iupDrawSelectRect         func(ih uintptr, x1, y1, x2, y2 int32)
	iupDrawFocusRect          func(ih uintptr, x1, y1, x2, y2 int32)
	iupDrawBezier             func(ih uintptr, x1, y1, x2, y2, x3, y3, x4, y4 int32)
	iupDrawQuadraticBezier    func(ih uintptr, x1, y1, x2, y2, x3, y3 int32)
	iupDrawGetSize            func(ih uintptr, w, h *int32)
	iupDrawGetTextSize        func(ih uintptr, str string, length int32, w, h *int32)
	iupDrawGetImageInfo       func(name string, w, h, bpp *int32)
	iupDrawGetImage           func(ih uintptr) uintptr
	iupDrawGetSvg             func(ih uintptr) string
	iupDrawLinearGradient     func(ih uintptr, x1, y1, x2, y2 int32, angle float32, color1, color2 string)
	iupDrawRadialGradient     func(ih uintptr, cx, cy, radius int32, colorCenter, colorEdge string)

	iupSetLanguage              func(lng string)
	iupGetLanguage              func() string
	iupStoreLanguageString      func(name, str string)
	iupGetLanguageString        func(name string) string
	iupSetLanguagePack          func(ih uintptr)
	iupExecute                  func(fileName, parameters string) int32
	iupExecuteWait              func(fileName, parameters string) int32
	iupHelp                     func(url string) int32
	iupLog                      func(typ, format, s string)
	iupLoopStepWait             func() int32
	iupPlayInput                func(fileName *byte) int32
	iupRecordInput              func(fileName *byte, mode int32) int32
	iupUser                     func() uintptr
	iupNotify                   func() uintptr
	iupGetAllNames              func(names *uintptr, n int32) int32
	iupGetAllDialogs            func(names *uintptr, n int32) int32
	iupGetAllFunctions          func(names *uintptr, max int32) int32
	iupClassMatch               func(ih uintptr, className string) int32
	iupCreate                   func(className string) uintptr
	iupCopyClassAttributes      func(src, dst uintptr)
	iupSaveClassAttributes      func(ih uintptr)
	iupSetClassDefaultAttribute func(className, name, value string)

	iupGetClassInfo            func(className string, parent, nativeType *uintptr, childType, isInteractive, hasAttribID *int32) int32
	iupGetClassConstructor     func(className string, format, formatAttr *uintptr) int32
	iupGetClassAttributeInfo   func(className, name string, def, sysDef *uintptr, flags *int32) int32
	iupGetClassCallbackFormat  func(className, name string) string
	iupGetGlobalInfo           func(name string, flags, drivers *int32) int32
	iupClassInfoDialog         func(dialog uintptr) uintptr
	iupElementPropertiesDialog func(parent, elem uintptr) uintptr
	iupGlobalsDialog           func() uintptr
	iupImageSave               func(ih uintptr, filename, format string) int32
	iupImageSaveToBuffer       func(ih uintptr, format string, size *int32) uintptr
	iupParam                   func(format string) uintptr
	iupParamBoxv               func(params []uintptr) uintptr
	iupListDialog              func(typ int32, title *byte, size int32, list *uintptr, op, maxCol, maxLin int32, marks *int32) int32
	iupGetParamv               func(title *byte, action, userData uintptr, format string, count, extra int32, data *uintptr) int32
	cfree                      func(ptr uintptr)
)

var baseOnce sync.Once

func init() { ensureBase() }

// ensureBase loads and binds the core libiup exactly once. The tagged extra-lib
// loaders call it first so libiup is always mapped before an extra lib whose
// NEEDED libiup.so.4 would otherwise pull in a second (system) copy.
func ensureBase() {
	baseOnce.Do(func() {
		lib := openLib("iup")
		if lib == 0 {
			panic("iup: cannot load libiup")
		}
		reg := func(fptr any, name string) { purego.RegisterLibFunc(fptr, lib, name) }

		reg(&iupOpen, "IupOpen")
		reg(&iupClose, "IupClose")
		reg(&iupMainLoop, "IupMainLoop")
		reg(&iupExitLoop, "IupExitLoop")
		reg(&iupFlush, "IupFlush")
		reg(&iupLoopStep, "IupLoopStep")
		reg(&iupMainLoopLevel, "IupMainLoopLevel")
		reg(&iupVersion, "IupVersion")
		reg(&iupVersionDate, "IupVersionDate")
		reg(&iupVersionNumber, "IupVersionNumber")

		reg(&iupSetStrGlobal, "IupSetStrGlobal")
		reg(&iupSetGlobal, "IupSetGlobal")
		reg(&iupGetGlobal, "IupGetGlobal")
		reg(&iupGetGlobalRaw, "IupGetGlobal")
		reg(&iupStringCompare, "IupStringCompare")

		reg(&iupSetStrAttribute, "IupSetStrAttribute")
		reg(&iupSetAttribute, "IupSetAttribute")
		reg(&iupSetInt, "IupSetInt")
		reg(&iupSetFloat, "IupSetFloat")
		reg(&iupSetDouble, "IupSetDouble")
		reg(&iupSetRGB, "IupSetRGB")
		reg(&iupSetRGBA, "IupSetRGBA")
		reg(&iupSetAttributes, "IupSetAttributes")
		reg(&iupResetAttribute, "IupResetAttribute")
		reg(&iupSetAttributeHandle, "IupSetAttributeHandle")
		reg(&iupSetAttributeHandleId, "IupSetAttributeHandleId")
		reg(&iupSetAttributeHandleI2, "IupSetAttributeHandleId2")
		reg(&iupSetAttributeId, "IupSetAttributeId")
		reg(&iupSetStrAttributeId, "IupSetStrAttributeId")
		reg(&iupSetIntId, "IupSetIntId")
		reg(&iupSetFloatId, "IupSetFloatId")
		reg(&iupSetDoubleId, "IupSetDoubleId")
		reg(&iupSetRGBId, "IupSetRGBId")
		reg(&iupSetAttributeId2, "IupSetAttributeId2")
		reg(&iupSetStrAttributeId2, "IupSetStrAttributeId2")
		reg(&iupSetIntId2, "IupSetIntId2")
		reg(&iupSetFloatId2, "IupSetFloatId2")
		reg(&iupSetDoubleId2, "IupSetDoubleId2")
		reg(&iupSetRGBId2, "IupSetRGBId2")

		reg(&iupGetAttribute, "IupGetAttribute")
		reg(&iupGetAttributeRaw, "IupGetAttribute")
		reg(&iupGetAttributeId, "IupGetAttributeId")
		reg(&iupGetAttributeIdRaw, "IupGetAttributeId")
		reg(&iupGetAttributeId2, "IupGetAttributeId2")
		reg(&iupGetAttributeId2Raw, "IupGetAttributeId2")
		reg(&iupGetAllAttributes, "IupGetAllAttributes")
		reg(&iupGetAttributes, "IupGetAttributes")
		reg(&iupGetAttributeHandle, "IupGetAttributeHandle")
		reg(&iupGetAttributeHandleId, "IupGetAttributeHandleId")
		reg(&iupGetAttributeHandleI2, "IupGetAttributeHandleId2")
		reg(&iupGetInt, "IupGetInt")
		reg(&iupGetIntInt, "IupGetIntInt")
		reg(&iupGetFloat, "IupGetFloat")
		reg(&iupGetDouble, "IupGetDouble")
		reg(&iupGetRGB, "IupGetRGB")
		reg(&iupGetRGBA, "IupGetRGBA")
		reg(&iupGetIntId, "IupGetIntId")
		reg(&iupGetFloatId, "IupGetFloatId")
		reg(&iupGetDoubleId, "IupGetDoubleId")
		reg(&iupGetRGBId, "IupGetRGBId")
		reg(&iupGetIntId2, "IupGetIntId2")
		reg(&iupGetFloatId2, "IupGetFloatId2")
		reg(&iupGetDoubleId2, "IupGetDoubleId2")
		reg(&iupGetRGBId2, "IupGetRGBId2")

		reg(&iupSetCallback, "IupSetCallback")
		reg(&iupGetCallback, "IupGetCallback")
		reg(&iupSetFunction, "IupSetFunction")
		reg(&iupGetFunction, "IupGetFunction")

		reg(&iupButton, "IupButton")
		reg(&iupLabel, "IupLabel")
		reg(&iupFill, "IupFill")
		reg(&iupVboxv, "IupVboxv")
		reg(&iupHboxv, "IupHboxv")
		reg(&iupDialog, "IupDialog")
		reg(&iupTimer, "IupTimer")

		reg(&iupShow, "IupShow")
		reg(&iupShowXY, "IupShowXY")
		reg(&iupMap, "IupMap")
		reg(&iupRefresh, "IupRefresh")
		reg(&iupUpdate, "IupUpdate")
		reg(&iupDestroy, "IupDestroy")
		reg(&iupSetHandle, "IupSetHandle")
		reg(&iupGetHandle, "IupGetHandle")

		reg(&iupAnimatedLabel, "IupAnimatedLabel")
		reg(&iupCalendar, "IupCalendar")
		reg(&iupCanvas, "IupCanvas")
		reg(&iupColorbar, "IupColorbar")
		reg(&iupColorBrowser, "IupColorBrowser")
		reg(&iupDatePick, "IupDatePick")
		reg(&iupDial, "IupDial")
		reg(&iupSeparator, "IupSeparator")
		reg(&iupLink, "IupLink")
		reg(&iupList, "IupList")
		reg(&iupProgressBar, "IupProgressBar")
		reg(&iupSpin, "IupSpin")
		reg(&iupSpinbox, "IupSpinbox")
		reg(&iupText, "IupText")
		reg(&iupMultiLine, "IupMultiLine")
		reg(&iupToggle, "IupToggle")
		reg(&iupTree, "IupTree")
		reg(&iupTable, "IupTable")
		reg(&iupVal, "IupVal")
		reg(&iupScrollbar, "IupScrollbar")
		reg(&iupPopover, "IupPopover")
		reg(&iupTextConvertLinColToPos, "IupTextConvertLinColToPos")
		reg(&iupTextConvertPosToLinCol, "IupTextConvertPosToLinCol")
		reg(&iupTreeSetUserId, "IupTreeSetUserId")
		reg(&iupTreeGetUserId, "IupTreeGetUserId")
		reg(&iupTreeGetId, "IupTreeGetId")
		reg(&iupTreeSetAttributeHandle, "IupTreeSetAttributeHandle")

		reg(&iupSpace, "IupSpace")
		reg(&iupCboxv, "IupCboxv")
		reg(&iupGridBoxv, "IupGridBoxv")
		reg(&iupMultiBoxv, "IupMultiBoxv")
		reg(&iupZboxv, "IupZboxv")
		reg(&iupRadio, "IupRadio")
		reg(&iupNormalizerv, "IupNormalizerv")
		reg(&iupFrame, "IupFrame")
		reg(&iupTabsv, "IupTabsv")
		reg(&iupBackgroundBox, "IupBackgroundBox")
		reg(&iupScrollBox, "IupScrollBox")
		reg(&iupDetachBox, "IupDetachBox")
		reg(&iupExpander, "IupExpander")
		reg(&iupSbox, "IupSbox")
		reg(&iupSplit, "IupSplit")

		reg(&iupPopup, "IupPopup")
		reg(&iupHide, "IupHide")
		reg(&iupFileDlg, "IupFileDlg")
		reg(&iupMessageDlg, "IupMessageDlg")
		reg(&iupColorDlg, "IupColorDlg")
		reg(&iupFontDlg, "IupFontDlg")
		reg(&iupProgressDlg, "IupProgressDlg")
		reg(&iupAlarm, "IupAlarm")
		reg(&iupGetColor, "IupGetColor")
		reg(&iupMessage, "IupMessage")
		reg(&iupVersionShow, "IupVersionShow")
		reg(&iupMessageError, "IupMessageError")
		reg(&iupMessageAlarm, "IupMessageAlarm")
		reg(&iupGetText, "IupGetText")

		reg(&iupAppend, "IupAppend")
		reg(&iupDetach, "IupDetach")
		reg(&iupInsert, "IupInsert")
		reg(&iupReparent, "IupReparent")
		reg(&iupGetParent, "IupGetParent")
		reg(&iupGetChild, "IupGetChild")
		reg(&iupGetChildPos, "IupGetChildPos")
		reg(&iupGetChildCount, "IupGetChildCount")
		reg(&iupGetNextChild, "IupGetNextChild")
		reg(&iupGetBrother, "IupGetBrother")
		reg(&iupGetDialog, "IupGetDialog")
		reg(&iupGetDialogChild, "IupGetDialogChild")

		reg(&iupRefreshChildren, "IupRefreshChildren")
		reg(&iupUpdateChildren, "IupUpdateChildren")
		reg(&iupRedraw, "IupRedraw")
		reg(&iupUnmap, "IupUnmap")
		reg(&iupConvertXYToPos, "IupConvertXYToPos")

		reg(&iupImage, "IupImage")
		reg(&iupImageRGB, "IupImageRGB")
		reg(&iupImageRGBA, "IupImageRGBA")
		reg(&iupImageGetHandle, "IupImageGetHandle")
		reg(&iupImageFromHandle, "IupImageFromHandle")
		reg(&iupMenuv, "IupMenuv")
		reg(&iupMenuItem, "IupMenuItem")
		reg(&iupMenuSeparator, "IupMenuSeparator")
		reg(&iupSubmenu, "IupSubmenu")
		reg(&iupNextField, "IupNextField")
		reg(&iupPreviousField, "IupPreviousField")
		reg(&iupGetFocus, "IupGetFocus")
		reg(&iupSetFocus, "IupSetFocus")
		reg(&iupGetName, "IupGetName")
		reg(&iupClipboard, "IupClipboard")
		reg(&iupThread, "IupThread")
		reg(&iupTray, "IupTray")
		reg(&iupGetFile, "IupGetFile")

		reg(&iupConfig, "IupConfig")
		reg(&iupConfigLoad, "IupConfigLoad")
		reg(&iupConfigSave, "IupConfigSave")
		reg(&iupConfigSetVariableStr, "IupConfigSetVariableStr")
		reg(&iupConfigSetVariableStrId, "IupConfigSetVariableStrId")
		reg(&iupConfigSetVariableInt, "IupConfigSetVariableInt")
		reg(&iupConfigSetVariableIntId, "IupConfigSetVariableIntId")
		reg(&iupConfigSetVariableDouble, "IupConfigSetVariableDouble")
		reg(&iupConfigSetVariableDoubleId, "IupConfigSetVariableDoubleId")
		reg(&iupConfigGetVariableStr, "IupConfigGetVariableStr")
		reg(&iupConfigGetVariableStrId, "IupConfigGetVariableStrId")
		reg(&iupConfigGetVariableInt, "IupConfigGetVariableInt")
		reg(&iupConfigGetVariableIntId, "IupConfigGetVariableIntId")
		reg(&iupConfigGetVariableDouble, "IupConfigGetVariableDouble")
		reg(&iupConfigGetVariableDoubleId, "IupConfigGetVariableDoubleId")
		reg(&iupConfigGetVariableStrDef, "IupConfigGetVariableStrDef")
		reg(&iupConfigGetVariableStrIdDef, "IupConfigGetVariableStrIdDef")
		reg(&iupConfigGetVariableIntDef, "IupConfigGetVariableIntDef")
		reg(&iupConfigGetVariableIntIdDef, "IupConfigGetVariableIntIdDef")
		reg(&iupConfigGetVariableDoubleDef, "IupConfigGetVariableDoubleDef")
		reg(&iupConfigGetVariableDoubleIdDef, "IupConfigGetVariableDoubleIdDef")
		reg(&iupConfigSetListVariable, "IupConfigSetListVariable")
		reg(&iupConfigDialogShow, "IupConfigDialogShow")
		reg(&iupConfigDialogClosed, "IupConfigDialogClosed")
		reg(&iupConfigRecentInit, "IupConfigRecentInit")
		reg(&iupConfigRecentUpdate, "IupConfigRecentUpdate")

		reg(&iupGetAllClasses, "IupGetAllClasses")
		reg(&iupGetClassAttributes, "IupGetClassAttributes")
		reg(&iupGetClassCallbacks, "IupGetClassCallbacks")
		reg(&iupGetClassType, "IupGetClassType")
		reg(&iupGetClassName, "IupGetClassName")
		reg(&iupGetAllGlobals, "IupGetAllGlobals")

		reg(&iupPostMessage, "IupPostMessage")

		reg(&iupDrawBegin, "IupDrawBegin")
		reg(&iupDrawEnd, "IupDrawEnd")
		reg(&iupDrawSetClipRect, "IupDrawSetClipRect")
		reg(&iupDrawSetClipRoundedRect, "IupDrawSetClipRoundedRect")
		reg(&iupDrawResetClip, "IupDrawResetClip")
		reg(&iupDrawGetClipRect, "IupDrawGetClipRect")
		reg(&iupDrawParentBackground, "IupDrawParentBackground")
		reg(&iupDrawLine, "IupDrawLine")
		reg(&iupDrawRectangle, "IupDrawRectangle")
		reg(&iupDrawArc, "IupDrawArc")
		reg(&iupDrawEllipse, "IupDrawEllipse")
		reg(&iupDrawPolygon, "IupDrawPolygon")
		reg(&iupDrawPixel, "IupDrawPixel")
		reg(&iupDrawRoundedRectangle, "IupDrawRoundedRectangle")
		reg(&iupDrawText, "IupDrawText")
		reg(&iupDrawImage, "IupDrawImage")
		reg(&iupDrawSelectRect, "IupDrawSelectRect")
		reg(&iupDrawFocusRect, "IupDrawFocusRect")
		reg(&iupDrawBezier, "IupDrawBezier")
		reg(&iupDrawQuadraticBezier, "IupDrawQuadraticBezier")
		reg(&iupDrawGetSize, "IupDrawGetSize")
		reg(&iupDrawGetTextSize, "IupDrawGetTextSize")
		reg(&iupDrawGetImageInfo, "IupDrawGetImageInfo")
		reg(&iupDrawGetImage, "IupDrawGetImage")
		reg(&iupDrawGetSvg, "IupDrawGetSvg")
		reg(&iupDrawLinearGradient, "IupDrawLinearGradient")
		reg(&iupDrawRadialGradient, "IupDrawRadialGradient")

		reg(&iupSetLanguage, "IupSetLanguage")
		reg(&iupGetLanguage, "IupGetLanguage")
		reg(&iupStoreLanguageString, "IupStoreLanguageString")
		reg(&iupGetLanguageString, "IupGetLanguageString")
		reg(&iupSetLanguagePack, "IupSetLanguagePack")
		reg(&iupExecute, "IupExecute")
		reg(&iupExecuteWait, "IupExecuteWait")
		reg(&iupHelp, "IupHelp")
		reg(&iupLog, "IupLog")
		reg(&iupLoopStepWait, "IupLoopStepWait")
		reg(&iupPlayInput, "IupPlayInput")
		reg(&iupRecordInput, "IupRecordInput")
		reg(&iupUser, "IupUser")
		reg(&iupNotify, "IupNotify")
		reg(&iupGetAllNames, "IupGetAllNames")
		reg(&iupGetAllDialogs, "IupGetAllDialogs")
		reg(&iupGetAllFunctions, "IupGetAllFunctions")
		reg(&iupClassMatch, "IupClassMatch")
		reg(&iupCreate, "IupCreate")
		reg(&iupCopyClassAttributes, "IupCopyClassAttributes")
		reg(&iupSaveClassAttributes, "IupSaveClassAttributes")
		reg(&iupSetClassDefaultAttribute, "IupSetClassDefaultAttribute")

		reg(&iupGetClassInfo, "IupGetClassInfo")
		reg(&iupGetClassConstructor, "IupGetClassConstructor")
		reg(&iupGetClassAttributeInfo, "IupGetClassAttributeInfo")
		reg(&iupGetClassCallbackFormat, "IupGetClassCallbackFormat")
		reg(&iupGetGlobalInfo, "IupGetGlobalInfo")
		reg(&iupClassInfoDialog, "IupClassInfoDialog")
		reg(&iupElementPropertiesDialog, "IupElementPropertiesDialog")
		reg(&iupGlobalsDialog, "IupGlobalsDialog")
		reg(&iupImageSave, "IupImageSave")
		reg(&iupImageSaveToBuffer, "IupImageSaveToBuffer")
		reg(&iupParam, "IupParam")
		reg(&iupParamBoxv, "IupParamBoxv")
		reg(&iupListDialog, "IupListDialog")
		reg(&iupGetParamv, "IupGetParamv")

		// free is used only to release IUP-allocated buffers (ImageSaveToBuffer).
		// It resolves via libc on unix/darwin; guard so a miss can't break init.
		func() {
			defer func() { recover() }()
			purego.RegisterLibFunc(&cfree, lib, "free")
		}()
	})
}
