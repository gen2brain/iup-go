//go:build !cgo && !js && !extlib && ctrl && darwin && arm64

package iup

import _ "embed"

//go:embed libs/darwin_arm64/libiupctrl.dylib.gz
var embIupCtrlDarwinArm64 []byte

var _ = regEmbed("iupctrl", embIupCtrlDarwinArm64)
