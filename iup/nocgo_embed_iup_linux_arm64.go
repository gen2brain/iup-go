//go:build !cgo && !js && !extlib && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64/libiup.so.gz
var embIupLinuxArm64 []byte

var _ = regEmbed("iup", embIupLinuxArm64)
