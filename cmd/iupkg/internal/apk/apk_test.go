package apk

import (
	"archive/zip"
	"bytes"
	"encoding/binary"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"slices"
	"testing"
)

func templateAPK(t *testing.T) string {
	_, file, _, _ := runtime.Caller(0)
	path := filepath.Join(filepath.Dir(file), "..", "..", "..", "..", "iup", "libs", "android", "template.apk")
	if _, err := os.Stat(path); err != nil {
		t.Skip(err)
	}
	return path
}

func TestPatchManifest(t *testing.T) {
	entries, err := ReadTemplate(templateAPK(t))
	if err != nil {
		t.Fatal(err)
	}
	m := Find(entries, "AndroidManifest.xml")
	data, err := m.Content()
	if err != nil {
		t.Fatal(err)
	}
	out, err := PatchManifest(data, ManifestValues{
		Package: "org.example.demo", Label: "Demo App", Version: "2.0", Build: 9, Library: "libdemo.so",
		Permissions: []string{"android.permission.CAMERA"},
	})
	if err != nil {
		t.Fatal(err)
	}

	a, err := parseAXML(out)
	if err != nil {
		t.Fatal(err)
	}
	get := func(elem string, res uint32, plain string) string {
		for _, c := range a.startElements(elem) {
			if at := a.findAttr(c, res, plain); at != nil {
				s, _ := a.attrString(at)
				return s
			}
		}
		return ""
	}
	if got := get("manifest", 0, "package"); got != "org.example.demo" {
		t.Errorf("package = %q", got)
	}
	if got := get("manifest", attrVersionName, ""); got != "2.0" {
		t.Errorf("versionName = %q", got)
	}
	if got := get("application", attrLabel, ""); got != "Demo App" {
		t.Errorf("label = %q", got)
	}
	var perms, authorities []string
	for _, c := range a.startElements("uses-permission") {
		s, _ := a.attrString(a.findAttr(c, attrName, ""))
		perms = append(perms, s)
	}
	for _, c := range a.startElements("provider") {
		s, _ := a.attrString(a.findAttr(c, 0x01010018, ""))
		authorities = append(authorities, s)
	}
	slices.Sort(perms)
	want := []string{"android.permission.CAMERA", "org.example.demo.DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION"}
	if !slices.Equal(perms, want) {
		t.Errorf("permissions = %q", perms)
	}
	for _, s := range authorities {
		if !bytes.HasPrefix([]byte(s), []byte("org.example.demo.")) {
			t.Errorf("authority %q not renamed", s)
		}
	}
	if at := a.findAttr(a.startElements("manifest")[0], attrVersionCode, ""); at == nil || binary.LittleEndian.Uint32(at[16:]) != 9 {
		t.Error("versionCode not 9")
	}
	if !bytes.Contains(out, []byte{'l', 0, 'i', 0, 'b', 0, 'd', 0, 'e', 0, 'm', 0, 'o', 0}) {
		t.Error("library name missing")
	}
	if _, err := PatchManifest(out, ManifestValues{Package: "a.b", Label: "x", Version: "1", Library: "libx.so"}); err != nil {
		t.Errorf("repatch: %v", err)
	}
}

func TestLauncherIconPaths(t *testing.T) {
	entries, err := ReadTemplate(templateAPK(t))
	if err != nil {
		t.Fatal(err)
	}
	table, err := Find(entries, "resources.arsc").Content()
	if err != nil {
		t.Fatal(err)
	}
	paths, err := LauncherIconPaths(table, "ic_launcher")
	if err != nil {
		t.Fatal(err)
	}
	if len(paths) != 5 {
		t.Fatalf("paths = %v, want 5", paths)
	}
	for _, p := range paths {
		if Find(entries, p) == nil {
			t.Errorf("%s not in the template", p)
		}
	}
}

func TestWriteZipAlignment(t *testing.T) {
	stored, _ := NewEntry("a/stored.bin", bytes.Repeat([]byte{1}, 1000), 0, Align)
	lib, _ := NewEntry("lib/arm64-v8a/libx.so", bytes.Repeat([]byte{2}, 5000), 0, AlignLib)
	deflated, _ := NewEntry("text.txt", bytes.Repeat([]byte("hello "), 200), 8, 0)
	data, err := WriteZip([]*Entry{deflated, stored, lib})
	if err != nil {
		t.Fatal(err)
	}
	zr, err := zip.NewReader(bytes.NewReader(data), int64(len(data)))
	if err != nil {
		t.Fatal(err)
	}
	for _, f := range zr.File {
		off, err := f.DataOffset()
		if err != nil {
			t.Fatal(err)
		}
		align := map[string]int64{"a/stored.bin": Align, "lib/arm64-v8a/libx.so": AlignLib, "text.txt": 1}[f.Name]
		if off%align != 0 {
			t.Errorf("%s data offset %d not aligned to %d", f.Name, off, align)
		}
		r, _ := f.Open()
		got, _ := io.ReadAll(r)
		if int64(len(got)) != int64(f.UncompressedSize64) {
			t.Errorf("%s size mismatch", f.Name)
		}
	}
}

func TestSignAPK(t *testing.T) {
	signer, err := DebugIdentity()
	if err != nil {
		t.Fatal(err)
	}
	entries, err := ReadTemplate(templateAPK(t))
	if err != nil {
		t.Fatal(err)
	}
	lib, _ := NewEntry("lib/arm64-v8a/libx.so", bytes.Repeat([]byte{7}, 70000), 0, AlignLib)
	entries, err = SignV1(append(entries, lib), signer)
	if err != nil {
		t.Fatal(err)
	}
	apk, err := WriteZip(entries)
	if err != nil {
		t.Fatal(err)
	}
	apk, err = SignV2(apk, signer)
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Contains(apk, []byte("APK Sig Block 42")) {
		t.Fatal("no signing block")
	}
	if _, err := zip.NewReader(bytes.NewReader(apk), int64(len(apk))); err != nil {
		t.Fatalf("signed apk unreadable: %v", err)
	}

	tools, _ := filepath.Glob(filepath.Join(os.Getenv("ANDROID_HOME"), "build-tools", "*", "apksigner"))
	if len(tools) == 0 {
		t.Skip("apksigner not found")
	}
	path := filepath.Join(t.TempDir(), "t.apk")
	os.WriteFile(path, apk, 0o644)
	out, err := exec.Command(tools[len(tools)-1], "verify", "-v", path).CombinedOutput()
	if err != nil {
		t.Fatalf("apksigner: %v\n%s", err, out)
	}
	for _, want := range []string{"v1 scheme (JAR signing): true", "v2 scheme (APK Signature Scheme v2): true"} {
		if !bytes.Contains(out, []byte(want)) {
			t.Errorf("apksigner output lacks %q:\n%s", want, out)
		}
	}
}
