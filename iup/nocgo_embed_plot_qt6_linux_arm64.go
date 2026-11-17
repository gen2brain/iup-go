//go:build !cgo && !js && !extlib && qt6 && plot && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64_qt6/libiupplot.so.gz
var embIupPlotQt6LinuxArm64 []byte

var _ = regEmbed("iupplot", embIupPlotQt6LinuxArm64)
