//go:build !cgo && !js && !extlib && qt6 && plot && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64_qt6/libiupplot.so.gz
var embIupPlotQt6LinuxAmd64 []byte

var _ = regEmbed("iupplot", embIupPlotQt6LinuxAmd64)
