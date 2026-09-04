package main

import (
	"bytes"
	_ "embed"
	"fmt"
	"image/gif"
	"log"

	"github.com/gen2brain/iup-go/iup"
)

//go:embed loading.gif
var loadingGIF []byte

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	img, err := gif.DecodeAll(bytes.NewReader(loadingGIF))
	if err != nil {
		log.Fatalln(err)
	}

	animation := iup.User()
	animation.SetAttribute("FRAMETIME", "125")

	for idx, i := range img.Image {
		name := fmt.Sprintf("loading%d", idx)
		iup.ImageFromImage(i).SetHandle(name)
		iup.Append(animation, iup.GetHandle(name))
	}

	lbl := iup.AnimatedLabel(animation)
	lbl.SetAttributes("START=YES, STOPWHENHIDDEN=YES")

	info := iup.Label(fmt.Sprintf("FRAMECOUNT=%s FRAMETIME=%s RUNNING=%s", lbl.GetAttribute("FRAMECOUNT"), lbl.GetAttribute("FRAMETIME"), lbl.GetAttribute("RUNNING")))
	info.SetAttribute("EXPAND", "HORIZONTAL")
	hide := iup.Button("Hide for two seconds (STOPWHENHIDDEN)")
	hide.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		lbl.SetAttribute("VISIBLE", "NO")
		check := iup.Timer().SetAttributes("TIME=500, RUN=YES")
		check.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
			info.SetAttribute("TITLE", "hidden: RUNNING="+lbl.GetAttribute("RUNNING"))
			ih.Destroy()
			return iup.DEFAULT
		}))
		t := iup.Timer().SetAttributes("TIME=2000, RUN=YES")
		t.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
			lbl.SetAttributes("VISIBLE=YES, START=YES")
			info.SetAttribute("TITLE", "shown again: RUNNING="+lbl.GetAttribute("RUNNING"))
			ih.Destroy()
			return iup.DEFAULT
		}))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(lbl, info, hide).SetAttributes("NMARGIN=10x10, NGAP=5"))

	dlg.SetAttribute("TITLE", "Animated Label")

	iup.Show(dlg)
	iup.MainLoop()
}
