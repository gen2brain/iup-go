//go:build !cgo && !js && !extlib && ctrl && darwin && amd64

package iup

import _ "embed"

//go:embed libs/darwin_amd64/libiupctrl.dylib.gz
var embIupCtrlDarwinAmd64 []byte

var _ = regEmbed("iupctrl", embIupCtrlDarwinAmd64)
