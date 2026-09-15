package main

import (
	"bytes"
	"testing"
)

func TestWindowsVersion(t *testing.T) {
	tests := []struct {
		version string
		build   int
		want    string
	}{
		{"1.0.0", 1, "1.0.0.1"},
		{"2.5", 7, "2.5.0.7"},
		{"3", 0, "3.0.0.0"},
		{"1.2.3.4", 9, "1.2.3.9"},
		{"1.2-beta", 3, "1.0.0.3"},
	}
	for _, tt := range tests {
		if got := windowsVersion(tt.version, tt.build); got != tt.want {
			t.Errorf("windowsVersion(%q, %d) = %q, want %q", tt.version, tt.build, got, tt.want)
		}
	}
}

func TestIdentifierPart(t *testing.T) {
	tests := map[string]string{
		"button":   "button",
		"My-App_2": "myapp2",
		"2048":     "app2048",
		"---":      "app",
	}
	for in, want := range tests {
		if got := identifierPart(in); got != want {
			t.Errorf("identifierPart(%q) = %q, want %q", in, got, want)
		}
	}
}

func TestInfoPlistEscapesAndPermissions(t *testing.T) {
	c := &config{name: "A & B", exe: "ab", id: "com.example.ab", version: "1.0", build: 2, permissions: []string{"camera"}}
	plist := string(infoPlist(c, "11.0"))
	for _, want := range []string{"<string>A &amp; B</string>", "<key>NSCameraUsageDescription</key>", "<string>11.0</string>"} {
		if !bytes.Contains([]byte(plist), []byte(want)) {
			t.Errorf("plist missing %q", want)
		}
	}
	if bytes.Contains([]byte(plist), []byte("NSMicrophoneUsageDescription")) {
		t.Error("plist has microphone usage without the permission")
	}
	if ent := entitlementsPlist(&config{}); ent != nil {
		t.Error("entitlements without permissions")
	}
}

func TestMaxVersion(t *testing.T) {
	tests := [][3]string{
		{"11.0", "13.0", "13.0"},
		{"13.0", "11.0", "13.0"},
		{"10.15", "10.9", "10.15"},
		{"12", "12.1", "12.1"},
	}
	for _, tt := range tests {
		if got := maxVersion(tt[0], tt[1]); got != tt[2] {
			t.Errorf("maxVersion(%q, %q) = %q, want %q", tt[0], tt[1], got, tt[2])
		}
	}
}

func TestProfileEntitlementsPlist(t *testing.T) {
	profile := []byte("garbage<plist><dict><key>Entitlements</key>\n<dict>\n\t<key>application-identifier</key>\n\t<string>ABC123.*</string>\n\t<key>get-task-allow</key>\n\t<true/>\n</dict><key>Name</key><string>x</string></dict></plist>trailer")
	out, err := profileEntitlementsPlist(profile, "com.example.app")
	if err != nil {
		t.Fatal(err)
	}
	for _, want := range []string{"<string>ABC123.com.example.app</string>", "<key>get-task-allow</key>", "<plist version=\"1.0\">"} {
		if !bytes.Contains(out, []byte(want)) {
			t.Errorf("missing %q in:\n%s", want, out)
		}
	}
	if bytes.Contains(out, []byte("<key>Name</key>")) {
		t.Error("copied more than the Entitlements dict")
	}
	if _, err := profileEntitlementsPlist([]byte("nothing"), "a"); err == nil {
		t.Error("expected an error without Entitlements")
	}
}
