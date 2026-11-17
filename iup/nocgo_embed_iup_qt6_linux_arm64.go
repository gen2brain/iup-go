//go:build !cgo && !js && !extlib && qt6 && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64_qt6/libiup.so.gz
var embIupQt6LinuxArm64 []byte

var _ = regEmbed("iup", embIupQt6LinuxArm64)
