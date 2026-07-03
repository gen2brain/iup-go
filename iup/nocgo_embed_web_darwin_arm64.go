//go:build !cgo && !js && !extlib && web && darwin && arm64

package iup

import _ "embed"

//go:embed libs/darwin_arm64/libiupweb.dylib.gz
var embIupWebDarwinArm64 []byte

var _ = regEmbed("iupweb", embIupWebDarwinArm64)
