//go:build !cgo && !js && !extlib && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64/libiup.dll.gz
var embIupWindowsArm64 []byte

var _ = regEmbed("iup", embIupWindowsArm64)
