//go:build !cgo && !js && !extlib && media && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64/libiupmedia.so.gz
var embIupMediaLinuxArm64 []byte

var _ = regEmbed("iupmedia", embIupMediaLinuxArm64)
