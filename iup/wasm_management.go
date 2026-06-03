//go:build js && wasm

package iup

type ClassAttributeInfo struct {
	DefaultValue  string
	SystemDefault string
	Flags         int
}

type ClassConstructor struct {
	Format     string
	FormatAttr string
}

type ClassInfo struct {
	Parent        string
	NativeType    string
	ChildType     int
	IsInteractive bool
	HasAttribID   int
}

func ClassMatch(ih Ihandle, className string) bool {
	return ccall("IupClassMatch", "number", []interface{}{"number", "string"}, []interface{}{int(ih), className}).Int() != 0
}

func ConvertXYToPos(ih Ihandle, x, y int) int {
	return ccall("IupConvertXYToPos", "number", []interface{}{"number", "number", "number"}, []interface{}{int(ih), x, y}).Int()
}

func CopyClassAttributes(srcIh, dstIh Ihandle) {
	ccall("IupCopyClassAttributes", "", []interface{}{"number", "number"}, []interface{}{int(srcIh), int(dstIh)})
}

func Create(className string) Ihandle {
	return ccallHandle("IupCreate", []interface{}{"string"}, []interface{}{className})
}

func GetAllClasses() []string { return wasmNames("IupGetAllClasses", nil, nil) }

func GetAllFunctions() []string { return wasmNames("IupGetAllFunctions", nil, nil) }

func GetAllGlobals() []string { return wasmNames("IupGetAllGlobals", nil, nil) }

func GetClassAttributeInfo(className, name string) (info ClassAttributeInfo, ok bool) {
	pd, ps, pf := wasmMalloc(4), wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pd)
	defer wasmFree(ps)
	defer wasmFree(pf)
	rc := ccall("IupGetClassAttributeInfo", "number", []interface{}{"string", "string", "number", "number", "number"}, []interface{}{className, name, pd, ps, pf}).Int()
	if rc != 1 {
		return
	}
	info.DefaultValue = wasmGetStrAt(pd)
	info.SystemDefault = wasmGetStrAt(ps)
	info.Flags = wasmGetI32(pf)
	ok = true
	return
}

func GetClassAttributes(className string) []string {
	return wasmNames("IupGetClassAttributes", []interface{}{"string"}, []interface{}{className})
}

func GetClassCallbackFormat(className, name string) string {
	return ccall("IupGetClassCallbackFormat", "string", []interface{}{"string", "string"}, []interface{}{className, name}).String()
}

func GetClassCallbacks(className string) []string {
	return wasmNames("IupGetClassCallbacks", []interface{}{"string"}, []interface{}{className})
}

func GetClassConstructor(className string) (info ClassConstructor, ok bool) {
	pf, pa := wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pf)
	defer wasmFree(pa)
	rc := ccall("IupGetClassConstructor", "number", []interface{}{"string", "number", "number"}, []interface{}{className, pf, pa}).Int()
	if rc < 0 {
		return
	}
	info.Format = wasmGetStrAt(pf)
	info.FormatAttr = wasmGetStrAt(pa)
	ok = true
	return
}

func GetClassInfo(className string) (info ClassInfo, ok bool) {
	pp, pn := wasmMalloc(4), wasmMalloc(4)
	pc, pi, ph := wasmMalloc(4), wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pp)
	defer wasmFree(pn)
	defer wasmFree(pc)
	defer wasmFree(pi)
	defer wasmFree(ph)
	rc := ccall("IupGetClassInfo", "number", []interface{}{"string", "number", "number", "number", "number", "number"}, []interface{}{className, pp, pn, pc, pi, ph}).Int()
	if rc != 0 {
		return
	}
	info.Parent = wasmGetStrAt(pp)
	info.NativeType = wasmGetStrAt(pn)
	info.ChildType = wasmGetI32(pc)
	info.IsInteractive = wasmGetI32(pi) != 0
	info.HasAttribID = wasmGetI32(ph)
	ok = true
	return
}

func GetClassName(ih Ihandle) string {
	return ccall("IupGetClassName", "string", []interface{}{"number"}, []interface{}{int(ih)}).String()
}

func GetClassType(ih Ihandle) string {
	return ccall("IupGetClassType", "string", []interface{}{"number"}, []interface{}{int(ih)}).String()
}

func GetGlobalInfo(name string) (info GlobalInfo, ok bool) {
	pf, pd := wasmMalloc(4), wasmMalloc(4)
	defer wasmFree(pf)
	defer wasmFree(pd)
	rc := ccall("IupGetGlobalInfo", "number", []interface{}{"string", "number", "number"}, []interface{}{name, pf, pd}).Int()
	if rc != 1 {
		return
	}
	info.Flags = wasmGetI32(pf)
	info.Drivers = wasmGetI32(pd)
	ok = true
	return
}

type GlobalInfo struct {
	Flags   int
	Drivers int
}

func Map(ih Ihandle) int {
	return ccall("IupMap", "number", []interface{}{"number"}, []interface{}{int(ih)}).Int()
}

func Redraw(ih Ihandle, children int) {
	ccall("IupRedraw", "", []interface{}{"number", "number"}, []interface{}{int(ih), children})
}

func Refresh(ih Ihandle) {
	ccall("IupRefresh", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func RefreshChildren(ih Ihandle) {
	ccall("IupRefreshChildren", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func SaveClassAttributes(ih Ihandle) {
	ccall("IupSaveClassAttributes", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func SetClassDefaultAttribute(className, name, value string) {
	ccall("IupSetClassDefaultAttribute", "", []interface{}{"string", "string", "string"}, []interface{}{className, name, value})
}

func Unmap(ih Ihandle) {
	ccall("IupUnmap", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func Update(ih Ihandle) {
	ccall("IupUpdate", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func UpdateChildren(ih Ihandle) {
	ccall("IupUpdateChildren", "", []interface{}{"number"}, []interface{}{int(ih)})
}

func Version() string {
	return ccall("IupVersion", "string", nil, nil).String()
}

func VersionDate() string {
	return ccall("IupVersionDate", "string", nil, nil).String()
}

func VersionNumber() int {
	return ccall("IupVersionNumber", "number", nil, nil).Int()
}

func wasmNames(cfunc string, argTypes, args []interface{}) []string {
	n := ccall(cfunc, "number", append(argTypes, "number", "number"), append(args, 0, 0)).Int()
	if n <= 0 {
		return nil
	}
	arr := wasmMalloc(n * 4)
	defer wasmFree(arr)
	ccall(cfunc, "number", append(argTypes, "number", "number"), append(args, arr, n))
	out := make([]string, n)
	for i := 0; i < n; i++ {
		out[i] = wasmGetStrAt(arr + i*4)
	}
	return out
}
