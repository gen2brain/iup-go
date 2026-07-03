//go:build !cgo && !js && !extlib && plot && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64/libiupplot.so.gz
var embIupPlotLinuxArm64 []byte

var _ = regEmbed("iupplot", embIupPlotLinuxArm64)
