//go:build !cgo && !js && !extlib && winui && media && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64_winui/libiupmedia.dll.gz
var embIupMediaWinuiWindowsAmd64 []byte

var _ = regEmbed("iupmedia", embIupMediaWinuiWindowsAmd64)
