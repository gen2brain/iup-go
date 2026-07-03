//go:build !cgo && !js && !extlib && gl && darwin && arm64

package iup

import _ "embed"

//go:embed libs/darwin_arm64/libiupgl.dylib.gz
var embIupGlDarwinArm64 []byte

var _ = regEmbed("iupgl", embIupGlDarwinArm64)
