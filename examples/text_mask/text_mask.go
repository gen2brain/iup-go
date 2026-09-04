package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var status iup.Ihandle

func report(msg string) {
	fmt.Println(msg)
	status.SetAttribute("TITLE", msg)
}

func field(label string, text iup.Ihandle) iup.Ihandle {
	text.SetAttributes("EXPAND=HORIZONTAL, VISIBLECOLUMNS=20")
	text.SetCallback("MASKFAIL_CB", iup.MaskFailFunc(func(ih iup.Ihandle, value string) int {
		report(fmt.Sprintf("MASKFAIL_CB on %s: %q rejected", label, value))
		return iup.DEFAULT
	}))
	return iup.Hbox(iup.Label(label), text).SetAttributes("NGAP=5, ALIGNMENT=ACENTER, NORMALIZESIZE=VERTICAL")
}

func main() {
	iup.Open()
	defer iup.Close()

	status = iup.Label("Type into the fields; rejected keys are reported here").SetAttribute("EXPAND", "HORIZONTAL")

	digits := iup.Text().SetAttribute("MASK", "/d*")
	integer := iup.Text().SetAttribute("MASKINT", "0:100")
	floating := iup.Text().SetAttributes("MASKDECIMALSYMBOL=\",\", MASKFLOAT=-1.5:1.5")
	real := iup.Text().SetAttribute("MASKREAL", "UNSIGNED")
	caseless := iup.Text().SetAttributes("MASKCASEI=YES, MASK=\"[a-f]*\"")
	noEmpty := iup.Text().SetAttributes("MASKNOEMPTY=YES, MASK=\"/w+\", VALUE=required")

	masked := iup.Button("VALUEMASKED=12ab on the digits field")
	masked.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		digits.SetAttribute("VALUEMASKED", "12ab")
		report("VALUEMASKED=12ab rejected, VALUE is still " + fmt.Sprintf("%q", digits.GetAttribute("VALUE")))
		digits.SetAttribute("VALUEMASKED", "1234")
		report("VALUEMASKED=1234 accepted, VALUE is " + digits.GetAttribute("VALUE"))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(
		field("MASK=/d* (digits)", digits),
		field("MASKINT=0:100", integer),
		field("MASKFLOAT=-1.5:1.5 with MASKDECIMALSYMBOL=,", floating),
		field("MASKREAL=UNSIGNED", real),
		field("MASK=[a-f]* with MASKCASEI=YES", caseless),
		field("MASK=/w+ with MASKNOEMPTY=YES", noEmpty),
		masked,
		status,
	).SetAttributes("NMARGIN=10x10, NGAP=6")).SetAttribute("TITLE", "Text MASK")

	iup.Show(dlg)
	iup.MainLoop()
}
