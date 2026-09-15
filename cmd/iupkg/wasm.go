package main

import (
	"fmt"
	"html"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

func packageJS(c *config) error {
	dir := filepath.Join(c.out, c.exe)
	if err := os.RemoveAll(dir); err != nil {
		return err
	}
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return err
	}

	env := []string{"GOOS=js", "GOARCH=wasm", "CGO_ENABLED=0"}
	if err := goBuildWith(c, env, filepath.Join(dir, "app.wasm"), nil, nil, nil); err != nil {
		return err
	}

	for _, name := range []string{"iup.js", "iup.wasm"} {
		src := filepath.Join(c.iupDir, "libs", "wasm", name+".gz")
		data, err := readGzip(src)
		if err != nil {
			return fmt.Errorf("%s: %w (run iup/libs/make.bash wasm)", src, err)
		}
		if err := os.WriteFile(filepath.Join(dir, name), data, 0o644); err != nil {
			return err
		}
	}

	web := filepath.Join(c.iupDir, "external", "wasm", "web")
	for _, name := range []string{"index.html", "worker.js", "iupwasm_dom.js"} {
		data, err := os.ReadFile(filepath.Join(web, name))
		if err != nil {
			return err
		}
		if name == "index.html" {
			data = []byte(strings.Replace(string(data), "<title>IUP WASM</title>", "<title>"+html.EscapeString(c.name)+"</title>", 1))
		}
		if err := os.WriteFile(filepath.Join(dir, name), data, 0o644); err != nil {
			return err
		}
	}

	goroot, err := exec.Command("go", "env", "GOROOT").Output()
	if err != nil {
		return err
	}
	root := strings.TrimSpace(string(goroot))
	execJS := filepath.Join(root, "lib", "wasm", "wasm_exec.js")
	if _, err := os.Stat(execJS); err != nil {
		execJS = filepath.Join(root, "misc", "wasm", "wasm_exec.js")
	}
	data, err := os.ReadFile(execJS)
	if err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(dir, "wasm_exec.js"), data, 0o644); err != nil {
		return err
	}

	fmt.Fprintln(os.Stderr, dir)
	return nil
}
