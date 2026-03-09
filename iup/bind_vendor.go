//go:build required

package iup

import (
	_ "github.com/gen2brain/iup-go/iup/external"
	_ "github.com/gen2brain/iup-go/iup/external/include"
	_ "github.com/gen2brain/iup-go/iup/external/src"
	_ "github.com/gen2brain/iup-go/iup/external/src/cocoa"
	_ "github.com/gen2brain/iup-go/iup/external/src/efl"
	_ "github.com/gen2brain/iup-go/iup/external/src/gtk"
	_ "github.com/gen2brain/iup-go/iup/external/src/gtk4"
	_ "github.com/gen2brain/iup-go/iup/external/src/mot"
	_ "github.com/gen2brain/iup-go/iup/external/src/qt"
	_ "github.com/gen2brain/iup-go/iup/external/src/win"
	_ "github.com/gen2brain/iup-go/iup/external/src/win/wdl"
	_ "github.com/gen2brain/iup-go/iup/external/src/win/wdl/dummy"
	_ "github.com/gen2brain/iup-go/iup/external/src/winui"
	_ "github.com/gen2brain/iup-go/iup/external/src/winui/winrt"
	_ "github.com/gen2brain/iup-go/iup/external/src/winui/winrt/impl"
	_ "github.com/gen2brain/iup-go/iup/external/srcctl"
	_ "github.com/gen2brain/iup-go/iup/external/srcctl/matrix"
	_ "github.com/gen2brain/iup-go/iup/external/srcctl/matrixex"
	_ "github.com/gen2brain/iup-go/iup/external/srcgl"
	_ "github.com/gen2brain/iup-go/iup/external/srcplot"
	_ "github.com/gen2brain/iup-go/iup/external/srcweb"
)
