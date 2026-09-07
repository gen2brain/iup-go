//go:build !cgo && !js && !extlib && media && darwin && arm64

package iup

import _ "embed"

//go:embed libs/darwin_arm64/libiupmedia.dylib.gz
var embIupMediaDarwinArm64 []byte

var _ = regEmbed("iupmedia", embIupMediaDarwinArm64)
