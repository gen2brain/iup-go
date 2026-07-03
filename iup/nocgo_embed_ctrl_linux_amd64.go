//go:build !cgo && !js && !extlib && ctrl && linux && amd64

package iup

import _ "embed"

//go:embed libs/linux_amd64/libiupctrl.so.gz
var embIupCtrlLinuxAmd64 []byte

var _ = regEmbed("iupctrl", embIupCtrlLinuxAmd64)
