//go:build !cgo && !js && !extlib && qt6 && gl && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64_qt6/libiupgl.so.gz
var embIupGlQt6LinuxArm64 []byte

var _ = regEmbed("iupgl", embIupGlQt6LinuxArm64)
