//go:build js && wasm

package iup

import "syscall/js"

func ExitLoop() {
	ccall("IupExitLoop", "", nil, nil)
}

func Flush() {
	ccall("IupFlush", "", nil, nil)
}

func GetCallback(ih Ihandle, name string) uintptr {
	return uintptr(ccall("IupGetCallback", "number", []interface{}{"number", "string"}, []interface{}{int(ih), name}).Int())
}

func GetFunction(name string) uintptr {
	return uintptr(ccall("IupGetFunction", "number", []interface{}{"string"}, []interface{}{name}).Int())
}

func LoopStep() (ret int) { return ccall("IupLoopStep", "number", nil, nil).Int() }

func LoopStepWait() (ret int) { return ccall("IupLoopStepWait", "number", nil, nil).Int() }

func MainLoopLevel() (ret int) { return ccall("IupMainLoopLevel", "number", nil, nil).Int() }

func PlayInput(fileName string) int {
	return ccall("IupPlayInput", "number", []interface{}{"string"}, []interface{}{fileName}).Int()
}

func PostMessage(ih Ihandle, s string, i int, p any) {
	var fn js.Func
	fn = js.FuncOf(func(this js.Value, args []js.Value) any {
		fn.Release()
		if cb, ok := callbacks[cbKey{ih, "POSTMESSAGE_CB"}].(PostMessageFunc); ok {
			cb(ih, s, i, p)
		}
		return nil
	})
	js.Global().Call("setTimeout", fn, 0)
}

func RecordInput(fileName string, mode int) int {
	return ccall("IupRecordInput", "number", []interface{}{"string", "number"}, []interface{}{fileName, mode}).Int()
}

func SetFunction(name string, fn interface{}) {
	if fn == nil {
		delete(globalFuncs, name)
		if name == "IDLE_ACTION" {
			ccall("iupwasmGoSetIdle", "", []interface{}{"number"}, []interface{}{0})
		}
		return
	}
	globalFuncs[name] = fn
	if name == "IDLE_ACTION" {
		ccall("iupwasmGoSetIdle", "", []interface{}{"number"}, []interface{}{1})
	}
}

var globalFuncs = map[string]interface{}{}
