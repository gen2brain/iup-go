//go:build !cgo && !js && !extlib && qt6 && ctrl && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64_qt6/libiupctrl.so.gz
var embIupCtrlQt6LinuxAmd64 []byte

var _ = regEmbed("iupctrl", embIupCtrlQt6LinuxAmd64)
