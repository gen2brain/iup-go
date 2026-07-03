//go:build !cgo && !js && !extlib && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64/libiup.dll.gz
var embIupWindowsAmd64 []byte

var _ = regEmbed("iup", embIupWindowsAmd64)
