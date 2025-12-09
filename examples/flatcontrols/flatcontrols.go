package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	statusText := iup.Text()
	statusText.SetAttributes("READONLY=YES, EXPAND=HORIZONTAL, VALUE=Flat Controls")
	iup.SetHandle("statusText", statusText)

	flatTabs := createFlatTabs()
	flatTabs.SetAttribute("EXPAND", "YES")

	mainVbox := iup.Vbox(
		iup.FlatLabel("IUP Flat Controls").SetAttributes("FONTSIZE=16, FONTBOLD=YES, ALIGNMENT=ACENTER"),
		iup.FlatSeparator(),
		flatTabs,
		iup.FlatSeparator(),
		statusText,
	).SetAttributes("MARGIN=10x10, GAP=5")

	dlg := iup.Dialog(mainVbox)
	dlg.SetAttributes(`TITLE="Flat Controls", RASTERSIZE=x600`)

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

	vbox := iup.Vbox(
		iup.FlatLabel("FlatButton Examples:").SetAttributes("FONTBOLD=YES"),
		iup.Hbox(flatBtn1, flatBtn2, flatBtn3).SetAttributes("GAP=10"),
		iup.Fill(),
		iup.FlatSeparator(),
		iup.Fill(),
		iup.FlatLabel("FlatToggle Examples:").SetAttributes("FONTBOLD=YES"),
		flatToggle1,
		flatToggle2,
		flatToggle3,
		iup.Fill(),
	).SetAttributes("MARGIN=10x10, GAP=5")

	return vbox
}

func createListAndTreeTab() iup.Ihandle {
	flatList := iup.FlatList()
	flatList.SetAttributes(`1="Item 1", 2="Item 2", 3="Item 3", 4="Item 4", 5="Item 5"`)
	flatList.SetAttributes("EXPAND=HORIZONTAL, VISIBLELINES=5")
	iup.SetCallback(flatList, "FLAT_ACTION", iup.FlatListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		updateStatus(fmt.Sprintf("FlatList: Item %d selected (%s), state=%d", item, text, state))
		return iup.DEFAULT
	}))

	flatTree := iup.FlatTree()

	flatTree.SetAttributes("EXPAND=YES")
	iup.SetAttributeId(flatTree, "ADDBRANCH", -1, "Figures")
	iup.SetAttributeId(flatTree, "ADDLEAF", 0, "Other")
	iup.SetAttributeId(flatTree, "ADDBRANCH", 0, "Triangle")
	iup.SetAttributeId(flatTree, "ADDLEAF", 2, "Equilateral")
	iup.SetAttributeId(flatTree, "ADDLEAF", 2, "Isosceles")
	iup.SetAttributeId(flatTree, "ADDLEAF", 2, "Scalene")
	iup.SetAttributeId(flatTree, "ADDBRANCH", 0, "Parallelogram")
	iup.SetAttributeId(flatTree, "ADDLEAF", 6, "Square")
	iup.SetAttributeId(flatTree, "ADDLEAF", 6, "Diamond")

	iup.SetCallback(flatTree, "SELECTION_CB", iup.SelectionFunc(func(ih iup.Ihandle, id, status int) int {
		title := iup.GetAttributeId(ih, "TITLE", id)
		updateStatus(fmt.Sprintf("FlatTree: Selected node %d (%s), status=%d", id, title, status))
		return iup.DEFAULT
	}))

	vbox := iup.Vbox(
		iup.FlatLabel("FlatList Example:").SetAttributes("FONTBOLD=YES"),
		flatList,
		iup.Fill(),
		iup.FlatSeparator(),
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
	flatValH.SetAttributes("EXPAND=HORIZONTAL, MIN=0, MAX=100")
	iup.SetCallback(flatValH, "VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
		value := ih.GetAttribute("VALUE")
		iup.GetHandle("valLabel").SetAttribute("VALUE", "Horizontal Value: "+value)
		updateStatus("Horizontal FlatVal value: " + value)
		return iup.DEFAULT
	}))

	flatValV := iup.FlatVal("VERTICAL")
	flatValV.SetAttributes("EXPAND=VERTICAL, MIN=0, MAX=100, RASTERSIZE=50x")

	flatLabel1 := iup.FlatLabel("This is a FlatLabel")
	flatLabel1.SetAttributes(`BGCOLOR="220 220 220", PADDING=10x10`)

	flatLabel2 := iup.FlatLabel("FlatLabel with custom styling")
	flatLabel2.SetAttributes(`BGCOLOR="100 180 255", FGCOLOR="255 255 255", PADDING=10x10, ALIGNMENT=ACENTER`)

	separator1 := iup.FlatSeparator()

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
	flatFrame.SetAttributes("TITLE=FlatFrame Example, EXPAND=HORIZONTAL")

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
	flatScrollBox.SetAttributes("RASTERSIZE=300x200, EXPAND=YES")

	vbox := iup.Vbox(
		iup.FlatLabel("FlatFrame Container:").SetAttributes("FONTBOLD=YES"),
		flatFrame,
		iup.Fill(),
		iup.FlatSeparator(),
		iup.Fill(),
		iup.FlatLabel("FlatScrollBox Container:").SetAttributes("FONTBOLD=YES"),
		flatScrollBox,
	).SetAttributes("MARGIN=10x10, GAP=5")

	return vbox
}

func updateStatus(message string) {
	statusText := iup.GetHandle("statusText")
	if statusText != 0 {
		statusText.SetAttribute("VALUE", message)
	}
	fmt.Println(message)
}
