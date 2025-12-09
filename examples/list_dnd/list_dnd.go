package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	list1 := iup.List()
	list1.SetAttributes("1=Gold, 2=Silver, 3=Bronze, 4=Latão, 5=None," +
		"SHOWIMAGE=YES, DRAGDROPLIST=YES, XXX_SPACING=4, VALUE=4")

	loadMedalImages()

	list1.SetAttribute("IMAGE1", "IMGGOLD")
	list1.SetAttribute("IMAGE2", "IMGSILVER")
	list1.SetAttribute("IMAGE3", "IMGBRONZE")
	list1.SetAttribute("DRAGSOURCE", "YES")
	list1.SetAttribute("DRAGTYPES", "ITEMLIST")

	frmMedal1 := iup.Frame(list1)
	frmMedal1.SetAttribute("TITLE", "List 1")

	list2 := iup.List()
	list2.SetAttributes("1=Açaí, 2=Cajá, 3=Pêssego, 4=Limão, 5=Morango, 6=Coco," +
		"SHOWIMAGE=YES, DRAGDROPLIST=YES, XXX_SPACING=4, VALUE=4")
	list2.SetAttribute("DROPTARGET", "YES")
	list2.SetAttribute("DROPTYPES", "ITEMLIST")

	frmMedal2 := iup.Frame(list2)
	frmMedal2.SetAttribute("TITLE", "List 2")

	dlg := iup.Dialog(iup.Hbox(frmMedal1, frmMedal2))
	dlg.SetAttribute("TITLE", "List DND")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	iup.MainLoop()
}

func loadMedalImages() {
	imgGold := []byte{
		0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0,
		0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
		0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
		0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0,
		0, 3, 4, 4, 2, 4, 4, 4, 4, 4, 2, 2, 4, 4, 3, 0,
		2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2,
		0, 0, 3, 3, 2, 2, 2, 1, 1, 2, 2, 2, 1, 3, 3, 0,
		0, 1, 1, 1, 3, 2, 1, 1, 1, 1, 2, 3, 3, 3, 3, 0,
		3, 3, 1, 1, 1, 3, 3, 3, 1, 3, 3, 1, 1, 1, 1, 1,
		3, 3, 1, 1, 1, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 3, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 3, 3, 0, 0, 1, 1, 3, 0, 0, 0, 0,
	}

	imgSilver := []byte{
		0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0,
		0, 0, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 0, 0,
		0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0,
		0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 0,
		3, 0, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 2, 2, 0, 3,
		0, 0, 1, 1, 2, 2, 1, 1, 1, 2, 2, 2, 1, 1, 3, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 2, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 2, 0, 0, 0, 0,
	}

	imgBronze := []byte{
		0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
		0, 0, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 3, 3, 1, 1, 1, 1, 3, 3, 1, 1, 1, 0,
		4, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 4,
		0, 0, 3, 3, 3, 3, 2, 2, 2, 2, 3, 2, 2, 3, 4, 0,
		0, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 3, 0,
		4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
		4, 3, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 3,
		0, 0, 0, 0, 3, 2, 2, 0, 0, 2, 2, 2, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 3, 3, 0, 0, 2, 2, 4, 0, 0, 0, 0,
	}

	imageGold := iup.Image(16, 16, imgGold)
	imageGold.SetAttribute("0", "BGCOLOR")
	imageGold.SetAttribute("1", "128 0 0")
	imageGold.SetAttribute("2", "128 128 0")
	imageGold.SetAttribute("3", "255 0 0")
	imageGold.SetAttribute("4", "255 255 0")
	iup.SetHandle("IMGGOLD", imageGold)

	imageSilver := iup.Image(16, 16, imgSilver)
	imageSilver.SetAttribute("0", "BGCOLOR")
	imageSilver.SetAttribute("1", "0 128 128")
	imageSilver.SetAttribute("2", "128 128 128")
	imageSilver.SetAttribute("3", "192 192 192")
	imageSilver.SetAttribute("4", "255 255 255")
	iup.SetHandle("IMGSILVER", imageSilver)

	imageBronze := iup.Image(16, 16, imgBronze)
	imageBronze.SetAttribute("0", "BGCOLOR")
	imageBronze.SetAttribute("1", "128 0 0")
	imageBronze.SetAttribute("2", "0 128 0")
	imageBronze.SetAttribute("3", "128 128 0")
	imageBronze.SetAttribute("4", "128 128 128")
	iup.SetHandle("IMGBRONZE", imageBronze)
}
