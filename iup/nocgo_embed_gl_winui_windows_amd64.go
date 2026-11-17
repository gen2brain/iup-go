//go:build !cgo && !js && !extlib && winui && gl && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64_winui/libiupgl.dll.gz
var embIupGlWinuiWindowsAmd64 []byte

var _ = regEmbed("iupgl", embIupGlWinuiWindowsAmd64)
