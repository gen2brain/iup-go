package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	g := iup.GetGlobal

	fmt.Println("IUP System Information")
	fmt.Println("======================")

	fmt.Println("\nVersion:")
	fmt.Printf("  Version:       %s\n", iup.Version())
	fmt.Printf("  Version Date:  %s\n", iup.VersionDate())
	fmt.Printf("  Version Number:%d\n", iup.VersionNumber())
	fmt.Printf("  Copyright:     %s\n", iup.COPYRIGHT)

	fmt.Println("\nDriver:")
	fmt.Printf("  DRIVER:        %s\n", g("DRIVER"))
	fmt.Printf("  WINDOWING:     %s\n", g("WINDOWING"))

	driver := g("DRIVER")

	if v := g("GTKVERSION"); v != "" {
		fmt.Printf("  GTKVERSION:    %s\n", v)
	}
	if v := g("MOTIFVERSION"); v != "" {
		fmt.Printf("  MOTIFVERSION:  %s\n", v)
		fmt.Printf("  MOTIFNUMBER:   %s\n", g("MOTIFNUMBER"))
	}
	if v := g("QTVERSION"); v != "" {
		fmt.Printf("  QTVERSION:     %s\n", v)
		fmt.Printf("  QTSTYLE:       %s\n", g("QTSTYLE"))
		fmt.Printf("  QTBUILDTYPE:   %s\n", g("QTBUILDTYPE"))
	}
	if v := g("EFLVERSION"); v != "" {
		fmt.Printf("  EFLVERSION:    %s\n", v)
	}
	if v := g("WINUIVERSION"); v != "" {
		fmt.Printf("  WINUIVERSION:  %s\n", v)
	}

	fmt.Println("\nSystem:")
	fmt.Printf("  SYSTEM:          %s\n", g("SYSTEM"))
	fmt.Printf("  SYSTEMVERSION:   %s\n", g("SYSTEMVERSION"))
	fmt.Printf("  SYSTEMLANGUAGE:  %s\n", g("SYSTEMLANGUAGE"))
	fmt.Printf("  SYSTEMLOCALE:    %s\n", g("SYSTEMLOCALE"))
	fmt.Printf("  COMPUTERNAME:    %s\n", g("COMPUTERNAME"))
	fmt.Printf("  USERNAME:        %s\n", g("USERNAME"))
	fmt.Printf("  EXEFILENAME:     %s\n", g("EXEFILENAME"))
	fmt.Printf("  DARKMODE:        %s\n", g("DARKMODE"))
	if v := g("SANDBOX"); v != "" {
		fmt.Printf("  SANDBOX:         %s\n", v)
	}

	fmt.Println("\nScreen:")
	fmt.Printf("  SCREENSIZE:      %s\n", g("SCREENSIZE"))
	fmt.Printf("  FULLSIZE:        %s\n", g("FULLSIZE"))
	fmt.Printf("  SCREENDEPTH:     %s\n", g("SCREENDEPTH"))
	fmt.Printf("  SCREENDPI:       %s\n", g("SCREENDPI"))
	fmt.Printf("  SCROLLBARSIZE:   %s\n", g("SCROLLBARSIZE"))
	fmt.Printf("  TRUECOLORCANVAS: %s\n", g("TRUECOLORCANVAS"))
	if v := g("VIRTUALSCREEN"); v != "" {
		fmt.Printf("  VIRTUALSCREEN:   %s\n", v)
	}
	if v := g("MONITORSCOUNT"); v != "" {
		fmt.Printf("  MONITORSCOUNT:   %s\n", v)
	}
	if v := g("MONITORSINFO"); v != "" {
		fmt.Printf("  MONITORSINFO:    %s\n", v)
	}
	if v := g("OVERLAYSCROLLBAR"); v != "" {
		fmt.Printf("  OVERLAYSCROLLBAR:%s\n", v)
	}

	fmt.Println("\nFonts:")
	fmt.Printf("  DEFAULTFONT:     %s\n", g("DEFAULTFONT"))
	fmt.Printf("  DEFAULTFONTFACE: %s\n", g("DEFAULTFONTFACE"))
	fmt.Printf("  DEFAULTFONTSIZE: %s\n", g("DEFAULTFONTSIZE"))
	fmt.Printf("  DEFAULTFONTSTYLE:%s\n", g("DEFAULTFONTSTYLE"))

	fmt.Println("\nColors:")
	fmt.Printf("  DLGBGCOLOR:      %s\n", g("DLGBGCOLOR"))
	fmt.Printf("  DLGFGCOLOR:      %s\n", g("DLGFGCOLOR"))
	fmt.Printf("  TXTBGCOLOR:      %s\n", g("TXTBGCOLOR"))
	fmt.Printf("  TXTFGCOLOR:      %s\n", g("TXTFGCOLOR"))
	fmt.Printf("  TXTHLCOLOR:      %s\n", g("TXTHLCOLOR"))
	fmt.Printf("  MENUBGCOLOR:     %s\n", g("MENUBGCOLOR"))
	fmt.Printf("  LINKFGCOLOR:     %s\n", g("LINKFGCOLOR"))

	fmt.Println("\nSettings:")
	fmt.Printf("  UTF8MODE:        %s\n", g("UTF8MODE"))
	fmt.Printf("  LANGUAGE:        %s\n", g("LANGUAGE"))
	fmt.Printf("  DEFAULTDECIMALSYMBOL: %s\n", g("DEFAULTDECIMALSYMBOL"))
	fmt.Printf("  DEFAULTBUTTONPADDING: %s\n", g("DEFAULTBUTTONPADDING"))

	if driver == "Win32" || driver == "WinUI" {
		fmt.Println("\nWindows:")
		if v := g("COMCTL32VER6"); v != "" {
			fmt.Printf("  COMCTL32VER6:    %s\n", v)
		}
		if v := g("DWM_COMPOSITION"); v != "" {
			fmt.Printf("  DWM_COMPOSITION: %s\n", v)
		}
		if v := g("LASTERROR"); v != "" {
			fmt.Printf("  LASTERROR:       %s\n", v)
		}
	}

	if driver == "GTK" || driver == "GTK4" || driver == "Motif" {
		fmt.Println("\nX11:")
		if v := g("XSERVERVENDOR"); v != "" {
			fmt.Printf("  XSERVERVENDOR:   %s\n", v)
		}
		if v := g("XVENDORRELEASE"); v != "" {
			fmt.Printf("  XVENDORRELEASE:  %s\n", v)
		}
	}

	if v := g("GL_VERSION"); v != "" {
		fmt.Println("\nOpenGL:")
		fmt.Printf("  GL_VERSION:      %s\n", v)
		fmt.Printf("  GL_VENDOR:       %s\n", g("GL_VENDOR"))
		fmt.Printf("  GL_RENDERER:     %s\n", g("GL_RENDERER"))
	}
}
