//go:build !cgo && !js && !extlib && qt6 && ctrl && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64_qt6/libiupctrl.so.gz
var embIupCtrlQt6LinuxArm64 []byte

var _ = regEmbed("iupctrl", embIupCtrlQt6LinuxArm64)
