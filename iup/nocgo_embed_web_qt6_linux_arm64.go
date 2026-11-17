//go:build !cgo && !js && !extlib && qt6 && web && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64_qt6/libiupweb.so.gz
var embIupWebQt6LinuxArm64 []byte

var _ = regEmbed("iupweb", embIupWebQt6LinuxArm64)
