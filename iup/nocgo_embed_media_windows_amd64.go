//go:build !cgo && !js && !extlib && media && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64/libiupmedia.dll.gz
var embIupMediaWindowsAmd64 []byte

var _ = regEmbed("iupmedia", embIupMediaWindowsAmd64)
