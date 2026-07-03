//go:build !cgo && !js && !extlib && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64/libiup.so.gz
var embIupLinuxAmd64 []byte

var _ = regEmbed("iup", embIupLinuxAmd64)
