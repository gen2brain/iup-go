package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"image"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"io"
	"log"
	"math/rand"
	"net/http"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	labelImage := iup.Label("").SetHandle("label").SetAttribute("IMAGE", "xkcd")
	button := iup.Button("Random XKCD").SetHandle("button")

	vbox := iup.Vbox(iup.Fill(), labelImage, button, iup.Fill()).SetAttributes(map[string]string{
		"ALIGNMENT": "ACENTER",
		"GAP":       "10",
		"MARGIN":    "10x10",
	})

	hbox := iup.Hbox(iup.Fill(), vbox, iup.Fill()).SetAttribute("ALIGNMENT", "ACENTER")

	dlg := iup.Dialog(hbox).SetAttribute("TITLE", "PostMessage").SetHandle("dlg")
	dlg.SetAttribute("DEFAULTENTER", "button")
	dlg.SetCallback("MAP_CB", iup.MapFunc(func(ih iup.Ihandle) int {
		buttonCb(button)
		return iup.DEFAULT
	}))

	button.SetCallback("ACTION", iup.ActionFunc(buttonCb))
	labelImage.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(messageCb))

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	iup.MainLoop()
}

func messageCb(ih iup.Ihandle, s string, i int, p any) int {
	defer iup.GetHandle("button").SetAttribute("ACTIVE", "YES")

	b := p.([]byte)
	img, _, err := image.Decode(bytes.NewReader(b))
	if err != nil {
		log.Println(err)
		return iup.DEFAULT
	}

	dlg := iup.GetHandle("dlg")
	dlg.SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", img.Bounds().Dx()+100, img.Bounds().Dy()+200))

	iup.Destroy(iup.GetHandle("xkcd"))
	iup.ImageFromImage(img).SetHandle("xkcd").SetAttribute("IMAGE", "xkcd")

	iup.GetHandle("label").SetAttribute("TIP", s)
	iup.GetHandle("label").SetAttribute("IMAGE", "xkcd")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	return iup.DEFAULT
}

func buttonCb(ih iup.Ihandle) int {
	ih.SetAttribute("ACTIVE", "NO")
	label := iup.GetHandle("label")

	type xkcd struct {
		Id    int64  `json:"num,omitempty"`
		Title string `json:"title,omitempty"`
		Alt   string `json:"alt,omitempty"`
		Img   string `json:"img,omitempty"`
	}

	go func() {
		random := rand.Intn(3150-1) + 1

		res, err := http.Get(fmt.Sprintf("https://xkcd.com/%d/info.0.json", random))
		if err != nil {
			log.Println(err)
			return
		}
		defer res.Body.Close()

		var ret xkcd
		err = json.NewDecoder(res.Body).Decode(&ret)
		if err != nil {
			log.Println(err)
			return
		}

		img, err := http.Get(ret.Img)
		if err != nil {
			log.Println(err)
			return
		}
		defer img.Body.Close()

		b, err := io.ReadAll(img.Body)
		if err != nil {
			log.Println(err)
			return
		}

		iup.PostMessage(label, ret.Alt, 0, b)
	}()

	return iup.DEFAULT
}
