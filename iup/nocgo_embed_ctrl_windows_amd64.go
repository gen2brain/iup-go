//go:build !cgo && !js && !extlib && ctrl && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64/libiupctrl.dll.gz
var embIupCtrlWindowsAmd64 []byte

var _ = regEmbed("iupctrl", embIupCtrlWindowsAmd64)
