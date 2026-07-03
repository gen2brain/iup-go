//go:build !cgo && !js && !extlib && gl && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64/libiupgl.so.gz
var embIupGlLinuxAmd64 []byte

var _ = regEmbed("iupgl", embIupGlLinuxAmd64)
