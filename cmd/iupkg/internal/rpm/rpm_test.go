package rpm

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
		Name: "demo", Version: "1.0", Release: "1", Arch: "x86_64", Summary: "Demo app", Description: "Demo app", License: "MIT", Vendor: "Demo",
		Files: []pkgtree.File{
			{Path: "/usr/bin/demo", Mode: 0o755, Data: []byte("#!/bin/sh\necho demo\n")},
			{Path: "/usr/share/applications/demo.desktop", Mode: 0o644, Data: []byte("[Desktop Entry]\n")},
			{Path: "/usr/bin/demo-link", Mode: 0o777, Link: "demo"},
		},
	})
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.HasPrefix(data, []byte{0xed, 0xab, 0xee, 0xdb}) {
		t.Fatal("bad lead")
	}
	if _, err := exec.LookPath("rpm"); err != nil {
		t.Skip("rpm not found")
	}
	path := filepath.Join(t.TempDir(), "demo.rpm")
	os.WriteFile(path, data, 0o644)
	out, err := exec.Command("rpm", "-qpi", path).CombinedOutput()
	if err != nil {
		t.Fatalf("rpm -qpi: %v\n%s", err, out)
	}
	for _, want := range []string{"Name        : demo", "Version     : 1.0", "Architecture: x86_64", "License     : MIT"} {
		if !bytes.Contains(out, []byte(want)) {
			t.Errorf("missing %q in\n%s", want, out)
		}
	}
	out, _ = exec.Command("rpm", "-K", "-v", path).CombinedOutput()
	if !bytes.Contains(out, []byte("Header SHA256 digest: OK")) || !bytes.Contains(out, []byte("Payload SHA256 digest: OK")) {
		t.Errorf("rpm -K:\n%s", out)
	}
	out, err = exec.Command("rpm", "-qpl", path).CombinedOutput()
	if err != nil || !bytes.Contains(out, []byte("/usr/bin/demo-link")) {
		t.Errorf("rpm -qpl: %v\n%s", err, out)
	}
}
