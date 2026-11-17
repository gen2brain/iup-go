//go:build !cgo && !js && !extlib && qt6 && web && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64_qt6/libiupweb.so.gz
var embIupWebQt6LinuxAmd64 []byte

var _ = regEmbed("iupweb", embIupWebQt6LinuxAmd64)
