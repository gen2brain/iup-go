//go:build go1.27

package iup

// On sets the callback cb of ih to fn and returns ih, like the [On] function.
//
//	iup.Button("OK").On(iup.ActionCB, func(ih iup.Ihandle) int { return iup.CLOSE })
//
// https://gen2brain.github.io/iup-go/func/iup_setcallback.html
func (ih Ihandle) On[F any](cb Callback[F], fn F) Ihandle {
	return On(ih, cb, fn)
}
