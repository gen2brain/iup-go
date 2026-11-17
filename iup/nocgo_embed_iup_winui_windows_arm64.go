//go:build !cgo && !js && !extlib && winui && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64_winui/libiup.dll.gz
var embIupWinuiWindowsArm64 []byte

var _ = regEmbed("iup", embIupWinuiWindowsArm64)
