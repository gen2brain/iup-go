//go:build !cgo && !js && !extlib && plot && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64/libiupplot.so.gz
var embIupPlotLinuxAmd64 []byte

var _ = regEmbed("iupplot", embIupPlotLinuxAmd64)
