//go:build !cgo && !js && !extlib && gl && darwin && amd64

package iup

import _ "embed"

//go:embed libs/darwin_amd64/libiupgl.dylib.gz
var embIupGlDarwinAmd64 []byte

var _ = regEmbed("iupgl", embIupGlDarwinAmd64)
