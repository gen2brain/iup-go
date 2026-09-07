//go:build !cgo && !js && !extlib && media && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64/libiupmedia.so.gz
var embIupMediaLinuxAmd64 []byte

var _ = regEmbed("iupmedia", embIupMediaLinuxAmd64)
