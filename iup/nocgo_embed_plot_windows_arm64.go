//go:build !cgo && !js && !extlib && plot && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64/libiupplot.dll.gz
var embIupPlotWindowsArm64 []byte

var _ = regEmbed("iupplot", embIupPlotWindowsArm64)
