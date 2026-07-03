//go:build !cgo && !js && !extlib && web && windows && arm64

package iup

import _ "embed"

//go:embed libs/windows_arm64/libiupweb.dll.gz
var embIupWebWindowsArm64 []byte

var _ = regEmbed("iupweb", embIupWebWindowsArm64)
