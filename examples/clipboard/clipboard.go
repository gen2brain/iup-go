package main

import (
	"fmt"
	"runtime"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

var clipboard iup.Ihandle

func logMsg(msg string) {
	log := iup.GetHandle("log")
	if log != 0 {
		timestamp := time.Now().Format("15:04:05")
		log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s\n", timestamp, msg))
	}
	fmt.Println(msg)
}

func updateStatus() {
	status := iup.GetHandle("status")
	if status == 0 {
		return
	}

	textAvail := clipboard.GetAttribute("TEXTAVAILABLE")
	imageAvail := clipboard.GetAttribute("IMAGEAVAILABLE")

	statusText := fmt.Sprintf("Text: %s | Image: %s", textAvail, imageAvail)

	// Platform-specific availability checks
	if runtime.GOOS == "windows" {
		wmfAvail := clipboard.GetAttribute("WMFAVAILABLE")
		emfAvail := clipboard.GetAttribute("EMFAVAILABLE")
		statusText += fmt.Sprintf(" | WMF: %s | EMF: %s", wmfAvail, emfAvail)
	} else if runtime.GOOS == "darwin" {
		pdfAvail := clipboard.GetAttribute("PDFAVAILABLE")
		statusText += fmt.Sprintf(" | PDF: %s", pdfAvail)
	}

	status.SetAttribute("TITLE", statusText)
}

func main() {
	iup.Open()
	defer iup.Close()

	clipboard = iup.Clipboard()
	defer clipboard.Destroy()

	// Text input/output
	txtInput := iup.Text()
	txtInput.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, VISIBLELINES=2")
	txtInput.SetAttribute("VALUE", "Type or paste text here...")

	txtOutput := iup.Text()
	txtOutput.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, VISIBLELINES=2, READONLY=YES")

	// Custom format fields
	txtFormatName := iup.Text().SetAttributes("EXPAND=HORIZONTAL")
	txtFormatName.SetAttribute("VALUE", "text/custom")

	txtFormatData := iup.Text().SetAttributes("EXPAND=HORIZONTAL")
	txtFormatData.SetAttribute("VALUE", "Custom format data")

	// HTML field (Qt driver only)
	txtHTML := iup.Text().SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, VISIBLELINES=2")
	txtHTML.SetAttribute("VALUE", "<h1>HTML Content</h1><p>This is <b>bold</b> text.</p>")

	// Log widget
	txtLog := iup.Text()
	txtLog.SetAttributes("MULTILINE=YES, EXPAND=HORIZONTAL, READONLY=YES, VISIBLELINES=10")
	driver := iup.GetGlobal("DRIVER")
	txtLog.SetAttribute("VALUE", "=== IUP Clipboard Demo ===\n"+
		"Platform: "+runtime.GOOS+"\n"+
		"Driver: "+driver+"\n"+
		"Test all clipboard features below.\n"+
		"---\n")

	// Status label
	lblStatus := iup.Label("Status: Ready").SetAttributes("EXPAND=HORIZONTAL")

	iup.SetHandle("log", txtLog)
	iup.SetHandle("status", lblStatus)

	// Basic text operations
	btnCopyText := iup.Button("Copy Text").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		text := txtInput.GetAttribute("VALUE")
		clipboard.SetAttribute("TEXT", text)
		logMsg(fmt.Sprintf("Copied text to clipboard: '%s'", text))
		updateStatus()
		return iup.DEFAULT
	}))

	btnPasteText := iup.Button("Paste Text").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		if clipboard.GetAttribute("TEXTAVAILABLE") == "YES" {
			text := clipboard.GetAttribute("TEXT")
			txtOutput.SetAttribute("VALUE", text)
			logMsg(fmt.Sprintf("Pasted text from clipboard: '%s'", text))
		} else {
			logMsg("No text available in clipboard")
		}
		updateStatus()
		return iup.DEFAULT
	}))

	btnClearClipboard := iup.Button("Clear Clipboard").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		clipboard.SetAttribute("TEXT", "")
		logMsg("Clipboard cleared")
		updateStatus()
		return iup.DEFAULT
	}))

	// Custom format operations
	btnRegisterFormat := iup.Button("Register Format").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		formatName := txtFormatName.GetAttribute("VALUE")
		clipboard.SetAttribute("ADDFORMAT", formatName)
		logMsg(fmt.Sprintf("Registered custom format: '%s'", formatName))
		return iup.DEFAULT
	}))

	btnCopyFormat := iup.Button("Copy Format Data").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		formatName := txtFormatName.GetAttribute("VALUE")
		formatData := txtFormatData.GetAttribute("VALUE")

		clipboard.SetAttribute("FORMAT", formatName)
		clipboard.SetAttribute("FORMATDATASTRING", formatData)

		logMsg(fmt.Sprintf("Copied custom format '%s': '%s'", formatName, formatData))
		updateStatus()
		return iup.DEFAULT
	}))

	btnPasteFormat := iup.Button("Paste Format Data").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		formatName := txtFormatName.GetAttribute("VALUE")
		clipboard.SetAttribute("FORMAT", formatName)

		if clipboard.GetAttribute("FORMATAVAILABLE") == "YES" {
			data := clipboard.GetAttribute("FORMATDATASTRING")
			size := clipboard.GetAttribute("FORMATDATASIZE")
			txtFormatData.SetAttribute("VALUE", data)
			logMsg(fmt.Sprintf("Pasted custom format '%s' (size: %s): '%s'", formatName, size, data))
		} else {
			logMsg(fmt.Sprintf("Custom format '%s' not available in clipboard", formatName))
		}
		updateStatus()
		return iup.DEFAULT
	}))

	// HTML operations (Qt driver only)
	var btnCopyHTML, btnPasteHTML iup.Ihandle
	if driver == "Qt" {
		btnCopyHTML = iup.Button("Copy HTML").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			html := txtHTML.GetAttribute("VALUE")
			clipboard.SetAttribute("HTML", html)
			logMsg(fmt.Sprintf("Copied HTML to clipboard: '%s'", html))
			updateStatus()
			return iup.DEFAULT
		}))

		btnPasteHTML = iup.Button("Paste HTML").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			if clipboard.GetAttribute("HTMLAVAILABLE") == "YES" {
				html := clipboard.GetAttribute("HTML")
				txtHTML.SetAttribute("VALUE", html)
				logMsg(fmt.Sprintf("Pasted HTML from clipboard: '%s'", html))
			} else {
				logMsg("No HTML available in clipboard")
			}
			updateStatus()
			return iup.DEFAULT
		}))
	}

	// Platform-specific controls
	var platformButtons []iup.Ihandle
	var platformFrame iup.Ihandle

	if runtime.GOOS == "windows" {
		btnCheckWMF := iup.Button("Check WMF/EMF").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			wmfAvail := clipboard.GetAttribute("WMFAVAILABLE")
			emfAvail := clipboard.GetAttribute("EMFAVAILABLE")
			logMsg(fmt.Sprintf("Windows Metafile available: WMF=%s, EMF=%s", wmfAvail, emfAvail))
			updateStatus()
			return iup.DEFAULT
		}))
		platformButtons = append(platformButtons, btnCheckWMF)

		hboxPlatform := iup.Hbox(platformButtons...).SetAttribute("GAP", "5")
		platformFrame = iup.Frame(hboxPlatform).SetAttributes("TITLE=Windows Metafiles, MARGIN=5x5")
	} else if runtime.GOOS == "darwin" {
		// macOS/Cocoa PDF vector image support
		txtPDFInfo := iup.Text().SetAttributes("EXPAND=HORIZONTAL, READONLY=YES")
		txtPDFInfo.SetAttribute("VALUE", "No PDF data")

		txtPDFFile := iup.Text().SetAttributes("EXPAND=HORIZONTAL")
		txtPDFFile.SetAttribute("VALUE", "/tmp/clipboard_vector.pdf")

		btnCheckPDF := iup.Button("Check PDF").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			pdfAvail := clipboard.GetAttribute("PDFAVAILABLE")
			logMsg(fmt.Sprintf("PDF vector image available: %s", pdfAvail))
			updateStatus()
			return iup.DEFAULT
		}))

		btnGetPDF := iup.Button("Get PDF Info").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			if clipboard.GetAttribute("PDFAVAILABLE") == "YES" {
				pdfData := clipboard.GetAttribute("NATIVEVECTORIMAGE")
				size := clipboard.GetAttribute("FORMATDATASIZE")
				if pdfData != "" {
					logMsg(fmt.Sprintf("Retrieved PDF vector image from clipboard (size: %s bytes)", size))
					txtPDFInfo.SetAttribute("VALUE", fmt.Sprintf("PDF data: %s bytes", size))
				} else {
					logMsg("Failed to retrieve PDF data")
					txtPDFInfo.SetAttribute("VALUE", "Failed to get PDF")
				}
			} else {
				logMsg("No PDF available in clipboard")
				txtPDFInfo.SetAttribute("VALUE", "No PDF available")
			}
			updateStatus()
			return iup.DEFAULT
		}))

		btnSavePDF := iup.Button("Save to File").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			if clipboard.GetAttribute("PDFAVAILABLE") == "YES" {
				filepath := txtPDFFile.GetAttribute("VALUE")
				clipboard.SetAttribute("SAVENATIVEVECTORIMAGE", filepath)
				logMsg(fmt.Sprintf("Saved PDF vector image to: %s", filepath))
			} else {
				logMsg("No PDF available in clipboard to save")
			}
			return iup.DEFAULT
		}))

		platformFrame = iup.Frame(iup.Vbox(
			iup.Label("PDF Vector Image (Cocoa):"),
			txtPDFInfo,
			iup.Hbox(
				btnCheckPDF,
				btnGetPDF,
			).SetAttribute("GAP", "5"),
			iup.Label("Save to file:"),
			txtPDFFile,
			btnSavePDF,
		)).SetAttributes("TITLE=macOS PDF Vector Images, MARGIN=5x5, GAP=5")
	}

	btnCheckStatus := iup.Button("Refresh Status").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		updateStatus()
		logMsg("Status refreshed")
		return iup.DEFAULT
	}))

	// Layout
	vboxMain := iup.Vbox(
		iup.Label("IUP Clipboard Demo").SetAttributes(`FONT="Sans, Bold 12"`),

		iup.Frame(iup.Vbox(
			iup.Label("Input Text:"),
			txtInput,
			iup.Hbox(
				btnCopyText,
				btnPasteText,
				btnClearClipboard,
			).SetAttribute("GAP", "5"),
			iup.Label("Output Text:"),
			txtOutput,
		)).SetAttributes("TITLE=Basic Text Operations, MARGIN=5x5, GAP=5"),

		iup.Frame(iup.Vbox(
			iup.Label("Format Name (e.g., text/custom, application/json):"),
			txtFormatName,
			iup.Label("Format Data:"),
			txtFormatData,
			iup.Hbox(
				btnRegisterFormat,
				btnCopyFormat,
				btnPasteFormat,
			).SetAttribute("GAP", "5"),
		)).SetAttributes(`TITLE="Custom Format Operations", MARGIN=5x5, GAP=5`),
	)

	// Add an HTML section if Qt driver is detected
	if btnCopyHTML != 0 {
		htmlFrame := iup.Frame(iup.Vbox(
			iup.Label("HTML Content (Qt Driver):"),
			txtHTML,
			iup.Hbox(
				btnCopyHTML,
				btnPasteHTML,
			).SetAttribute("GAP", "5"),
		)).SetAttributes(`TITLE="HTML Operations", MARGIN=5x5, GAP=5`)
		vboxMain = iup.Append(vboxMain, htmlFrame)
	}

	// Add platform-specific frame
	if platformFrame != 0 {
		vboxMain = iup.Append(vboxMain, platformFrame)
	}

	// Add status and log
	vboxMain = iup.Append(vboxMain, iup.Hbox(btnCheckStatus, lblStatus).SetAttribute("GAP", "10"))
	vboxMain = iup.Append(vboxMain, iup.Frame(txtLog).SetAttributes(`TITLE="Event Log", MARGIN=5x5`))

	vboxMain.SetAttributes("MARGIN=10x10, GAP=10")

	dlg := iup.Dialog(vboxMain).SetAttributes(`TITLE="IUP Clipboard"`)

	iup.Show(dlg)

	// Initial status update
	updateStatus()

	logMsg(fmt.Sprintf("Clipboard demo ready. Platform: %s, Driver: %s", runtime.GOOS, driver))

	iup.MainLoop()
}
