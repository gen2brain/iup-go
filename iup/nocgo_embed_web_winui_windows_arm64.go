//go:build !cgo && !js && !extlib && winui && web && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64_winui/libiupweb.dll.gz
var embIupWebWinuiWindowsArm64 []byte

var _ = regEmbed("iupweb", embIupWebWinuiWindowsArm64)
