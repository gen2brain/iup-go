//go:build !cgo && !js && !extlib && web && windows && amd64

package iup

import _ "embed"

//go:embed libs/windows_amd64/libiupweb.dll.gz
var embIupWebWindowsAmd64 []byte

var _ = regEmbed("iupweb", embIupWebWindowsAmd64)
