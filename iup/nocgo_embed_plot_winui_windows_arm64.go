//go:build !cgo && !js && !extlib && winui && plot && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64_winui/libiupplot.dll.gz
var embIupPlotWinuiWindowsArm64 []byte

var _ = regEmbed("iupplot", embIupPlotWinuiWindowsArm64)
