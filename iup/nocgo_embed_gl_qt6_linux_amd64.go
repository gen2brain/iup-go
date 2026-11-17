//go:build !cgo && !js && !extlib && qt6 && gl && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64_qt6/libiupgl.so.gz
var embIupGlQt6LinuxAmd64 []byte

var _ = regEmbed("iupgl", embIupGlQt6LinuxAmd64)
