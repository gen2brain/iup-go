package main

import (
	"bytes"
	"encoding/xml"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/apple"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/icon"
)

var iosIcons = []struct {
	name string
	size int
}{
	{"Icon-60@2x.png", 120}, {"Icon-60@3x.png", 180},
	{"Icon-76@2x.png", 152}, {"Icon-83.5@2x.png", 167},
	{"Icon-1024.png", 1024},
}

func packageIOS(c *config) error {
	arch := c.goarch
	if arch != "arm64" && !(c.simulator && arch == "amd64") {
		return fmt.Errorf("unsupported ios arch %q", arch)
	}

	env, err := iosToolchain(c, arch)
	if err != nil {
		return err
	}

	tmp, err := os.MkdirTemp("", "iupkg-")
	if err != nil {
		return err
	}
	defer os.RemoveAll(tmp)

	app := filepath.Join(tmp, "Payload", c.exe+".app")
	if err := os.MkdirAll(app, 0o755); err != nil {
		return err
	}
	if err := goBuildWith(c, env, filepath.Join(app, c.exe), nil, nil, nil); err != nil {
		return err
	}

	img, err := icon.Load(c.icon)
	if err != nil {
		return err
	}
	for _, ic := range iosIcons {
		data, err := icon.PNG(icon.Resize(img, ic.size))
		if err != nil {
			return err
		}
		if err := os.WriteFile(filepath.Join(app, ic.name), data, 0o644); err != nil {
			return err
		}
	}
	if err := os.WriteFile(filepath.Join(app, "Info.plist"), iosInfoPlist(c), 0o644); err != nil {
		return err
	}

	if !c.simulator {
		if err := signIOS(c, app, tmp); err != nil {
			return err
		}
	}

	suffix := ".ipa"
	if c.simulator {
		suffix = "-simulator.zip"
	}
	archive := filepath.Join(c.out, c.exe+suffix)
	if err := zipDir(archive, filepath.Join(tmp, "Payload")); err != nil {
		return err
	}
	fmt.Fprintln(os.Stderr, archive)

	if c.install && !c.simulator {
		if runtime.GOOS == "darwin" {
			return run("xcrun", "devicectl", "device", "install", "app", archive)
		}
		return run("go-ios", "install", "--path="+archive)
	}
	return nil
}

func iosToolchain(c *config, arch string) ([]string, error) {
	env := []string{"GOOS=ios", "GOARCH=" + arch, "CGO_ENABLED=1", "IOS_DEPLOYMENT_TARGET=" + c.minOS}
	if runtime.GOOS == "darwin" {
		sdk := "iphoneos"
		target := "arm64-apple-ios" + c.minOS
		if c.simulator {
			sdk = "iphonesimulator"
			target = map[string]string{"arm64": "arm64", "amd64": "x86_64"}[arch] + "-apple-ios" + c.minOS + "-simulator"
		}
		sdkPath, err := exec.Command("xcrun", "-sdk", sdk, "--show-sdk-path").Output()
		if err != nil {
			return nil, fmt.Errorf("xcrun: %w", err)
		}
		cc, err := exec.Command("xcrun", "-sdk", sdk, "-find", "clang").Output()
		if err != nil {
			return nil, fmt.Errorf("xcrun: %w", err)
		}
		flags := "-isysroot " + strings.TrimSpace(string(sdkPath)) + " -target " + target
		return append(env,
			"CC="+strings.TrimSpace(string(cc)), "CXX="+strings.TrimSpace(string(cc))+"++",
			"CGO_CFLAGS="+flags, "CGO_LDFLAGS="+flags), nil
	}

	cc := "arm64-apple-ios-clang"
	if c.simulator {
		cc = map[string]string{"arm64": "arm64", "amd64": "x86_64"}[arch] + "-apple-ios-simulator-clang"
	}
	if v := os.Getenv("CC"); v != "" {
		cc = v
	}
	if _, err := exec.LookPath(cc); err != nil {
		return nil, fmt.Errorf("%s not found; install osxcross with the iOS SDK or set CC", cc)
	}
	return append(env, "CC="+cc, "CXX="+cc+"++"), nil
}

func iosInfoPlist(c *config) []byte {
	var b bytes.Buffer
	b.WriteString(`<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
`)
	str := func(key, value string) {
		b.WriteString("\t<key>" + key + "</key>\n\t<string>")
		xml.EscapeText(&b, []byte(value))
		b.WriteString("</string>\n")
	}
	platform := "iPhoneOS"
	if c.simulator {
		platform = "iPhoneSimulator"
	}
	str("CFBundleDevelopmentRegion", "en")
	str("CFBundleDisplayName", c.name)
	str("CFBundleExecutable", c.exe)
	str("CFBundleIdentifier", c.id)
	str("CFBundleInfoDictionaryVersion", "6.0")
	str("CFBundleName", c.name)
	str("CFBundlePackageType", "APPL")
	str("CFBundleShortVersionString", c.version)
	str("CFBundleVersion", strconv.Itoa(c.build))
	b.WriteString("\t<key>CFBundleSupportedPlatforms</key>\n\t<array>\n\t\t<string>" + platform + "</string>\n\t</array>\n")
	b.WriteString("\t<key>LSRequiresIPhoneOS</key>\n\t<true/>\n")
	str("MinimumOSVersion", c.minOS)
	b.WriteString(`	<key>UIDeviceFamily</key>
	<array>
		<integer>1</integer>
		<integer>2</integer>
	</array>
	<key>CFBundleIcons</key>
	<dict>
		<key>CFBundlePrimaryIcon</key>
		<dict>
			<key>CFBundleIconFiles</key>
			<array>
				<string>Icon-60</string>
			</array>
		</dict>
	</dict>
	<key>CFBundleIcons~ipad</key>
	<dict>
		<key>CFBundlePrimaryIcon</key>
		<dict>
			<key>CFBundleIconFiles</key>
			<array>
				<string>Icon-60</string>
				<string>Icon-76</string>
				<string>Icon-83.5</string>
			</array>
		</dict>
	</dict>
	<key>UILaunchScreen</key>
	<dict/>
	<key>UIRequiredDeviceCapabilities</key>
	<array>
		<string>arm64</string>
	</array>
	<key>UISupportedInterfaceOrientations</key>
	<array>
		<string>UIInterfaceOrientationPortrait</string>
		<string>UIInterfaceOrientationLandscapeLeft</string>
		<string>UIInterfaceOrientationLandscapeRight</string>
	</array>
	<key>UISupportedInterfaceOrientations~ipad</key>
	<array>
		<string>UIInterfaceOrientationPortrait</string>
		<string>UIInterfaceOrientationPortraitUpsideDown</string>
		<string>UIInterfaceOrientationLandscapeLeft</string>
		<string>UIInterfaceOrientationLandscapeRight</string>
	</array>
`)
	if c.hasPermission("camera") {
		str("NSCameraUsageDescription", c.name+" uses the camera.")
	}
	if c.hasPermission("microphone") {
		str("NSMicrophoneUsageDescription", c.name+" uses the microphone.")
	}
	if c.hasPermission("location") {
		str("NSLocationWhenInUseUsageDescription", c.name+" uses your location.")
	}
	b.WriteString("</dict>\n</plist>\n")
	return b.Bytes()
}

var (
	profileEntitlements = regexp.MustCompile(`(?s)<key>Entitlements</key>\s*(<dict>.*?</dict>)`)
	profileAppID        = regexp.MustCompile(`(<key>application-identifier</key>\s*<string>[A-Z0-9]+\.)\*(</string>)`)
)

func profileEntitlementsPlist(profile []byte, bundleID string) ([]byte, error) {
	m := profileEntitlements.FindSubmatch(profile)
	if m == nil {
		return nil, errors.New("provisioning profile has no Entitlements")
	}
	dict := profileAppID.ReplaceAll(m[1], []byte("${1}"+bundleID+"${2}"))
	return fmt.Appendf(nil, `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
%s
</plist>
`, dict), nil
}

func embedProfile(path, app, bundleID string) ([]byte, error) {
	profile, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}
	if err := os.WriteFile(filepath.Join(app, "embedded.mobileprovision"), profile, 0o644); err != nil {
		return nil, err
	}
	entitlements, err := profileEntitlementsPlist(profile, bundleID)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", path, err)
	}
	return entitlements, nil
}

func signIOS(c *config, app, tmp string) error {
	if c.sign == "" || c.profile == "" {
		fmt.Fprintln(os.Stderr, "iupkg: --sign and --profile not given, the app is not signed")
		return nil
	}
	entitlements, err := embedProfile(c.profile, app, c.id)
	if err != nil {
		return err
	}
	entFile := filepath.Join(tmp, "entitlements.plist")
	if err := os.WriteFile(entFile, entitlements, 0o644); err != nil {
		return err
	}

	if c.signer == "codesign" {
		return run("codesign", "--force", "--sign", c.sign, "--timestamp=none", "--entitlements", entFile, app)
	}
	signer, err := appleSigner(c)
	if err != nil {
		return err
	}
	_, err = apple.SignBundle(app, apple.BundleOptions{Signer: signer, Entitlements: entitlements})
	return err
}
