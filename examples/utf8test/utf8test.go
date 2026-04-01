package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// UTF-8 mode is enabled by default in Go bindings
	fmt.Println("UTF8MODE:", iup.GetGlobal("UTF8MODE"))
	fmt.Println("UTF8MODE_FILE:", iup.GetGlobal("UTF8MODE_FILE"))
	fmt.Println("DRIVER:", iup.GetGlobal("DRIVER"))
	fmt.Println("DEFAULTFONT:", iup.GetGlobal("DEFAULTFONT"))

	// UTF-8 test strings in various languages
	labelEnglish := iup.Label("English: Hello World")
	labelChinese := iup.Label("Chinese: 你好世界 (Nǐ hǎo shìjiè)")
	labelJapanese := iup.Label("Japanese: こんにちは世界 (Kon'nichiwa sekai)")
	labelKorean := iup.Label("Korean: 안녕하세요 세계 (Annyeonghaseyo segye)")
	labelRussian := iup.Label("Russian: Привет мир (Privet mir)")
	labelArabic := iup.Label("Arabic: مرحبا بالعالم (Marhaban bialealim)")
	labelGreek := iup.Label("Greek: Γεια σου κόσμε (Geia sou kosme)")
	labelHebrew := iup.Label("Hebrew: שלום עולם (Shalom olam)")
	labelSerbian := iup.Label("Serbian: Здраво свете (Zdravo svete)")
	labelHindi := iup.Label("Hindi: नमस्ते दुनिया (Namaste duniya)")
	labelThai := iup.Label("Thai: สวัสดีชาวโลก (S̄wạs̄dī chāw lok)")
	labelEmoji := iup.Label("Emoji: 🌍🌎🌏 Hello 👋 World 🌟")

	// UTF-8 button
	btnUTF8 := iup.Button("Click Me: 按钮 🔘")
	iup.SetCallback(btnUTF8, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		iup.Message("UTF-8 Test", "Button clicked! 成功! ✓")
		return iup.DEFAULT
	}))

	// UTF-8 text input
	textInput := iup.Text()
	textInput.SetAttribute("EXPAND", "HORIZONTAL")
	textInput.SetAttribute("VALUE", "Type UTF-8 here: 你好 مرحبا Привет")

	// Text output area
	textOutput := iup.Text()
	textOutput.SetAttributes("MULTILINE=YES,EXPAND=YES,SIZE=300x100")
	textOutput.SetAttribute("VALUE", "Multi-line UTF-8 test:\n"+
		"Line 1: English\n"+
		"Line 2: 中文 (Chinese)\n"+
		"Line 3: 日本語 (Japanese)\n"+
		"Line 4: Русский (Russian)\n"+
		"Line 5: العربية (Arabic)\n"+
		"Line 6: Emoji: 😀😃😄😁😊🎉🎊")

	// Button to copy input to output
	btnCopy := iup.Button("Copy Input → Output")
	iup.SetCallback(btnCopy, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		value := textInput.GetAttribute("VALUE")
		textOutput.SetAttribute("VALUE", value)
		fmt.Println("Copied text:", value)
		return iup.DEFAULT
	}))

	// Button to test getters
	btnTest := iup.Button("Test Get/Set UTF-8")
	iup.SetCallback(btnTest, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		// Set UTF-8 text
		testStr := "Test: 你好世界 مرحبا Привет ✓"
		textInput.SetAttribute("VALUE", testStr)

		// Get it back
		retrieved := textInput.GetAttribute("VALUE")

		// Compare
		if retrieved == testStr {
			fmt.Println("✓ UTF-8 round-trip successful!")
			iup.Message("Success", "UTF-8 round-trip test PASSED! ✓\n\nSet: "+testStr+"\nGot: "+retrieved)
		} else {
			fmt.Println("✗ UTF-8 round-trip FAILED!")
			fmt.Printf("Set: %q\nGot: %q\n", testStr, retrieved)
			iup.Message("Error", "UTF-8 round-trip test FAILED!\n\nSet: "+testStr+"\nGot: "+retrieved)
		}
		return iup.DEFAULT
	}))

	// Frame for labels
	frameLabels := iup.Frame(
		iup.Vbox(
			labelEnglish,
			labelChinese,
			labelJapanese,
			labelKorean,
			labelRussian,
			labelArabic,
			labelGreek,
			labelHebrew,
			labelSerbian,
			labelHindi,
			labelThai,
			labelEmoji,
		),
	)
	frameLabels.SetAttribute("TITLE", "UTF-8 Labels (Multiple Languages)")

	// Frame for input/output
	frameIO := iup.Frame(
		iup.Vbox(
			iup.Label("Single-line UTF-8 Input:"),
			textInput,
			iup.Hbox(
				btnCopy,
				btnTest,
			),
			iup.Label("Multi-line UTF-8 Output:"),
			textOutput,
		),
	)
	frameIO.SetAttribute("TITLE", "UTF-8 Text Input/Output Test")

	// Main layout
	vbox := iup.Vbox(
		frameLabels,
		btnUTF8,
		frameIO,
	)
	vbox.SetAttribute("MARGIN", "10x10")
	vbox.SetAttribute("GAP", "10")

	// Dialog
	dlg := iup.Dialog(vbox)
	dlg.SetAttribute("TITLE", "UTF-8 Test")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)

	iup.MainLoop()
}
