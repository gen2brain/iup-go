//go:build !cgo && !js && !extlib && qt6 && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64_qt6/libiup.so.gz
var embIupQt6LinuxAmd64 []byte

var _ = regEmbed("iup", embIupQt6LinuxAmd64)
