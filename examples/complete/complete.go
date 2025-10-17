package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"net/url"

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

	if iup.GetFocus() != list && (c == iup.K_DOWN || c == iup.K_UP) {
		iup.SetFocus(list)
		list.SetAttribute("VALUE", 1)
		return iup.IGNORE
	}

	if c == iup.K_ESC || c == iup.K_CR {
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
	list := iup.GetHandle("list")
	text := iup.GetHandle("text")

	if len(text.GetAttribute("VALUE")) < 3 {
		iup.Hide(iup.GetHandle("list"))
		return iup.DEFAULT
	}

	query := url.QueryEscape(text.GetAttribute("VALUE"))

	go func(h iup.Ihandle, q string) {
		client := &http.Client{}

		req, err := http.NewRequest("GET", "https://en.wikipedia.org/w/api.php?action=opensearch&format=json&search="+q, nil)
		if err != nil {
			fmt.Println(err)
			return
		}

		req.Header.Set("User-Agent", "Golang_Example/1.0")

		res, err := client.Do(req)
		if err != nil {
			fmt.Println(err)
			return
		}

		defer res.Body.Close()

		var ret []any
		err = json.NewDecoder(res.Body).Decode(&ret)
		if err != nil {
			log.Println(err)
			return
		}

		items := make([]string, 0)
		for n, item := range ret {
			if n != 1 {
				continue
			}
			switch it := item.(type) {
			case []interface{}:
				for _, val := range it {
					items = append(items, val.(string))
				}
			}
		}

		iup.PostMessage(h, "", 0, items)
	}(list, query)

	return iup.DEFAULT
}

func messageCb(ih iup.Ihandle, s string, i int, p any) int {
	text := iup.GetHandle("text")
	list := iup.GetHandle("list")

	results := p.([]string)

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
