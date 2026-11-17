//go:build !cgo && !js && !extlib && winui && ctrl && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64_winui/libiupctrl.dll.gz
var embIupCtrlWinuiWindowsArm64 []byte

var _ = regEmbed("iupctrl", embIupCtrlWinuiWindowsArm64)
