//go:build !cgo && !js

package iup

func Show(ih Ihandle) int {
	return int(iupShow(uintptr(ih)))
}

func ShowXY(ih Ihandle, x, y int) int {
	return int(iupShowXY(uintptr(ih), int32(x), int32(y)))
}

func Map(ih Ihandle) int {
	return int(iupMap(uintptr(ih)))
}

func Refresh(ih Ihandle) {
	iupRefresh(uintptr(ih))
}

func Update(ih Ihandle) {
	iupUpdate(uintptr(ih))
}

func Destroy(ih Ihandle) {
	iupDestroy(uintptr(ih))
}

func SetHandle(name string, ih Ihandle) {
	iupSetHandle(name, uintptr(ih))
}

func GetHandle(name string) Ihandle {
	return mkih(iupGetHandle(name))
}

func MainLoop() int {
	return int(iupMainLoop())
}

func MainLoopLevel() int {
	return int(iupMainLoopLevel())
}

func LoopStep() int {
	return int(iupLoopStep())
}

func ExitLoop() {
	iupExitLoop()
}

func Flush() {
	iupFlush()
}

func RefreshChildren(ih Ihandle) {
	iupRefreshChildren(uintptr(ih))
}

func UpdateChildren(ih Ihandle) {
	iupUpdateChildren(uintptr(ih))
}

func Redraw(ih Ihandle, children int) {
	iupRedraw(uintptr(ih), int32(children))
}

func Unmap(ih Ihandle) {
	iupUnmap(uintptr(ih))
}

func ConvertXYToPos(ih Ihandle, x, y int) int {
	return int(iupConvertXYToPos(uintptr(ih), int32(x), int32(y)))
}

func GetAllClasses() (names []string) {
	n := int(iupGetAllClasses(nil, 0))
	if n > 0 {
		buf := make([]uintptr, n)
		iupGetAllClasses(&buf[0], int32(n))
		names = make([]string, n)
		for i := 0; i < n; i++ {
			names[i] = goString(buf[i])
		}
	}
	return
}

func GetAllGlobals() (names []string) {
	max := int(iupGetAllGlobals(nil, 0))
	if max <= 0 {
		return
	}
	buf := make([]uintptr, max)
	n := int(iupGetAllGlobals(&buf[0], int32(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = goString(buf[i])
	}
	return
}

func GetClassAttributes(className string) (names []string) {
	max := int(iupGetClassAttributes(className, nil, 0))
	if max <= 0 {
		return
	}
	buf := make([]uintptr, max)
	n := int(iupGetClassAttributes(className, &buf[0], int32(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = goString(buf[i])
	}
	return
}

func GetClassCallbacks(className string) (names []string) {
	max := int(iupGetClassCallbacks(className, nil, 0))
	if max <= 0 {
		return
	}
	buf := make([]uintptr, max)
	n := int(iupGetClassCallbacks(className, &buf[0], int32(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = goString(buf[i])
	}
	return
}

func GetClassType(ih Ihandle) string {
	return iupGetClassType(uintptr(ih))
}

func GetClassName(ih Ihandle) string {
	return iupGetClassName(uintptr(ih))
}

func LoopStepWait() int { return int(iupLoopStepWait()) }

func Execute(fileName, parameters string) int { return int(iupExecute(fileName, parameters)) }

func ExecuteWait(fileName, parameters string) int { return int(iupExecuteWait(fileName, parameters)) }

func Help(url string) int { return int(iupHelp(url)) }

func PlayInput(fileName string) int { return int(iupPlayInput(optCStr(fileName))) }

func RecordInput(fileName string, mode int) int {
	return int(iupRecordInput(optCStr(fileName), int32(mode)))
}

func ClassMatch(ih Ihandle, className string) bool { return iupClassMatch(uintptr(ih), className) != 0 }

func Create(className string) Ihandle { return mkih(iupCreate(className)) }

func CopyClassAttributes(srcIh, dstIh Ihandle) {
	iupCopyClassAttributes(uintptr(srcIh), uintptr(dstIh))
}

func SaveClassAttributes(ih Ihandle) { iupSaveClassAttributes(uintptr(ih)) }

func SetClassDefaultAttribute(className, name, value string) {
	iupSetClassDefaultAttribute(className, name, value)
}

func GetAllNames() (names []string) {
	n := int(iupGetAllNames(nil, 0))
	if n > 0 {
		buf := make([]uintptr, n)
		iupGetAllNames(&buf[0], int32(n))
		names = make([]string, n)
		for i := 0; i < n; i++ {
			names[i] = goString(buf[i])
		}
	}
	return
}

func GetAllDialogs() (names []string) {
	n := int(iupGetAllDialogs(nil, 0))
	if n > 0 {
		buf := make([]uintptr, n)
		iupGetAllDialogs(&buf[0], int32(n))
		names = make([]string, n)
		for i := 0; i < n; i++ {
			names[i] = goString(buf[i])
		}
	}
	return
}

func GetAllFunctions() (names []string) {
	max := int(iupGetAllFunctions(nil, 0))
	if max <= 0 {
		return
	}
	buf := make([]uintptr, max)
	n := int(iupGetAllFunctions(&buf[0], int32(max)))
	names = make([]string, n)
	for i := 0; i < n; i++ {
		names[i] = goString(buf[i])
	}
	return
}

type ClassInfo struct {
	Parent        string
	NativeType    string
	ChildType     int
	IsInteractive bool
	HasAttribID   int
}

func GetClassInfo(className string) (info ClassInfo, ok bool) {
	var parent, nativeType uintptr
	var childType, isInteractive, hasAttribID int32
	if iupGetClassInfo(className, &parent, &nativeType, &childType, &isInteractive, &hasAttribID) != 0 {
		return
	}
	info.Parent = goString(parent)
	info.NativeType = goString(nativeType)
	info.ChildType = int(childType)
	info.IsInteractive = isInteractive != 0
	info.HasAttribID = int(hasAttribID)
	ok = true
	return
}

type ClassConstructor struct {
	Format     string
	FormatAttr string
}

func GetClassConstructor(className string) (info ClassConstructor, ok bool) {
	var format, formatAttr uintptr
	if iupGetClassConstructor(className, &format, &formatAttr) < 0 {
		return
	}
	info.Format = goString(format)
	info.FormatAttr = goString(formatAttr)
	ok = true
	return
}

type ClassAttributeInfo struct {
	DefaultValue  string
	SystemDefault string
	Flags         int
}

func GetClassAttributeInfo(className, name string) (info ClassAttributeInfo, ok bool) {
	var def, sysDef uintptr
	var flags int32
	if iupGetClassAttributeInfo(className, name, &def, &sysDef, &flags) != 1 {
		return
	}
	info.DefaultValue = goString(def)
	info.SystemDefault = goString(sysDef)
	info.Flags = int(flags)
	ok = true
	return
}

func GetClassCallbackFormat(className, name string) string {
	return iupGetClassCallbackFormat(className, name)
}

type GlobalInfo struct {
	Flags   int
	Drivers int
}

func GetGlobalInfo(name string) (info GlobalInfo, ok bool) {
	var flags, drivers int32
	if iupGetGlobalInfo(name, &flags, &drivers) != 1 {
		return
	}
	info.Flags = int(flags)
	info.Drivers = int(drivers)
	ok = true
	return
}
