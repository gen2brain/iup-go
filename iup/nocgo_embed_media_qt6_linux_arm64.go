//go:build !cgo && !js && !extlib && qt6 && media && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64_qt6/libiupmedia.so.gz
var embIupMediaQt6LinuxArm64 []byte

var _ = regEmbed("iupmedia", embIupMediaQt6LinuxArm64)
