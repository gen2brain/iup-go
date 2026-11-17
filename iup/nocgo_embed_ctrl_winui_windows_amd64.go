//go:build !cgo && !js && !extlib && winui && ctrl && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64_winui/libiupctrl.dll.gz
var embIupCtrlWinuiWindowsAmd64 []byte

var _ = regEmbed("iupctrl", embIupCtrlWinuiWindowsAmd64)
