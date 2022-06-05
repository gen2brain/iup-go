package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"runtime/cgo"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	text := iup.Text().SetHandle("text")
	text.SetAttributes(`SIZE=200x`)
	text.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(textValueChangedCb))

	list := iup.List().SetHandle("list")
	list.SetAttributes("VISIBLE=NO, FLOATING=YES, ZORDER=TOP")
	list.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(messageCb))
	list.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(listValueChangedCb))

	dlg := iup.Dialog(iup.Vbox(text, list).SetAttributes("GAP=5"))
	dlg.SetHandle("dlg")
	dlg.SetAttributes(`TITLE="Complete", SIZE=xQUARTER`)
	dlg.SetCallback("K_ANY", iup.KAnyFunc(kAnyCb))

	iup.Show(dlg)
	iup.MainLoop()
}

func kAnyCb(ih iup.Ihandle, c int) int {
	list := iup.GetHandle("list")
	text := iup.GetHandle("text")

	if c == iup.K_DOWN || c == iup.K_UP {
		iup.SetFocus(list)
		list.SetAttribute("VALUE", 1)
	} else if c == iup.K_ESC || c == iup.K_CR {
		iup.Hide(list)
		iup.SetFocus(text)
		text.SetAttribute("SELECTION", "NONE")
		text.SetAttribute("SCROLLTOPOS", text.GetInt("COUNT"))
	}
	return iup.DEFAULT
}

func listValueChangedCb(ih iup.Ihandle) int {
	val := ih.GetAttribute("VALUESTRING")
	iup.GetHandle("text").SetAttribute("VALUE", val)
	return iup.DEFAULT
}

func textValueChangedCb(ih iup.Ihandle) int {
	text := iup.GetHandle("text")
	if len(text.GetAttribute("VALUE")) < 3 {
		iup.Hide(iup.GetHandle("list"))
		return iup.DEFAULT
	}

	go func() {
		resp, err := http.Get("https://en.wikipedia.org/w/api.php?action=opensearch&search=" + url.QueryEscape(text.GetAttribute("VALUE")))
		if err != nil {
			fmt.Println(err)
			return
		}

		var results [][]string
		json.NewDecoder(resp.Body).Decode(&results)

		h := cgo.NewHandle(results[1])
		iup.PostMessage(iup.GetHandle("list"), "", 0, 1.0, h)
	}()

	return iup.DEFAULT
}

func messageCb(ih iup.Ihandle, s string, i int, f float64, p *cgo.Handle) int {
	text := iup.GetHandle("text")
	list := iup.GetHandle("list")

	results := p.Value().([]string)
	defer p.Delete()

	if len(results) == 0 {
		iup.Hide(list)
		return iup.DEFAULT
	}

	iup.SetAttributeId(list, "", 1, nil) // remove all items
	for idx, res := range results {
		iup.SetAttributeId(list, "", idx+1, res)
	}

	var w, h int
	fmt.Sscanf(text.GetAttribute("RASTERSIZE"), "%dx%d", &w, &h)

	list.SetAttribute("VALUE", 0)
	list.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", w, h*5))
	list.SetAttribute("POSITION", fmt.Sprintf(",%d", h))

	iup.Refresh(list)
	iup.Show(list)

	return iup.DEFAULT
}
