//go:build !cgo && !js && !extlib && winui && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64_winui/libiup.dll.gz
var embIupWinuiWindowsAmd64 []byte

var _ = regEmbed("iup", embIupWinuiWindowsAmd64)
