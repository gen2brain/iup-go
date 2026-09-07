//go:build !cgo && !js && !extlib && media && darwin && amd64

package iup

import _ "embed"

//go:embed libs/darwin_amd64/libiupmedia.dylib.gz
var embIupMediaDarwinAmd64 []byte

var _ = regEmbed("iupmedia", embIupMediaDarwinAmd64)
