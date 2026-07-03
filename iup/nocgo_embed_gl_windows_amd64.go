//go:build !cgo && !js && !extlib && gl && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64/libiupgl.dll.gz
var embIupGlWindowsAmd64 []byte

var _ = regEmbed("iupgl", embIupGlWindowsAmd64)
