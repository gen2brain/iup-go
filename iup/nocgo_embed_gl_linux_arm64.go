//go:build !cgo && !js && !extlib && gl && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64/libiupgl.so.gz
var embIupGlLinuxArm64 []byte

var _ = regEmbed("iupgl", embIupGlLinuxArm64)
