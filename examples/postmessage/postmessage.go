package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"image"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"io/ioutil"
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

	vbox := iup.Vbox(labelImage, button).SetAttributes(map[string]string{
		"ALIGNMENT": "ACENTER",
		"GAP":       "10",
		"MARGIN":    "10x10",
	})

	dlg := iup.Dialog(vbox).SetAttribute("TITLE", "PostMessage")
	dlg.SetAttribute("RESIZE", "NO")
	dlg.SetHandle("dlg")

	labelImage.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(messageCb))
	button.SetCallback("ACTION", iup.ActionFunc(buttonCb))

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	buttonCb(button)

	iup.MainLoop()
}

func messageCb(ih iup.Ihandle, s string, i int, p any) int {
	b := p.([]byte)
	img, _, err := image.Decode(bytes.NewReader(b))
	if err != nil {
		log.Fatalln(err)
	}

	iup.ImageFromImage(img).SetHandle("xkcd")

	iup.GetHandle("label").SetAttribute("TIP", s)
	iup.GetHandle("label").SetAttribute("IMAGE", "xkcd")
	iup.GetHandle("dlg").SetAttribute("RASTERSIZE", fmt.Sprintf("%dx%d", img.Bounds().Dx()+20, img.Bounds().Dy()+80))

	iup.GetHandle("button").SetAttribute("ACTIVE", "YES")

	iup.Refresh(iup.GetHandle("dlg"))
	iup.ShowXY(iup.GetHandle("dlg"), iup.CENTER, iup.CENTER)

	return iup.DEFAULT
}

func buttonCb(ih iup.Ihandle) int {
	ih.SetAttribute("ACTIVE", "NO")

	type xkcd struct {
		Id    int64  `json:"num,omitempty"`
		Title string `json:"title,omitempty"`
		Alt   string `json:"alt,omitempty"`
		Img   string `json:"img,omitempty"`
	}

	go func() {
		random := rand.Intn(2800-1) + 1

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

		b, err := ioutil.ReadAll(img.Body)
		if err != nil {
			log.Println(err)
			return
		}

		iup.PostMessage(iup.GetHandle("label"), ret.Alt, 0, b)
	}()

	return iup.DEFAULT
}
