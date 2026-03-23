package main

import (
	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Labels with markup
	lblBold := iup.Label(`<b>Bold text</b>`).SetAttributes("MARKUP=YES")
	lblItalic := iup.Label(`<i>Italic text</i>`).SetAttributes("MARKUP=YES")
	lblUnderline := iup.Label(`<u>Underlined text</u>`).SetAttributes("MARKUP=YES")
	lblStrike := iup.Label(`<s>Strikethrough text</s>`).SetAttributes("MARKUP=YES")

	lblCombined := iup.Label(`<b><i>Bold italic</i></b> and <u>underlined</u>`).SetAttributes("MARKUP=YES")

	lblColors := iup.Label(`<span foreground="#FF0000">Red</span> <span foreground="#00AA00">Green</span> <span foreground="#0000FF">Blue</span>`).SetAttributes("MARKUP=YES")
	lblBgColor := iup.Label(`<span background="#FFFF00" foreground="#000000">Highlighted text</span>`).SetAttributes("MARKUP=YES")

	lblSizes := iup.Label(`<big>Big</big> Normal <small>Small</small>`).SetAttributes("MARKUP=YES")
	lblSubSup := iup.Label(`H<sub>2</sub>O and E=mc<sup>2</sup>`).SetAttributes("MARKUP=YES")

	lblSpan := iup.Label(`<span font_family="Courier" font_size="14" foreground="#8B0000">Courier 14pt dark red</span>`).SetAttributes("MARKUP=YES")
	lblWeight := iup.Label(`<span font_weight="bold" foreground="#006400">Bold weight green</span>`).SetAttributes("MARKUP=YES")

	lblComplex := iup.Label(`<b>Name:</b> <i><span foreground="#0000CC">John</span></i> <span background="#EEEEEE"><u>Doe</u></span>`).SetAttributes("MARKUP=YES")
	lblNested := iup.Label(`<b><i>Bold italic</i></b> and <u><span foreground="#8B008B">underline purple</span></u>`).SetAttributes("MARKUP=YES")
	lblMultiFont := iup.Label(`<span font_family="Serif" font_size="16">Serif 16</span> <span font_family="Monospace" font_size="10">Mono 10</span>`).SetAttributes("MARKUP=YES")

	frLabels := iup.Frame(
		iup.Vbox(
			lblBold,
			lblItalic,
			lblUnderline,
			lblStrike,
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			lblCombined,
			lblColors,
			lblBgColor,
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			lblSizes,
			lblSubSup,
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			lblSpan,
			lblWeight,
			iup.Label("").SetAttributes("SEPARATOR=HORIZONTAL"),
			lblComplex,
			lblNested,
			lblMultiFont,
		).SetAttribute("GAP", "3"),
	).SetAttribute("TITLE", "Labels")

	// Buttons with markup
	btnBold := iup.Button(`<b>Bold Button</b>`).SetAttributes("MARKUP=YES, PADDING=5x5")
	btnColor := iup.Button(`<span foreground="#FF0000">Red</span> Button`).SetAttributes("MARKUP=YES, PADDING=5x5")
	btnMixed := iup.Button(`<b>Save</b> <i><span foreground="#666666">(Ctrl+S)</span></i>`).SetAttributes("MARKUP=YES, PADDING=5x5")
	btnPlain := iup.Button("Plain Button").SetAttributes("PADDING=5x5")

	frButtons := iup.Frame(
		iup.Vbox(
			btnBold,
			btnColor,
			btnMixed,
			btnPlain,
		).SetAttribute("GAP", "5"),
	).SetAttribute("TITLE", "Buttons")

	// Toggles with markup
	tglBold := iup.Toggle(`<b>Bold Toggle</b>`).SetAttributes("MARKUP=YES")
	tglColor := iup.Toggle(`<span foreground="#0000FF">Blue Toggle</span>`).SetAttributes("MARKUP=YES")
	tglMixed := iup.Toggle(`<i>Italic</i> and <u>underlined</u>`).SetAttributes("MARKUP=YES")
	tglPlain := iup.Toggle("Plain Toggle")

	frToggles := iup.Frame(
		iup.Vbox(
			tglBold,
			tglColor,
			tglMixed,
			tglPlain,
		).SetAttribute("GAP", "5"),
	).SetAttribute("TITLE", "Toggles")

	dlg := iup.Dialog(
		iup.Hbox(
			frLabels,
			iup.Vbox(
				frButtons,
				frToggles,
			),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttributes(`TITLE="Markup Example"`)

	iup.Show(dlg)
	iup.MainLoop()
}
