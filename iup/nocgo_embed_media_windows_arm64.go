//go:build !cgo && !js && !extlib && media && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64/libiupmedia.dll.gz
var embIupMediaWindowsArm64 []byte

var _ = regEmbed("iupmedia", embIupMediaWindowsArm64)
