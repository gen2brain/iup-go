// Package manifest provides a basic manifest for use with iup-go.
// If your Windows is using the Windows Classic theme or any other theme, IUP controls appearance will follow the system appearance only if you are using the manifest.
// If not using the manifest, then it will always look like Windows XP Classic.
//
// You import it for its side effects only, as
//
// 	import _ "github.com/gen2brain/iup-go/iup/manifest"
//
// On non-Windows platforms this package does nothing.
//
// To use `go generate`, install `rsrc` tool with `go install github.com/akavel/rsrc@latest`
package manifest

//go:generate rsrc --manifest iup_386.manifest --arch 386 -o manifest_windows_386.syso
//go:generate rsrc --manifest iup_amd64.manifest --arch amd64 -o manifest_windows_amd64.syso
//go:generate rsrc --manifest iup_arm64.manifest --arch arm64 -o manifest_windows_arm64.syso
