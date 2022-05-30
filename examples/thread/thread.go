package main

import (
	"fmt"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	label := iup.Label("Start second thread:")
	button := iup.Button("Start")

	vbox := iup.Vbox(label, button).SetAttributes(map[string]string{
		"ALIGNMENT": "ACENTER",
		"GAP":       "10",
		"MARGIN":    "10x10",
	})

	dlg := iup.Dialog(vbox).SetAttribute("TITLE", "Thread")

	thread := iup.Thread()
	thread.SetCallback("THREAD_CB", iup.ThreadFunc(threadCb))
	thread.SetHandle("thread")

	button.SetCallback("ACTION", iup.ActionFunc(buttonCb))

	iup.Show(dlg)
	iup.MainLoop()
}

func buttonCb(ih iup.Ihandle) int {
	thread := iup.GetHandle("thread")
	thread.SetAttribute("START", "YES")
	fmt.Println("buttonCb", "ISCURRENT", thread.GetAttribute("ISCURRENT"))
	return iup.DEFAULT
}

func threadCb(ih iup.Ihandle) int {
	thread := iup.GetHandle("thread")
	fmt.Println("threadCb", "ISCURRENT", thread.GetAttribute("ISCURRENT"))
	time.Sleep(3 * time.Second)
	fmt.Println("threadCb", "EXIT")
	return iup.DEFAULT
}
