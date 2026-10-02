package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/hpkg"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/icon"
)

func packageHaiku(c *config) error {
	if c.goarch != "amd64" {
		return fmt.Errorf("unsupported haiku arch %q (amd64)", c.goarch)
	}
	version, err := hpkg.ParseVersion(c.version)
	if err != nil {
		return err
	}
	if c.build < 1 {
		return fmt.Errorf("--build must be at least 1 for haiku")
	}

	env := []string{"GOOS=haiku", "GOARCH=amd64", "CGO_ENABLED=1"}
	goCmd := "go"
	if v := os.Getenv("GOHAIKU"); v != "" {
		goCmd = v
	}
	if runtime.GOOS != "haiku" {
		if os.Getenv("CC") == "" {
			if _, err := exec.LookPath("haiku-x86_64-cc"); err != nil {
				return fmt.Errorf("haiku-x86_64-cc not found; set CC to the Haiku cross compiler")
			}
			env = append(env, "CC=haiku-x86_64-cc", "CXX=haiku-x86_64-cxx")
		}
	}

	tmp, err := os.MkdirTemp("", "iupkg-")
	if err != nil {
		return err
	}
	defer os.RemoveAll(tmp)

	bin := filepath.Join(tmp, c.exe)
	if err := goBuildCmd(c, goCmd, env, bin, nil, nil, nil); err != nil {
		return err
	}
	exe, err := os.ReadFile(bin)
	if err != nil {
		return err
	}
	img, err := icon.Load(c.icon)
	if err != nil {
		return err
	}

	sig := "application/x-vnd.iup-Application"
	if c.idSet {
		sig = c.id
	}
	copyright := c.copyright
	if copyright == "" {
		copyright = c.name
	}
	name := hpkg.PackageName(c.exe)
	info := hpkg.Info{
		Name: name, Version: version, Revision: uint32(c.build),
		Summary: c.name, Description: c.name, Vendor: c.vendor, Packager: "iupkg",
		Copyright: copyright, License: c.license,
		Provides: []string{name, "app:" + name},
		Requires: []string{"haiku", "lib:libstdc++"},
	}

	root := &hpkg.Entry{Dir: true}
	root.Add(&hpkg.Entry{Name: ".PackageInfo", Data: hpkg.InfoText(info, "x86_64")})
	apps := root.Add(&hpkg.Entry{Name: "apps", Dir: true})
	app := apps.Add(&hpkg.Entry{Name: c.name, Dir: true})
	app.Add(&hpkg.Entry{Name: c.exe, Mode: 0o755, Data: exe, Attrs: []hpkg.FileAttr{
		{Name: "BEOS:APP_SIG", Type: hpkg.MimeString, Data: append([]byte(sig), 0)},
		{Name: "BEOS:TYPE", Type: hpkg.MimeString, Data: append([]byte("application/x-vnd.Be-elfexecutable"), 0)},
		{Name: "BEOS:L:STD_ICON", Type: hpkg.LargeIconType, Data: hpkg.BitmapIcon(img, 32)},
		{Name: "BEOS:M:STD_ICON", Type: hpkg.MiniIconType, Data: hpkg.BitmapIcon(img, 16)},
	}})
	dataDir := root.Add(&hpkg.Entry{Name: "data", Dir: true})
	if len(c.files) > 0 {
		if strings.EqualFold(c.exe, "deskbar") {
			return fmt.Errorf("--data: data/%s is reserved in the package", c.exe)
		}
		res := dataDir.Add(&hpkg.Entry{Name: c.exe, Dir: true})
		for _, f := range c.files {
			dir := res
			parts := strings.Split(f.name, "/")
			for _, part := range parts[:len(parts)-1] {
				var next *hpkg.Entry
				for _, child := range dir.Children {
					if child.Dir && child.Name == part {
						next = child
					}
				}
				if next == nil {
					next = dir.Add(&hpkg.Entry{Name: part, Dir: true})
				}
				dir = next
			}
			dir.Add(&hpkg.Entry{Name: parts[len(parts)-1], Mode: uint32(f.mode), Data: f.data})
		}
	}
	menu := dataDir.Add(&hpkg.Entry{Name: "deskbar", Dir: true}).
		Add(&hpkg.Entry{Name: "menu", Dir: true}).
		Add(&hpkg.Entry{Name: "Applications", Dir: true})
	menu.Add(&hpkg.Entry{Name: c.name, Link: "../../../../apps/" + c.name + "/" + c.exe})

	data, err := hpkg.Write(root, info, hpkg.ArchX86_64)
	if err != nil {
		return err
	}
	out := filepath.Join(c.out, fmt.Sprintf("%s-%s-%d-x86_64.hpkg", name, hpkg.VersionString(version), c.build))
	if err := os.WriteFile(out, data, 0o644); err != nil {
		return err
	}
	fmt.Fprintln(os.Stderr, out)
	return nil
}
