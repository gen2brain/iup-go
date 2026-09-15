package main

import (
	"archive/zip"
	"bytes"
	"errors"
	"fmt"
	"image/png"
	"os"
	"path/filepath"
	"runtime"
	"slices"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/apk"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/icon"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/keys"
)

type androidABI struct {
	abi    string
	triple string
}

var androidABIs = map[string]androidABI{
	"arm64": {"arm64-v8a", "aarch64-linux-android"},
	"arm":   {"armeabi-v7a", "armv7a-linux-androideabi"},
	"amd64": {"x86_64", "x86_64-linux-android"},
	"386":   {"x86", "i686-linux-android"},
}

var androidPermissions = map[string][]string{
	"camera":        {"android.permission.CAMERA"},
	"microphone":    {"android.permission.RECORD_AUDIO"},
	"location":      {"android.permission.ACCESS_COARSE_LOCATION", "android.permission.ACCESS_FINE_LOCATION"},
	"notifications": {"android.permission.POST_NOTIFICATIONS"},
	"internet":      {"android.permission.INTERNET"},
}

const androidAPI = "22"

func packageAndroid(c *config) error {
	var archs []string
	for _, a := range splitList(c.goarch) {
		if _, ok := androidABIs[a]; !ok {
			return fmt.Errorf("unknown android arch %q (arm64, arm, amd64, 386)", a)
		}
		archs = append(archs, a)
	}

	toolchain, err := ndkToolchain()
	if err != nil {
		return err
	}

	template := filepath.Join(c.iupDir, "libs", "android", "template.apk")
	entries, err := apk.ReadTemplate(template)
	if err != nil {
		return fmt.Errorf("%s: %w", template, err)
	}

	manifest := apk.Find(entries, "AndroidManifest.xml")
	arsc := apk.Find(entries, "resources.arsc")
	if manifest == nil || arsc == nil {
		return fmt.Errorf("%s: not an APK template", template)
	}

	var perms []string
	for _, p := range c.permissions {
		perms = append(perms, androidPermissions[p]...)
	}
	lib := "lib" + c.exe + ".so"
	data, err := manifest.Content()
	if err != nil {
		return err
	}
	data, err = apk.PatchManifest(data, apk.ManifestValues{
		Package: c.id, Label: c.name, Version: c.version, Build: c.build, Library: lib, Permissions: perms,
	})
	if err != nil {
		return fmt.Errorf("%s: %w", template, err)
	}
	if err := manifest.SetContent(data); err != nil {
		return err
	}

	if err := replaceLauncherIcons(c, entries, arsc); err != nil {
		return err
	}

	tmp, err := os.MkdirTemp("", "iupkg-")
	if err != nil {
		return err
	}
	defer os.RemoveAll(tmp)

	for _, arch := range archs {
		abi := androidABIs[arch]
		out := filepath.Join(tmp, abi.abi, lib)
		if err := os.MkdirAll(filepath.Dir(out), 0o755); err != nil {
			return err
		}
		clang := filepath.Join(toolchain, abi.triple+androidAPI+"-clang")
		env := []string{
			"GOOS=android", "GOARCH=" + arch, "CGO_ENABLED=1",
			"CC=" + clang, "CXX=" + clang + "++", "PKG_CONFIG=true",
		}
		ldflags := []string{"-extldflags=-Wl,-soname," + lib}
		if err := goBuildWith(c, env, out, nil, ldflags, []string{"-buildmode=c-shared"}); err != nil {
			return err
		}
		so, err := os.ReadFile(out)
		if err != nil {
			return err
		}
		e, err := apk.NewEntry("lib/"+abi.abi+"/"+lib, so, zip.Store, apk.AlignLib)
		if err != nil {
			return err
		}
		entries = append(entries, e)
	}

	var id *keys.Identity
	if c.sign == "" {
		id, err = apk.DebugIdentity()
	} else {
		id, err = c.identity()
	}
	if err != nil {
		return err
	}
	entries, err = apk.SignV1(entries, id)
	if err != nil {
		return err
	}
	data, err = apk.WriteZip(entries)
	if err != nil {
		return err
	}
	data, err = apk.SignV2(data, id)
	if err != nil {
		return err
	}

	out := filepath.Join(c.out, c.exe+".apk")
	if err := os.WriteFile(out, data, 0o644); err != nil {
		return err
	}
	fmt.Fprintln(os.Stderr, out)

	if c.install {
		return run("adb", "install", "-r", out)
	}
	return nil
}

func replaceLauncherIcons(c *config, entries []*apk.Entry, arsc *apk.Entry) error {
	table, err := arsc.Content()
	if err != nil {
		return err
	}
	paths, err := apk.LauncherIconPaths(table, "ic_launcher")
	if err != nil {
		return err
	}
	img, err := icon.Load(c.icon)
	if err != nil {
		return err
	}
	for _, p := range paths {
		e := apk.Find(entries, p)
		if e == nil {
			return fmt.Errorf("template icon %s missing", p)
		}
		old, err := e.Content()
		if err != nil {
			return err
		}
		cfg, err := png.DecodeConfig(bytes.NewReader(old))
		if err != nil {
			return fmt.Errorf("template icon %s: %w", p, err)
		}
		data, err := icon.PNG(icon.Resize(img, cfg.Width))
		if err != nil {
			return err
		}
		if err := e.SetContent(data); err != nil {
			return err
		}
	}
	return nil
}

func ndkToolchain() (string, error) {
	var roots []string
	for _, v := range []string{"ANDROID_NDK_HOME", "ANDROID_NDK_ROOT"} {
		if d := os.Getenv(v); d != "" {
			roots = append(roots, d)
		}
	}
	for _, v := range []string{"ANDROID_SDK_ROOT", "ANDROID_HOME"} {
		if d := os.Getenv(v); d != "" {
			versions, _ := filepath.Glob(filepath.Join(d, "ndk", "*"))
			slices.Sort(versions)
			slices.Reverse(versions)
			roots = append(roots, versions...)
		}
	}

	host := runtime.GOOS + "-x86_64"
	for _, root := range roots {
		dir := filepath.Join(root, "toolchains", "llvm", "prebuilt", host, "bin")
		if _, err := os.Stat(filepath.Join(dir, "clang"+exeSuffix())); err == nil {
			return dir, nil
		}
	}
	return "", errors.New("no Android NDK found; set ANDROID_NDK_HOME")
}

func exeSuffix() string {
	if runtime.GOOS == "windows" {
		return ".exe"
	}
	return ""
}
