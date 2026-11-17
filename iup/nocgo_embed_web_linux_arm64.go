//go:build !cgo && !js && !extlib && !qt6 && web && linux && arm64

package iup

import _ "embed"

//go:embed libs/linux_arm64/libiupweb.so.gz
var embIupWebLinuxArm64 []byte

var _ = regEmbed("iupweb", embIupWebLinuxArm64)
