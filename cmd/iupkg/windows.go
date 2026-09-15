package main

import (
	"bytes"
	"fmt"
	"os"
	"os/signal"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/authenticode"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/icon"
	"github.com/tc-hib/winres"
	"github.com/tc-hib/winres/version"
)

func packageWindows(c *config) error {
	existing, err := filepath.Glob(filepath.Join(c.pkgDir, "*.syso"))
	if err != nil {
		return err
	}
	if len(existing) > 0 {
		return fmt.Errorf("%s already has resources (%s); remove them, iupkg writes its own", c.pkg, filepath.Base(existing[0]))
	}

	img, err := icon.Load(c.icon)
	if err != nil {
		return err
	}
	ico, err := winres.NewIconFromResizedImage(img, []int{256, 64, 48, 40, 32, 24, 20, 16})
	if err != nil {
		return err
	}
	manifest, err := os.ReadFile(filepath.Join(c.iupDir, "external", "iup.manifest"))
	if err != nil {
		return err
	}

	rs := &winres.ResourceSet{}
	if err := rs.SetIcon(winres.Name("APP"), ico); err != nil {
		return err
	}
	rs.SetVersionInfo(versionInfo(c))
	if err := rs.Set(winres.RT_MANIFEST, winres.ID(1), winres.LCIDDefault, manifest); err != nil {
		return err
	}

	var obj bytes.Buffer
	if err := rs.WriteObject(&obj, winres.Arch(c.goarch)); err != nil {
		return err
	}
	syso := filepath.Join(c.pkgDir, "iupkg_windows_"+c.goarch+".syso")
	if err := os.WriteFile(syso, obj.Bytes(), 0o644); err != nil {
		return err
	}
	defer os.Remove(syso)

	stop := make(chan os.Signal, 1)
	signal.Notify(stop, os.Interrupt)
	defer signal.Stop(stop)
	go func() {
		if _, ok := <-stop; ok {
			os.Remove(syso)
			os.Exit(130)
		}
	}()

	var ldflags []string
	if !c.console {
		ldflags = append(ldflags, "-H=windowsgui")
	}
	out := filepath.Join(c.out, c.exe+".exe")
	if err := goBuild(c, c.goarch, out, []string{"nomanifest"}, ldflags); err != nil {
		return err
	}

	if c.sign != "" {
		if err := signWindows(c, out); err != nil {
			return err
		}
	}
	fmt.Fprintln(os.Stderr, out)
	return nil
}

func signWindows(c *config, exe string) error {
	id, err := c.identity()
	if err != nil {
		return err
	}
	signer := &authenticode.Signer{Key: id.Key, Chain: id.Certificates()}
	if c.timestamp {
		signer.TimestampURL = c.tsaURL
	}
	data, err := os.ReadFile(exe)
	if err != nil {
		return err
	}
	signed, err := authenticode.Sign(data, signer)
	if err != nil {
		return err
	}
	return os.WriteFile(exe, signed, 0o755)
}

func versionInfo(c *config) version.Info {
	fileVersion := windowsVersion(c.version, c.build)

	vi := version.Info{}
	vi.Set(version.LangDefault, version.ProductName, c.name)
	vi.Set(version.LangDefault, version.FileDescription, c.name)
	vi.Set(version.LangDefault, version.InternalName, c.exe)
	vi.Set(version.LangDefault, version.OriginalFilename, c.exe+".exe")
	vi.Set(version.LangDefault, version.CompanyName, c.vendor)
	if c.copyright != "" {
		vi.Set(version.LangDefault, version.LegalCopyright, c.copyright)
	}
	vi.SetFileVersion(fileVersion)
	vi.SetProductVersion(fileVersion)
	vi.Set(version.LangDefault, version.ProductVersion, c.version)
	return vi
}

func windowsVersion(v string, build int) string {
	parts := make([]string, 0, 4)
	for f := range strings.SplitSeq(v, ".") {
		if len(parts) == 3 {
			break
		}
		n, err := strconv.Atoi(f)
		if err != nil {
			break
		}
		parts = append(parts, strconv.Itoa(n))
	}
	for len(parts) < 3 {
		parts = append(parts, "0")
	}
	return strings.Join(append(parts, strconv.Itoa(build)), ".")
}
