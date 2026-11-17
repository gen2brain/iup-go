//go:build !cgo && !js && !extlib && winui && web && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64_winui/libiupweb.dll.gz
var embIupWebWinuiWindowsAmd64 []byte

var _ = regEmbed("iupweb", embIupWebWinuiWindowsAmd64)
