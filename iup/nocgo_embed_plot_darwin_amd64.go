//go:build !cgo && !js && !extlib && plot && darwin && amd64

package iup

import _ "embed"

//go:embed libs/darwin_amd64/libiupplot.dylib.gz
var embIupPlotDarwinAmd64 []byte

var _ = regEmbed("iupplot", embIupPlotDarwinAmd64)
