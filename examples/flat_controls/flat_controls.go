//go:build ctrl

package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()
	iup.ControlsOpen()

	iup.SetHandle("dot_blue", makeImage(60, 140, 220, 16))
	iup.SetHandle("dot_red", makeImage(200, 60, 60, 16))
	iup.SetHandle("dot_gray", makeImage(170, 170, 170, 16))
	iup.SetHandle("check_off", makeImage(220, 220, 220, 14))
	iup.SetHandle("check_off_high", makeImage(240, 240, 240, 14))
	iup.SetHandle("check_on", makeImage(40, 160, 60, 14))
	iup.SetHandle("check_on_high", makeImage(90, 210, 110, 14))
	iup.SetHandle("check_on_press", makeImage(20, 110, 40, 14))
	iup.SetHandle("check_notdef", makeImage(230, 170, 40, 14))
	iup.SetHandle("check_inactive", makeImage(200, 200, 200, 14))
	iup.SetHandle("arrow_up", makeImage(60, 140, 220, 12))
	iup.SetHandle("arrow_down", makeImage(200, 60, 60, 12))
	iup.SetHandle("arrow_high", makeImage(120, 190, 255, 12))
	iup.SetHandle("arrow_press", makeImage(20, 70, 140, 12))

	statusText := iup.Text()
	statusText.SetAttributes(`READONLY=YES, EXPAND=HORIZONTAL, VALUE="Flat Controls"`)
	iup.SetHandle("statusText", statusText)

	flatTabs := createFlatTabs()
	flatTabs.SetAttribute("EXPAND", "YES")

	mainVbox := iup.Vbox(
		iup.FlatLabel("IUP Flat Controls").SetAttributes("FONTSIZE=16, FONTBOLD=YES, ALIGNMENT=ACENTER"),
		iup.Separator(),
		flatTabs,
		iup.Separator(),
		statusText,
	).SetAttributes("MARGIN=10x10, GAP=5")

	dlg := iup.Dialog(mainVbox)
	dlg.SetAttribute("TITLE", "Flat Controls")

	iup.Show(dlg)
	iup.MainLoop()
}

func createFlatTabs() iup.Ihandle {
	tab1 := createButtonsAndTogglesTab()
	tab1.SetAttribute("TABTITLE", "Buttons & Toggles")

	tab2 := createListAndTreeTab()
	tab2.SetAttribute("TABTITLE", "List & Tree")

	tab3 := createValAndOthersTab()
	tab3.SetAttribute("TABTITLE", "Val & Others")

	tab4 := createContainersTab()
	tab4.SetAttribute("TABTITLE", "Containers")

	return iup.FlatTabs(tab1, tab2, tab3, tab4)
}

func createButtonsAndTogglesTab() iup.Ihandle {
	flatBtn1 := iup.FlatButton("FlatButton 1")
	flatBtn1.SetAttributes("PADDING=10x5")
	iup.SetCallback(flatBtn1, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("FlatButton 1 clicked")
		return iup.DEFAULT
	}))

	flatBtn2 := iup.FlatButton("FlatButton 2")
	flatBtn2.SetAttributes(`PADDING=10x5, BGCOLOR="100 150 200"`)
	iup.SetCallback(flatBtn2, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("FlatButton 2 clicked (custom color)")
		return iup.DEFAULT
	}))

	flatBtn3 := iup.FlatButton("FlatButton 3")
	flatBtn3.SetAttributes(`PADDING=10x5, FGCOLOR="255 0 0"`)
	iup.SetCallback(flatBtn3, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("FlatButton 3 clicked (red text)")
		return iup.DEFAULT
	}))

	// Rounded corners examples
	flatBtnRounded1 := iup.FlatButton("Rounded 5px")
	flatBtnRounded1.SetAttributes(`PADDING=10x5, CORNERRADIUS=5, BGCOLOR="100 200 150"`)
	iup.SetCallback(flatBtnRounded1, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Rounded button (5px) clicked")
		return iup.DEFAULT
	}))

	flatBtnRounded2 := iup.FlatButton("Rounded 10px")
	flatBtnRounded2.SetAttributes(`PADDING=10x5, CORNERRADIUS=10, BGCOLOR="200 150 100"`)
	iup.SetCallback(flatBtnRounded2, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Rounded button (10px) clicked")
		return iup.DEFAULT
	}))

	flatBtnRounded3 := iup.FlatButton("Pill Button")
	flatBtnRounded3.SetAttributes(`PADDING=15x8, CORNERRADIUS=20, BGCOLOR="150 100 200"`)
	iup.SetCallback(flatBtnRounded3, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Pill button (20px radius) clicked")
		return iup.DEFAULT
	}))

	// Gradient examples
	flatBtnGrad1 := iup.FlatButton("Gradient V")
	flatBtnGrad1.SetAttributes(`PADDING=10x5, GRADIENT="100 180 255:50 100 200", FGCOLOR="255 255 255"`)
	iup.SetCallback(flatBtnGrad1, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Gradient button (vertical) clicked")
		return iup.DEFAULT
	}))

	flatBtnGrad2 := iup.FlatButton("Gradient H")
	flatBtnGrad2.SetAttributes(`PADDING=10x5, GRADIENT="255 150 100:200 80 50", GRADIENTANGLE=0, FGCOLOR="255 255 255"`)
	iup.SetCallback(flatBtnGrad2, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Gradient button (horizontal) clicked")
		return iup.DEFAULT
	}))

	flatBtnGrad3 := iup.FlatButton("Gradient 45°")
	flatBtnGrad3.SetAttributes(`PADDING=10x5, GRADIENT="150 255 150:50 180 50", GRADIENTANGLE=45, FGCOLOR="255 255 255"`)
	iup.SetCallback(flatBtnGrad3, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Gradient button (45°) clicked")
		return iup.DEFAULT
	}))

	// Combined: rounded + gradient
	flatBtnCombo1 := iup.FlatButton("Rounded + Gradient")
	flatBtnCombo1.SetAttributes(`PADDING=12x6, CORNERRADIUS=8, GRADIENT="80 150 255:40 80 180", FGCOLOR="255 255 255"`)
	iup.SetCallback(flatBtnCombo1, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Rounded gradient button clicked")
		return iup.DEFAULT
	}))

	flatBtnCombo2 := iup.FlatButton("Pill + Gradient")
	flatBtnCombo2.SetAttributes(`PADDING=15x8, CORNERRADIUS=20, GRADIENT="255 100 150:200 50 100", FGCOLOR="255 255 255"`)
	iup.SetCallback(flatBtnCombo2, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Pill gradient button clicked")
		return iup.DEFAULT
	}))

	// Gradient with hover/press states
	flatBtnStates := iup.FlatButton("Hover Me!")
	flatBtnStates.SetAttributes(`PADDING=12x6, CORNERRADIUS=8`)
	flatBtnStates.SetAttributes(`GRADIENT="100 150 200:70 120 170"`)
	flatBtnStates.SetAttributes(`GRADIENTHL="130 180 230:100 150 200"`)
	flatBtnStates.SetAttributes(`GRADIENTPS="70 120 170:40 90 140"`)
	flatBtnStates.SetAttributes(`FGCOLOR="255 255 255"`)
	iup.SetCallback(flatBtnStates, "FLAT_ACTION", iup.FlatActionFunc(func(ih iup.Ihandle) int {
		updateStatus("Button with gradient states clicked")
		return iup.DEFAULT
	}))

	flatToggle1 := iup.FlatToggle("FlatToggle 1")
	iup.SetCallback(flatToggle1, "FLAT_ACTION", iup.FlatToggleActionFunc(func(ih iup.Ihandle, state int) int {
		updateStatus(fmt.Sprintf("FlatToggle 1 state: %d", state))
		return iup.DEFAULT
	}))

	flatToggle2 := iup.FlatToggle("FlatToggle 2")
	flatToggle2.SetAttribute("VALUE", "ON")
	iup.SetCallback(flatToggle2, "FLAT_ACTION", iup.FlatToggleActionFunc(func(ih iup.Ihandle, state int) int {
		updateStatus(fmt.Sprintf("FlatToggle 2 state: %d", state))
		return iup.DEFAULT
	}))

	flatToggle3 := iup.FlatToggle("FlatToggle 3")
	flatToggle3.SetAttributes("3STATE=NO")
	iup.SetCallback(flatToggle3, "FLAT_ACTION", iup.FlatToggleActionFunc(func(ih iup.Ihandle, state int) int {
		updateStatus(fmt.Sprintf("FlatToggle 3 state: %d", state))
		return iup.DEFAULT
	}))

	checkColors := iup.FlatToggle("Check colors")
	checkColors.SetAttributes(`CHECKSIZE=16, CHECKSPACING=10, CHECKBGCOLOR="255 250 220", CHECKFGCOLOR="40 120 210", CHECKHLCOLOR="200 40 40", CHECKPSCOLOR="0 140 60", VALUE=ON`)
	iup.SetCallback(checkColors, "FLAT_ACTION", iup.FlatToggleActionFunc(func(ih iup.Ihandle, state int) int {
		updateStatus(fmt.Sprintf("Check colors state: %d", state))
		return iup.DEFAULT
	}))

	checkImages := iup.FlatToggle("Check images (3STATE)")
	checkImages.SetAttributes("CHECKSIZE=16, 3STATE=YES, CHECKIMAGE=check_off, CHECKIMAGEHIGHLIGHT=check_off_high, CHECKIMAGEPRESS=check_on_press, CHECKIMAGEINACTIVE=check_inactive")
	checkImages.SetAttributes("CHECKIMAGEON=check_on, CHECKIMAGEONHIGHLIGHT=check_on_high, CHECKIMAGEONPRESS=check_on_press, CHECKIMAGEONINACTIVE=check_inactive")
	checkImages.SetAttributes("CHECKIMAGENOTDEF=check_notdef, CHECKIMAGENOTDEFHIGHLIGHT=check_notdef, CHECKIMAGENOTDEFPRESS=check_on_press, CHECKIMAGENOTDEFINACTIVE=check_inactive")
	iup.SetCallback(checkImages, "FLAT_ACTION", iup.FlatToggleActionFunc(func(ih iup.Ihandle, state int) int {
		updateStatus(fmt.Sprintf("Check images state: %d", state))
		return iup.DEFAULT
	}))

	radioToggle := func(title string) iup.Ihandle {
		t := iup.FlatToggle(title)
		t.SetAttributes("SELECTEDNOTIFY=YES, CHECKSIZE=16")
		iup.SetCallback(t, "FLAT_ACTION", iup.FlatToggleActionFunc(func(ih iup.Ihandle, state int) int {
			updateStatus(fmt.Sprintf("Radio %s state: %d", title, state))
			return iup.DEFAULT
		}))
		return t
	}
	radioA, radioB, radioC := radioToggle("Radio A"), radioToggle("Radio B"), radioToggle("Not in the radio")
	radioC.SetAttribute("IGNORERADIO", "YES")
	radioA.SetAttribute("VALUE", "ON")
	radio := iup.Radio(iup.Hbox(radioA, radioB, radioC).SetAttribute("GAP", "10"))

	activeToggle := iup.Toggle("Toggles active")
	activeToggle.SetAttribute("VALUE", "ON")
	iup.SetCallback(activeToggle, "ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, state int) int {
		value := "NO"
		if state != 0 {
			value = "YES"
		}
		checkColors.SetAttribute("ACTIVE", value)
		checkImages.SetAttribute("ACTIVE", value)
		return iup.DEFAULT
	}))

	vbox := iup.Vbox(
		iup.FlatLabel("FlatButton Examples:").SetAttributes("FONTBOLD=YES"),
		iup.Hbox(flatBtn1, flatBtn2, flatBtn3).SetAttributes("GAP=10"),
		iup.Fill(),
		iup.FlatLabel("Rounded Corners:").SetAttributes("FONTBOLD=YES"),
		iup.Hbox(flatBtnRounded1, flatBtnRounded2, flatBtnRounded3).SetAttributes("GAP=10"),
		iup.Fill(),
		iup.FlatLabel("Gradient Backgrounds:").SetAttributes("FONTBOLD=YES"),
		iup.Hbox(flatBtnGrad1, flatBtnGrad2, flatBtnGrad3).SetAttributes("GAP=10"),
		iup.Fill(),
		iup.FlatLabel("Combined (Rounded + Gradient):").SetAttributes("FONTBOLD=YES"),
		iup.Hbox(flatBtnCombo1, flatBtnCombo2, flatBtnStates).SetAttributes("GAP=10"),
		iup.Fill(),
		iup.Separator(),
		iup.Fill(),
		iup.FlatLabel("FlatToggle Examples:").SetAttributes("FONTBOLD=YES"),
		flatToggle1,
		flatToggle2,
		flatToggle3,
		iup.Hbox(checkColors, checkImages, activeToggle).SetAttributes("GAP=10"),
		radio,
		iup.Fill(),
	).SetAttributes("MARGIN=10x10, GAP=5")

	return vbox
}

func createListAndTreeTab() iup.Ihandle {
	flatList := iup.FlatList()
	flatList.SetAttributes(`1="Item 1", 2="Item 2", 3="Item 3", 4="Item 4", 5="Item 5"`)
	flatList.SetAttributes("EXPAND=HORIZONTAL, VISIBLELINES=5, IMAGEPOSITION=RIGHT, ICONSPACING=8")
	flatList.SetAttributes(`IMAGE1=dot_blue, IMAGE3=dot_red, ITEMFONTSTYLE1=Bold, ITEMFONTSIZE2=14, ITEMTIP1="First item", ITEMTIP2="Bigger font", ITEMTIP3="Red dot"`)
	iup.SetCallback(flatList, "FLAT_ACTION", iup.FlatListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		updateStatus(fmt.Sprintf("FlatList: Item %d selected (%s), state=%d", item, text, state))
		return iup.DEFAULT
	}))

	flatTree := iup.FlatTree()

	flatTree.SetAttributes("EXPAND=YES, SHOWTOGGLE=YES, EMPTYTOGGLE=YES, ICONSPACING=4, EXTRATEXTWIDTH=80")
	flatTree.SetAttributes(`TOGGLEBGCOLOR="255 250 220", TOGGLEFGCOLOR="40 120 210", TOGGLESIZE=14, LINECOLOR="200 60 60"`)
	flatTree.SetAttributes(`BUTTONBGCOLOR="225 240 255", BUTTONFGCOLOR="0 60 120", BUTTONBRDCOLOR="60 140 220", BUTTONSIZE=12`)
	iup.SetAttributeId(flatTree, "ADDBRANCH", -1, "Figures")
	iup.SetAttributeId(flatTree, "ADDLEAF", 0, "Other")
	iup.SetAttributeId(flatTree, "ADDBRANCH", 0, "Triangle")
	iup.SetAttributeId(flatTree, "ADDLEAF", 2, "Equilateral")
	iup.SetAttributeId(flatTree, "ADDLEAF", 2, "Isosceles")
	iup.SetAttributeId(flatTree, "ADDLEAF", 2, "Scalene")
	iup.SetAttributeId(flatTree, "ADDBRANCH", 0, "Parallelogram")
	iup.SetAttributeId(flatTree, "ADDLEAF", 6, "Square")
	iup.SetAttributeId(flatTree, "ADDLEAF", 6, "Diamond")
	for id, text := range map[int]string{0: "root", 1: "leaf", 2: "3 sides", 6: "4 sides"} {
		iup.SetAttributeId(flatTree, "EXTRATEXT", id, text)
	}
	iup.SetAttributeId(flatTree, "TOGGLEVALUE", 3, "ON")
	iup.SetAttributeId(flatTree, "TOGGLEVISIBLE", 1, "NO")

	iup.SetCallback(flatTree, "TOGGLEVALUE_CB", iup.ToggleValueFunc(func(ih iup.Ihandle, id, state int) int {
		updateStatus(fmt.Sprintf("FlatTree: TOGGLEVALUE_CB node %d state=%d", id, state))
		return iup.DEFAULT
	}))

	iup.SetCallback(flatTree, "SELECTION_CB", iup.SelectionFunc(func(ih iup.Ihandle, id, status int) int {
		title := iup.GetAttributeId(ih, "TITLE", id)
		updateStatus(fmt.Sprintf("FlatTree: Selected node %d (%s), status=%d", id, title, status))
		return iup.DEFAULT
	}))

	vbox := iup.Vbox(
		iup.FlatLabel("FlatList Example:").SetAttributes("FONTBOLD=YES"),
		flatList,
		iup.Fill(),
		iup.Separator(),
		iup.Fill(),
		iup.FlatLabel("FlatTree Example:").SetAttributes("FONTBOLD=YES"),
		flatTree,
	).SetAttributes("MARGIN=10x10, GAP=5")

	return vbox
}

func createValAndOthersTab() iup.Ihandle {
	valLabel := iup.FlatLabel("Value: 0")
	iup.SetHandle("valLabel", valLabel)

	flatValH := iup.FlatVal("HORIZONTAL")
	flatValH.SetAttributes("EXPAND=HORIZONTAL, MIN=0, MAX=100, HANDLERSIZE=24, SLIDERSIZE=8")
	flatValH.SetAttributes(`SLIDERBORDERCOLOR="40 120 210", BORDERHLCOLOR="200 60 60", BORDERPSCOLOR="140 20 20", FOCUSFEEDBACK=NO`)
	iup.SetCallback(flatValH, "VALUECHANGING_CB", iup.ValueChangingFunc(func(ih iup.Ihandle, start int) int {
		updateStatus(fmt.Sprintf("Horizontal FlatVal VALUECHANGING_CB start=%d", start))
		return iup.DEFAULT
	}))
	iup.SetCallback(flatValH, "FLAT_WHEEL_CB", iup.WheelFunc(func(ih iup.Ihandle, delta float64, x, y int, st string) int {
		updateStatus(fmt.Sprintf("Horizontal FlatVal FLAT_WHEEL_CB delta=%.0f at %d,%d", delta, x, y))
		return iup.DEFAULT
	}))
	iup.SetCallback(flatValH, "FLAT_BUTTON_CB", iup.ButtonFunc(func(ih iup.Ihandle, button, pressed, x, y int, st string) int {
		fmt.Printf("Horizontal FlatVal FLAT_BUTTON_CB button=%d pressed=%d at %d,%d\n", button, pressed, x, y)
		return iup.DEFAULT
	}))
	iup.SetCallback(flatValH, "VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		value := ih.GetAttribute("VALUE")
		iup.GetHandle("valLabel").SetAttribute("VALUE", "Horizontal Value: "+value)
		updateStatus("Horizontal FlatVal value: " + value)
		return iup.DEFAULT
	}))

	flatValV := iup.FlatVal("VERTICAL")
	flatValV.SetAttributes("EXPAND=VERTICAL, MIN=0, MAX=100, HANDLERSIZE=30, SLIDERSIZE=12, IMAGE=dot_blue, IMAGEHIGHLIGHT=dot_red, IMAGEPRESS=dot_gray, IMAGEINACTIVE=dot_gray")

	flatLabel1 := iup.FlatLabel("This is a FlatLabel")
	flatLabel1.SetAttributes(`BGCOLOR="220 220 220", PADDING=10x10`)

	flatLabel2 := iup.FlatLabel("FlatLabel with custom styling")
	flatLabel2.SetAttributes(`BGCOLOR="100 180 255", FGCOLOR="255 255 255", PADDING=10x10, ALIGNMENT=ACENTER`)

	flatLabel3 := iup.FlatLabel("Right aligned\nmulti-line label\nwith an image")
	flatLabel3.SetAttributes("IMAGE=dot_blue, IMAGEINACTIVE=dot_gray, IMAGEPOSITION=RIGHT, TEXTALIGNMENT=ARIGHT, CPADDING=2x1, CSPACING=2")

	flatLabel4 := iup.FlatLabel("Front image over a back image")
	flatLabel4.SetAttributes("BACKIMAGE=dot_red, BACKIMAGEZOOM=YES, FRONTIMAGE=dot_blue, FRONTIMAGEINACTIVE=dot_gray, PADDING=10x10")

	flatLabel5 := iup.FlatLabel("Rotated label")
	flatLabel5.SetAttributes(`TEXTORIENTATION=90, BGCOLOR="220 220 220", PADDING=4x8`)

	separator1 := iup.Separator()

	vbox := iup.Vbox(
		iup.FlatLabel("FlatVal Horizontal Example:").SetAttributes("FONTBOLD=YES"),
		flatValH,
		valLabel,
		iup.Fill(),
		separator1,
		iup.Fill(),
		iup.FlatLabel("FlatLabel Examples:").SetAttributes("FONTBOLD=YES"),
		flatLabel1,
		flatLabel2,
		iup.Hbox(flatLabel3, flatLabel4, flatLabel5).SetAttributes("GAP=10, ALIGNMENT=ACENTER"),
		iup.Fill(),
	).SetAttributes("MARGIN=10x10, GAP=5")

	hbox := iup.Hbox(
		vbox,
		iup.Vbox(
			iup.FlatLabel("FlatVal Vertical:").SetAttributes("FONTBOLD=YES"),
			flatValV,
		).SetAttributes("MARGIN=10x10, GAP=5"),
	).SetAttributes("GAP=10")

	return hbox
}

func createContainersTab() iup.Ihandle {
	innerContent := iup.Vbox(
		iup.FlatLabel("Content inside FlatFrame"),
		iup.FlatButton("Button 1"),
		iup.FlatButton("Button 2"),
		iup.FlatToggle("Toggle inside frame"),
	).SetAttributes("MARGIN=10x10, GAP=5")

	flatFrame := iup.FlatFrame(innerContent)
	flatFrame.SetAttributes(`TITLE="FlatFrame Example", EXPAND=HORIZONTAL, FRAMEWIDTH=2, FRAMESPACE=6, TITLELINE=YES, TITLELINEWIDTH=2, TITLEPADDING=6x3`)
	flatFrame.SetAttributes(`FRAMECOLOR="40 120 210", TITLECOLOR="0 60 120", TITLEBGCOLOR="225 240 255", TITLELINECOLOR="40 120 210", TITLEALIGNMENT=ACENTER`)
	flatFrame.SetAttributes("TITLEIMAGE=dot_blue, TITLEIMAGEINACTIVE=dot_gray, TITLEIMAGEPOSITION=RIGHT, TITLEIMAGESPACING=8")

	scrollContent := iup.Vbox(
		iup.FlatLabel("This content is inside a FlatScrollBox."),
		iup.FlatLabel("Add more content to see scrollbars..."),
		iup.FlatButton("Button 1"),
		iup.FlatButton("Button 2"),
		iup.FlatButton("Button 3"),
		iup.FlatButton("Button 4"),
		iup.FlatButton("Button 5"),
		iup.FlatLabel("More labels..."),
		iup.FlatLabel("Even more labels..."),
		iup.FlatButton("Button 6"),
		iup.FlatButton("Button 7"),
		iup.FlatLabel("Last label in scroll area"),
	).SetAttributes("MARGIN=5x5, GAP=5")

	flatScrollBox := iup.FlatScrollBox(scrollContent)
	flatScrollBox.SetAttributes("EXPAND=YES, LAYOUTDRAG=NO, SHOWARROWS=YES, ARROWIMAGES=YES")
	flatScrollBox.SetAttributes(`SB_BACKCOLOR="240 240 240", SB_FORECOLOR="60 140 220", SB_HIGHCOLOR="120 190 255", SB_PRESSCOLOR="20 70 140"`)
	flatScrollBox.SetAttributes("SB_IMAGETOP=arrow_up, SB_IMAGEBOTTOM=arrow_down, SB_IMAGETOPHIGHLIGHT=arrow_high, SB_IMAGEBOTTOMHIGHLIGHT=arrow_high")
	flatScrollBox.SetAttributes("SB_IMAGETOPPRESS=arrow_press, SB_IMAGEBOTTOMPRESS=arrow_press, SB_IMAGETOPINACTIVE=dot_gray, SB_IMAGEBOTTOMINACTIVE=dot_gray")

	split := iup.Split(
		iup.Vbox(
			iup.FlatLabel("FlatFrame Container:").SetAttributes("FONTBOLD=YES"),
			flatFrame,
		).SetAttributes("GAP=5"),
		iup.Vbox(
			iup.FlatLabel("FlatScrollBox Container (drag the bar down to scroll):").SetAttributes("FONTBOLD=YES"),
			flatScrollBox,
		).SetAttributes("GAP=5"),
	).SetAttributes("ORIENTATION=HORIZONTAL")

	vbox := iup.Vbox(split).SetAttributes("MARGIN=10x10, GAP=5")

	return vbox
}

func updateStatus(message string) {
	statusText := iup.GetHandle("statusText")
	if statusText != 0 {
		statusText.SetAttribute("VALUE", message)
	}
	fmt.Println(message)
}

func makeImage(r, g, b byte, n int) iup.Ihandle {
	pixels := make([]byte, n*n*4)
	c := float64(n) / 2
	for y := 0; y < n; y++ {
		for x := 0; x < n; x++ {
			i := (y*n + x) * 4
			dx, dy := float64(x)-c+0.5, float64(y)-c+0.5
			if dx*dx+dy*dy <= (c-1)*(c-1) {
				pixels[i+0], pixels[i+1], pixels[i+2], pixels[i+3] = r, g, b, 255
			}
		}
	}
	return iup.ImageRGBA(n, n, pixels)
}
