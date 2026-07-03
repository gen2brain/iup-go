//go:build !cgo && !js && !extlib && darwin && amd64

package iup

import _ "embed"

//go:embed libs/darwin_amd64/libiup.dylib.gz
var embIupDarwinAmd64 []byte

var _ = regEmbed("iup", embIupDarwinAmd64)
