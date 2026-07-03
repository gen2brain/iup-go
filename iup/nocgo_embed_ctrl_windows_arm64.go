//go:build !cgo && !js && !extlib && ctrl && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64/libiupctrl.dll.gz
var embIupCtrlWindowsArm64 []byte

var _ = regEmbed("iupctrl", embIupCtrlWindowsArm64)
