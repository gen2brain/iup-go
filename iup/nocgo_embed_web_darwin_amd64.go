//go:build !cgo && !js && !extlib && web && darwin && amd64

package iup

import _ "embed"

//go:embed libs/darwin_amd64/libiupweb.dylib.gz
var embIupWebDarwinAmd64 []byte

var _ = regEmbed("iupweb", embIupWebDarwinAmd64)
