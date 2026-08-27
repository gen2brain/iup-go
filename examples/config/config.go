package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	// Where the per-user directories land on this platform
	fmt.Println("Directories:")
	fmt.Printf("  CONFIGDIR=%s\n", iup.GetGlobal("CONFIGDIR"))
	fmt.Printf("  DATADIR=%s\n", iup.GetGlobal("DATADIR"))
	fmt.Println()

	// Using APP_NAME attribute on a config handle
	fmt.Println("APP_NAME attribute on config handle")
	config1 := iup.Config()
	config1.SetAttribute("APP_NAME", "TestApp1")
	iup.ConfigLoad(config1)
	filename1 := config1.GetAttribute("FILENAME")
	fmt.Printf("  APP_NAME=TestApp1 -> FILENAME=%s\n", filename1)
	config1.Destroy()

	// Using global APPID as fallback
	fmt.Println("\nGlobal APPID fallback (no APP_NAME, no APPNAME)")
	iup.SetGlobal("APPID", "com.example.MyApp")
	config2 := iup.Config()
	iup.ConfigLoad(config2)
	filename2 := config2.GetAttribute("FILENAME")
	fmt.Printf("  APPID=com.example.MyApp -> FILENAME=%s\n", filename2)
	config2.Destroy()

	// Using global APPNAME
	fmt.Println("\nGlobal APPNAME (no APP_NAME on config)")
	iup.SetGlobal("APPNAME", "GlobalAppName")
	config3 := iup.Config()
	iup.ConfigLoad(config3)
	filename3 := config3.GetAttribute("FILENAME")
	fmt.Printf("  APPNAME=GlobalAppName -> FILENAME=%s\n", filename3)
	config3.Destroy()

	// APP_NAME takes precedence over global APPNAME/APPID
	fmt.Println("\nAPP_NAME takes precedence over global attributes")
	config4 := iup.Config()
	config4.SetAttribute("APP_NAME", "LocalAppName")
	iup.ConfigLoad(config4)
	filename4 := config4.GetAttribute("FILENAME")
	fmt.Printf("  APP_NAME=LocalAppName, APPNAME=GlobalAppName, APPID=com.example.MyApp\n")
	fmt.Printf("  -> FILENAME=%s\n", filename4)
	config4.Destroy()

	// Save and load config
	fmt.Println("\nSave and load config data")
	config5 := iup.Config()
	config5.SetAttribute("APP_NAME", "XDGTestApp")

	// What a previous run left behind
	iup.ConfigLoad(config5)
	fmt.Printf("  Previous run: Title=%q Width=%d Height=%d Volume=%g\n",
		iup.ConfigGetVariableStr(config5, "Window", "Title"),
		iup.ConfigGetVariableInt(config5, "Window", "Width"),
		iup.ConfigGetVariableInt(config5, "Window", "Height"),
		iup.ConfigGetVariableDouble(config5, "Settings", "Volume"))

	// Save some data
	iup.ConfigSetVariableStr(config5, "Window", "Title", "My Application")
	iup.ConfigSetVariableInt(config5, "Window", "Width", 800)
	iup.ConfigSetVariableInt(config5, "Window", "Height", 600)
	iup.ConfigSetVariableDouble(config5, "Settings", "Volume", 0.75)

	result := iup.ConfigSave(config5)
	filename5 := config5.GetAttribute("FILENAME")
	fmt.Printf("  Saved to: %s (result=%d)\n", filename5, result)

	// Read it back
	config6 := iup.Config()
	config6.SetAttribute("APP_NAME", "XDGTestApp")
	iup.ConfigLoad(config6)
	fmt.Printf("  Read back:    Title=%q Width=%d Height=%d Volume=%g\n",
		iup.ConfigGetVariableStr(config6, "Window", "Title"),
		iup.ConfigGetVariableInt(config6, "Window", "Width"),
		iup.ConfigGetVariableInt(config6, "Window", "Height"),
		iup.ConfigGetVariableDouble(config6, "Settings", "Volume"))
	config6.Destroy()

	config5.Destroy()
}
