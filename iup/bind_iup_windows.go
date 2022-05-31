//go:build windows && !nomanifest

package iup

// IUP controls appearance will follow the system appearance only if you are using the manifest.
// If not using the manifest, then it will always look like Windows XP Classic.
import _ "github.com/gen2brain/iup-go/iup/manifest"
