//go:build !cgo && !js && !extlib && winui && media && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64_winui/libiupmedia.dll.gz
var embIupMediaWinuiWindowsArm64 []byte

var _ = regEmbed("iupmedia", embIupMediaWinuiWindowsArm64)
