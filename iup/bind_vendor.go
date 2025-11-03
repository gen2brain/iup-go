//go:build required

package iup

import (
	_ "github.com/gen2brain/iup-go/iup/external"
	_ "github.com/gen2brain/iup-go/iup/external/include"
	_ "github.com/gen2brain/iup-go/iup/external/src"
	_ "github.com/gen2brain/iup-go/iup/external/src/cocoa"
	_ "github.com/gen2brain/iup-go/iup/external/src/gtk"
	_ "github.com/gen2brain/iup-go/iup/external/src/mot"
	_ "github.com/gen2brain/iup-go/iup/external/src/qt"
	_ "github.com/gen2brain/iup-go/iup/external/src/win"
	_ "github.com/gen2brain/iup-go/iup/external/src/win/wdl"
	_ "github.com/gen2brain/iup-go/iup/external/src/win/wdl/dummy"
	_ "github.com/gen2brain/iup-go/iup/external/srcgl"
	_ "github.com/gen2brain/iup-go/iup/external/srcweb"
)
