//go:build !cgo && !js && !extlib && winui && plot && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64_winui/libiupplot.dll.gz
var embIupPlotWinuiWindowsAmd64 []byte

var _ = regEmbed("iupplot", embIupPlotWinuiWindowsAmd64)
