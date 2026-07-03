//go:build !cgo && !js && !extlib && plot && darwin && arm64

package iup

import _ "embed"

//go:embed libs/darwin_arm64/libiupplot.dylib.gz
var embIupPlotDarwinArm64 []byte

var _ = regEmbed("iupplot", embIupPlotDarwinArm64)
