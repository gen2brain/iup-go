//go:build !cgo && !js && !extlib && gl && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64/libiupgl.dll.gz
var embIupGlWindowsArm64 []byte

var _ = regEmbed("iupgl", embIupGlWindowsArm64)
