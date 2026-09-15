package main

import (
	"archive/zip"
	"bytes"
	"compress/gzip"
	"encoding/xml"
	"errors"
	"fmt"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"slices"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/apple"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/icon"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/macho"
)

var optionalLibs = map[string]string{
	"ctrl":  "iupctrl",
	"gl":    "iupgl",
	"media": "iupmedia",
	"plot":  "iupplot",
	"web":   "iupweb",
}

func packageDarwin(c *config) error {
	archs := []string{c.goarch}
	if c.goarch == "universal" {
		archs = []string{"amd64", "arm64"}
	}

	app := filepath.Join(c.out, c.name+".app")
	if err := os.RemoveAll(app); err != nil {
		return err
	}
	contents := filepath.Join(app, "Contents")
	for _, dir := range []string{"MacOS", "Resources"} {
		if err := os.MkdirAll(filepath.Join(contents, dir), 0o755); err != nil {
			return err
		}
	}

	tmp, err := os.MkdirTemp("", "iupkg-")
	if err != nil {
		return err
	}
	defer os.RemoveAll(tmp)

	var tags []string
	if !c.cgo {
		tags = append(tags, "extlib")
	}

	var bins [][]byte
	for _, arch := range archs {
		bin := filepath.Join(tmp, c.exe+"-"+arch)
		if err := goBuild(c, arch, bin, tags, nil); err != nil {
			return err
		}
		data, err := os.ReadFile(bin)
		if err != nil {
			return err
		}
		bins = append(bins, data)
	}
	exe, err := macho.Universal(bins)
	if err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(contents, "MacOS", c.exe), exe, 0o755); err != nil {
		return err
	}

	if !c.cgo {
		if err := copyDylibs(c, archs, filepath.Join(contents, "Frameworks")); err != nil {
			return err
		}
	}

	img, err := icon.Load(c.icon)
	if err != nil {
		return err
	}
	icns, err := icon.ICNS(img)
	if err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(contents, "Resources", c.exe+".icns"), icns, 0o644); err != nil {
		return err
	}

	minOS, err := macho.MinVersion(bins[0])
	if err != nil {
		return err
	}
	if c.cgo {
		goMin, err := goMacOSMinVersion(c, archs[0], tmp)
		if err != nil {
			return err
		}
		minOS = maxVersion(minOS, goMin)
	}
	if err := os.WriteFile(filepath.Join(contents, "Info.plist"), infoPlist(c, minOS), 0o644); err != nil {
		return err
	}

	if err := signDarwin(c, app, tmp); err != nil {
		return err
	}

	archive := filepath.Join(c.out, fmt.Sprintf("%s-%s-darwin-%s.zip", c.exe, c.version, c.goarch))
	if err := zipDir(archive, app); err != nil {
		return err
	}

	fmt.Fprintln(os.Stderr, app)
	fmt.Fprintln(os.Stderr, archive)
	return nil
}

func copyDylibs(c *config, archs []string, dir string) error {
	major, err := c.iupMajor()
	if err != nil {
		return err
	}
	bases := []string{"iup"}
	for _, tag := range c.tags {
		if base, ok := optionalLibs[tag]; ok && !slices.Contains(bases, base) {
			bases = append(bases, base)
		}
	}

	if err := os.MkdirAll(dir, 0o755); err != nil {
		return err
	}
	for _, base := range bases {
		var libs [][]byte
		for _, arch := range archs {
			data, err := readGzip(filepath.Join(c.iupDir, "libs", "darwin_"+arch, "lib"+base+".dylib.gz"))
			if err != nil {
				return err
			}
			libs = append(libs, data)
		}
		lib, err := macho.Universal(libs)
		if err != nil {
			return err
		}
		if err := os.WriteFile(filepath.Join(dir, "lib"+base+"."+major+".dylib"), lib, 0o644); err != nil {
			return err
		}
	}
	return nil
}

func readGzip(path string) ([]byte, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	zr, err := gzip.NewReader(f)
	if err != nil {
		return nil, fmt.Errorf("%s: %w", path, err)
	}
	return io.ReadAll(zr)
}

func goMacOSMinVersion(c *config, arch, tmp string) (string, error) {
	dir := filepath.Join(tmp, "gomin")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return "", err
	}
	if err := os.WriteFile(filepath.Join(dir, "go.mod"), []byte("module gomin\n"), 0o644); err != nil {
		return "", err
	}
	if err := os.WriteFile(filepath.Join(dir, "main.go"), []byte("package main\n\nfunc main() {}\n"), 0o644); err != nil {
		return "", err
	}
	bin := filepath.Join(dir, "gomin")
	cmd := exec.Command("go", "build", "-o", bin, ".")
	cmd.Dir = dir
	cmd.Env = append(os.Environ(), "GOOS=darwin", "GOARCH="+arch, "CGO_ENABLED=0", "GOFLAGS=")
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return "", err
	}
	data, err := os.ReadFile(bin)
	if err != nil {
		return "", err
	}
	return macho.MinVersion(data)
}

func maxVersion(a, b string) string {
	pa, pb := strings.Split(a, "."), strings.Split(b, ".")
	for i := range max(len(pa), len(pb)) {
		var na, nb int
		if i < len(pa) {
			na, _ = strconv.Atoi(pa[i])
		}
		if i < len(pb) {
			nb, _ = strconv.Atoi(pb[i])
		}
		if na != nb {
			if na > nb {
				return a
			}
			return b
		}
	}
	return a
}

func infoPlist(c *config, minOS string) []byte {
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
	str("CFBundleDevelopmentRegion", "en")
	str("CFBundleDisplayName", c.name)
	str("CFBundleExecutable", c.exe)
	str("CFBundleIconFile", c.exe)
	str("CFBundleIdentifier", c.id)
	str("CFBundleInfoDictionaryVersion", "6.0")
	str("CFBundleName", c.name)
	str("CFBundlePackageType", "APPL")
	str("CFBundleShortVersionString", c.version)
	str("CFBundleVersion", strconv.Itoa(c.build))
	str("LSMinimumSystemVersion", minOS)
	str("NSPrincipalClass", "NSApplication")
	b.WriteString("\t<key>NSHighResolutionCapable</key>\n\t<true/>\n")
	if c.hasPermission("camera") {
		str("NSCameraUsageDescription", c.name+" uses the camera.")
	}
	if c.hasPermission("microphone") {
		str("NSMicrophoneUsageDescription", c.name+" uses the microphone.")
	}
	if c.hasPermission("location") {
		str("NSLocationUsageDescription", c.name+" uses your location.")
		str("NSLocationWhenInUseUsageDescription", c.name+" uses your location.")
	}
	b.WriteString("</dict>\n</plist>\n")
	return b.Bytes()
}

func entitlementsPlist(c *config) []byte {
	var keys []string
	if c.hasPermission("camera") {
		keys = append(keys, "com.apple.security.device.camera")
	}
	if c.hasPermission("microphone") {
		keys = append(keys, "com.apple.security.device.audio-input")
	}
	if c.hasPermission("location") {
		keys = append(keys, "com.apple.security.personal-information.location")
	}
	if len(keys) == 0 {
		return nil
	}

	var b bytes.Buffer
	b.WriteString(`<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
`)
	for _, k := range keys {
		b.WriteString("\t<key>" + k + "</key>\n\t<true/>\n")
	}
	b.WriteString("</dict>\n</plist>\n")
	return b.Bytes()
}

func signDarwin(c *config, app, tmp string) error {
	notarize := c.notaryKey != ""
	if notarize && (c.sign == "" || c.notaryIssue == "") {
		return errors.New("notarization needs --sign, --notary-key and --notary-issuer")
	}

	var entitlements string
	entData := entitlementsPlist(c)
	if entData != nil {
		entitlements = filepath.Join(tmp, "entitlements.plist")
		if err := os.WriteFile(entitlements, entData, 0o644); err != nil {
			return err
		}
	}

	if c.signer == "codesign" {
		return codesign(c, app, tmp, entitlements, notarize)
	}
	signer, err := appleSigner(c)
	if err != nil {
		return err
	}
	cdhash, err := apple.SignBundle(app, apple.BundleOptions{
		MacOS: true, Signer: signer, Entitlements: entData, HardenedRuntime: signer.Key != nil,
	})
	if err != nil {
		return err
	}
	if !notarize {
		return nil
	}
	return notarizeDarwin(c, app, tmp, cdhash)
}

func notarizeDarwin(c *config, app, tmp string, cdhash []byte) error {
	key, err := apple.LoadNotaryKey(c.notaryKey, c.notaryID, c.notaryIssue)
	if err != nil {
		return err
	}
	archive := filepath.Join(tmp, "notarize.zip")
	if err := zipDir(archive, app); err != nil {
		return err
	}
	data, err := os.ReadFile(archive)
	if err != nil {
		return err
	}
	if _, err := apple.Notarize(key, filepath.Base(app)+".zip", data, os.Stderr); err != nil {
		return err
	}
	fmt.Fprintln(os.Stderr, "notary: accepted, stapling")
	return apple.Staple(app, cdhash)
}

func codesign(c *config, app, tmp, entitlements string, notarize bool) error {
	identity := c.sign
	if identity == "" {
		identity = "-"
	}
	args := []string{"--force", "--sign", identity}
	if c.sign != "" {
		args = append(args, "--timestamp", "--options", "runtime")
	}

	libs, _ := filepath.Glob(filepath.Join(app, "Contents", "Frameworks", "*.dylib"))
	for _, lib := range libs {
		if err := run("codesign", append(slices.Clone(args), lib)...); err != nil {
			return err
		}
	}
	if entitlements != "" {
		args = append(args, "--entitlements", entitlements)
	}
	if err := run("codesign", append(args, app)...); err != nil {
		return err
	}

	if !notarize {
		return nil
	}
	archive := filepath.Join(tmp, c.exe+".zip")
	if err := run("ditto", "-c", "-k", "--keepParent", app, archive); err != nil {
		return err
	}
	if err := run("xcrun", "notarytool", "submit", archive, "--key", c.notaryKey, "--key-id", c.notaryID, "--issuer", c.notaryIssue, "--wait"); err != nil {
		return err
	}
	return run("xcrun", "stapler", "staple", app)
}

func zipDir(archive, dir string) error {
	f, err := os.Create(archive)
	if err != nil {
		return err
	}
	defer f.Close()

	zw := zip.NewWriter(f)
	base := filepath.Dir(dir)
	err = filepath.WalkDir(dir, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		info, err := d.Info()
		if err != nil {
			return err
		}
		rel, err := filepath.Rel(base, path)
		if err != nil {
			return err
		}
		hdr, err := zip.FileInfoHeader(info)
		if err != nil {
			return err
		}
		hdr.Name = filepath.ToSlash(rel)
		if d.IsDir() {
			hdr.Name += "/"
			_, err = zw.CreateHeader(hdr)
			return err
		}
		hdr.Method = zip.Deflate
		w, err := zw.CreateHeader(hdr)
		if err != nil {
			return err
		}
		src, err := os.Open(path)
		if err != nil {
			return err
		}
		defer src.Close()
		_, err = io.Copy(w, src)
		return err
	})
	if err != nil {
		return err
	}
	if err := zw.Close(); err != nil {
		return err
	}
	return f.Close()
}
