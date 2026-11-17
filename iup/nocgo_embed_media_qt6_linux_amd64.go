//go:build !cgo && !js && !extlib && qt6 && media && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64_qt6/libiupmedia.so.gz
var embIupMediaQt6LinuxAmd64 []byte

var _ = regEmbed("iupmedia", embIupMediaQt6LinuxAmd64)
