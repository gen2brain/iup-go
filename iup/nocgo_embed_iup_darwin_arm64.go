//go:build !cgo && !js && !extlib && darwin && arm64

package iup

import _ "embed"

//go:embed libs/darwin_arm64/libiup.dylib.gz
var embIupDarwinArm64 []byte

var _ = regEmbed("iup", embIupDarwinArm64)
