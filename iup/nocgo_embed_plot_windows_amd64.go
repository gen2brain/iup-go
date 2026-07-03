//go:build !cgo && !js && !extlib && plot && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64/libiupplot.dll.gz
var embIupPlotWindowsAmd64 []byte

var _ = regEmbed("iupplot", embIupPlotWindowsAmd64)
