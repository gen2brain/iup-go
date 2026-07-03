//go:build !cgo && !js && !extlib && ctrl && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64/libiupctrl.so.gz
var embIupCtrlLinuxArm64 []byte

var _ = regEmbed("iupctrl", embIupCtrlLinuxArm64)
