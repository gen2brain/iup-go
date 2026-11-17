//go:build !cgo && !js && !extlib && winui && gl && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64_winui/libiupgl.dll.gz
var embIupGlWinuiWindowsArm64 []byte

var _ = regEmbed("iupgl", embIupGlWinuiWindowsArm64)
