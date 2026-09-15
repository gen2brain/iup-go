package deb

import (
	"bytes"
	"os"
	"os/exec"
	"path/filepath"
	"testing"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/pkgtree"
)

func TestWrite(t *testing.T) {
	data, err := Write(Package{
		Name: "demo", Version: "1.0-1", Arch: "amd64", Maintainer: "Demo <demo@example.com>", Description: "Demo app", Section: "misc",
		Files: []pkgtree.File{
			{Path: "/usr/bin/demo", Mode: 0o755, Data: []byte("#!/bin/sh\necho demo\n")},
			{Path: "/usr/share/applications/demo.desktop", Mode: 0o644, Data: []byte("[Desktop Entry]\n")},
		},
	})
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.HasPrefix(data, []byte("!<arch>\ndebian-binary")) {
		t.Fatal("not an ar archive")
	}
	if _, err := exec.LookPath("dpkg-deb"); err != nil {
		t.Skip("dpkg-deb not found")
	}
	path := filepath.Join(t.TempDir(), "demo.deb")
	os.WriteFile(path, data, 0o644)
	out, err := exec.Command("dpkg-deb", "-I", path).CombinedOutput()
	if err != nil {
		t.Fatalf("dpkg-deb -I: %v\n%s", err, out)
	}
	for _, want := range []string{"Package: demo", "Version: 1.0-1", "Architecture: amd64"} {
		if !bytes.Contains(out, []byte(want)) {
			t.Errorf("missing %q in\n%s", want, out)
		}
	}
	out, err = exec.Command("dpkg-deb", "-c", path).CombinedOutput()
	if err != nil || !bytes.Contains(out, []byte("./usr/bin/demo")) || !bytes.Contains(out, []byte("root/root")) {
		t.Errorf("dpkg-deb -c: %v\n%s", err, out)
	}
}
