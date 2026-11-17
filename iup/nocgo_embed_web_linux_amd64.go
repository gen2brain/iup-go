//go:build !cgo && !js && !extlib && !qt6 && web && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64/libiupweb.so.gz
var embIupWebLinuxAmd64 []byte

var _ = regEmbed("iupweb", embIupWebLinuxAmd64)
