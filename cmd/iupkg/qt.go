package main

import (
	"bytes"
	"debug/macho"
	"debug/pe"
	"encoding/json"
	"errors"
	"fmt"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"slices"
	"sort"
	"strings"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/apple"
)

type qtInfo struct {
	bins     string
	libexecs string
	qml      string
}

func (c *config) qtBundle() bool {
	return c.cgo && (slices.Contains(c.tags, "qt") || slices.Contains(c.tags, "qml"))
}

func (c *config) qtPaths() (*qtInfo, error) {
	major := "6."
	tools := [][2]string{{"qtpaths6", "--query"}, {"qtpaths", "--query"}, {"qmake6", "-query"}, {"qmake", "-query"}}
	if slices.Contains(c.tags, "qt5") {
		major = "5."
		tools = [][2]string{{"qtpaths-qt5", "--query"}, {"qmake-qt5", "-query"}, {"qmake5", "-query"}, {"qtpaths", "--query"}, {"qmake", "-query"}}
	}
	for _, t := range tools {
		tool, err := exec.LookPath(t[0])
		if err != nil {
			continue
		}
		query := func(key string) string {
			out, err := exec.Command(tool, t[1], key).Output()
			if err != nil {
				return ""
			}
			return strings.TrimSpace(string(out))
		}
		if !strings.HasPrefix(query("QT_VERSION"), major) {
			continue
		}
		info := &qtInfo{
			bins:     filepath.FromSlash(query("QT_INSTALL_BINS")),
			libexecs: filepath.FromSlash(query("QT_INSTALL_LIBEXECS")),
			qml:      filepath.FromSlash(query("QT_INSTALL_QML")),
		}
		if info.bins != "" {
			return info, nil
		}
	}
	return nil, fmt.Errorf("qt: no Qt %sx qtpaths or qmake found; package Qt builds where the Qt they link against is installed", major)
}

func (q *qtInfo) tool(names ...string) (string, error) {
	for _, name := range names {
		if runtime.GOOS == "windows" {
			name += ".exe"
		}
		for _, dir := range []string{q.bins, q.libexecs} {
			path := filepath.Join(dir, name)
			if _, err := os.Stat(path); err == nil {
				return path, nil
			}
		}
	}
	return "", fmt.Errorf("qt: %s not found in %s", names[0], q.bins)
}

var qmlImport = regexp.MustCompile(`import (Qt[A-Za-z0-9.]*)`)

func (c *config) qmlStub(dir string) error {
	srcs, err := filepath.Glob(filepath.Join(c.iupDir, "external", "src", "qml", "*.cpp"))
	if err != nil {
		return err
	}
	var imports []string
	for _, src := range srcs {
		data, err := os.ReadFile(src)
		if err != nil {
			return err
		}
		for _, m := range qmlImport.FindAllSubmatch(data, -1) {
			if name := string(m[1]); !slices.Contains(imports, name) {
				imports = append(imports, name)
			}
		}
	}
	if len(imports) == 0 {
		return errors.New("qml: no imports found in the driver sources")
	}
	sort.Strings(imports)
	var b strings.Builder
	for _, name := range imports {
		fmt.Fprintf(&b, "import %s\n", name)
	}
	b.WriteString("Item {}\n")
	if err := os.MkdirAll(dir, 0o755); err != nil {
		return err
	}
	return os.WriteFile(filepath.Join(dir, "iup.qml"), []byte(b.String()), 0o644)
}

func (q *qtInfo) qmlModules(stub string) (map[string]bool, error) {
	scanner, err := q.tool("qmlimportscanner")
	if err != nil {
		return nil, err
	}
	out, err := exec.Command(scanner, "-rootPath", stub, "-importPath", q.qml).Output()
	if err != nil {
		return nil, fmt.Errorf("qmlimportscanner: %w", err)
	}
	var entries []struct {
		Type         string `json:"type"`
		RelativePath string `json:"relativePath"`
	}
	if err := json.Unmarshal(out, &entries); err != nil {
		return nil, fmt.Errorf("qmlimportscanner: %w", err)
	}
	modules := map[string]bool{}
	for _, e := range entries {
		if e.Type == "module" && e.RelativePath != "" {
			modules[filepath.FromSlash(e.RelativePath)] = true
		}
	}
	return modules, nil
}

func deployQtDarwin(c *config, app, tmp string) error {
	info, err := c.qtPaths()
	if err != nil {
		return err
	}
	tool, err := info.tool("macdeployqt6", "macdeployqt")
	if err != nil {
		return err
	}
	args := []string{app, "-always-overwrite"}
	stub := filepath.Join(tmp, "qmlstub")
	qml := slices.Contains(c.tags, "qml")
	if qml {
		if err := c.qmlStub(stub); err != nil {
			return err
		}
		args = append(args, "-qmldir="+stub)
	}
	if err := run(tool, args...); err != nil {
		return err
	}
	contents := filepath.Join(app, "Contents")
	if err := rpathQtDarwin(contents); err != nil {
		return err
	}
	if qml {
		modules, err := info.qmlModules(stub)
		if err != nil {
			return err
		}
		if err := trimQML(contents, modules); err != nil {
			return err
		}
	}
	return pruneMachO(contents)
}

func trimQML(contents string, modules map[string]bool) error {
	root := filepath.Join(contents, "Resources", "qml")
	err := filepath.WalkDir(root, func(path string, d fs.DirEntry, err error) error {
		if err != nil || !d.IsDir() || path == root {
			return err
		}
		if _, err := os.Stat(filepath.Join(path, "qmldir")); err != nil {
			return nil
		}
		rel, _ := filepath.Rel(root, path)
		if modules[rel] {
			return nil
		}
		for m := range modules {
			if strings.HasPrefix(m, rel+string(filepath.Separator)) {
				return nil
			}
		}
		if err := os.RemoveAll(path); err != nil {
			return err
		}
		return filepath.SkipDir
	})
	if err != nil {
		return err
	}

	used := map[string]bool{}
	err = filepath.WalkDir(root, func(path string, d fs.DirEntry, err error) error {
		if err != nil || d.Type()&fs.ModeSymlink == 0 {
			return err
		}
		target, err := filepath.EvalSymlinks(path)
		if err == nil {
			used[target] = true
		}
		return nil
	})
	if err != nil {
		return err
	}
	plugins, _ := filepath.Glob(filepath.Join(contents, "PlugIns", "quick", "*"))
	for _, plugin := range plugins {
		target, err := filepath.EvalSymlinks(plugin)
		if err != nil {
			return err
		}
		if !used[target] {
			if err := os.Remove(plugin); err != nil {
				return err
			}
		}
	}
	return nil
}

func machODylibs(path string) ([]string, error) {
	return machOStrings(path, 0xc, 0x20, 0x80000018, 0x8000001f, 0x80000023)
}

func machOStrings(path string, cmds ...uint32) ([]string, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	var mf *macho.File
	if fat, err := macho.NewFatFile(f); err == nil {
		defer fat.Close()
		mf = fat.Arches[0].File
	} else if mf, err = macho.NewFile(f); err != nil {
		return nil, nil
	} else {
		defer mf.Close()
	}
	var names []string
	for _, load := range mf.Loads {
		raw := load.Raw()
		if len(raw) < 12 {
			continue
		}
		if slices.Contains(cmds, mf.ByteOrder.Uint32(raw)) {
			off := mf.ByteOrder.Uint32(raw[8:])
			if int(off) < len(raw) {
				names = append(names, string(bytes.TrimRight(raw[off:], "\x00")))
			}
		}
	}
	return names, nil
}

func isMachO(path string) bool {
	f, err := os.Open(path)
	if err != nil {
		return false
	}
	defer f.Close()
	var magic [4]byte
	_, err = f.Read(magic[:])
	return err == nil && apple.IsMachO(magic[:])
}

func machOFiles(dir string) ([]string, error) {
	var files []string
	err := filepath.WalkDir(dir, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.Type().IsRegular() && isMachO(path) {
			files = append(files, path)
		}
		return nil
	})
	return files, err
}

func rpathQtDarwin(contents string) error {
	frameworks := filepath.Join(contents, "Frameworks")
	files, err := machOFiles(contents)
	if err != nil {
		return err
	}
	for _, path := range files {
		var args []string
		ids, err := machOStrings(path, 0xd)
		if err != nil {
			return err
		}
		for _, id := range ids {
			if r, ok := strings.CutPrefix(id, "@executable_path/../Frameworks/"); ok {
				args = append(args, "-id", "@rpath/"+r)
			}
		}
		names, err := machODylibs(path)
		if err != nil {
			return err
		}
		for _, name := range names {
			if r := bundledLib(frameworks, name); r != "" {
				args = append(args, "-change", name, "@rpath/"+r)
			}
		}
		if filepath.Dir(path) == filepath.Join(contents, "MacOS") {
			rpaths, err := machOStrings(path, 0x8000001c)
			if err != nil {
				return err
			}
			if !slices.Contains(rpaths, "@executable_path/../Frameworks") {
				args = append(args, "-add_rpath", "@executable_path/../Frameworks")
			}
		}
		if len(args) > 0 {
			if err := run("install_name_tool", append(args, path)...); err != nil {
				return err
			}
		}
	}
	return nil
}

func bundledLib(frameworks, name string) string {
	if r, ok := strings.CutPrefix(name, "@executable_path/../Frameworks/"); ok {
		return r
	}
	if !strings.HasPrefix(name, "/") || strings.HasPrefix(name, "/System/") || strings.HasPrefix(name, "/usr/lib/") {
		return ""
	}
	r := filepath.Base(name)
	if i := strings.Index(name, ".framework/"); i >= 0 {
		r = name[strings.LastIndex(name[:i], "/")+1:]
	}
	if _, err := os.Stat(filepath.Join(frameworks, filepath.FromSlash(r))); err != nil {
		return ""
	}
	return r
}

func pruneMachO(contents string) error {
	frameworks := filepath.Join(contents, "Frameworks")

	var queue []string
	err := filepath.WalkDir(contents, func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if d.IsDir() && filepath.Dir(path) == frameworks && strings.HasSuffix(d.Name(), ".framework") {
			return filepath.SkipDir
		}
		if d.Type().IsRegular() && isMachO(path) {
			queue = append(queue, path)
		}
		return nil
	})
	if err != nil {
		return err
	}

	reached := map[string]bool{}
	for len(queue) > 0 {
		path := queue[0]
		queue = queue[1:]
		names, err := machODylibs(path)
		if err != nil {
			return err
		}
		for _, name := range names {
			var lib string
			if r, ok := strings.CutPrefix(name, "@rpath/"); ok {
				lib = filepath.Join(frameworks, filepath.FromSlash(r))
			} else if r, ok := strings.CutPrefix(name, "@executable_path/"); ok {
				lib = filepath.Join(contents, "MacOS", filepath.FromSlash(r))
			} else if r, ok := strings.CutPrefix(name, "@loader_path/"); ok {
				lib = filepath.Join(filepath.Dir(path), filepath.FromSlash(r))
			} else {
				continue
			}
			rel, err := filepath.Rel(frameworks, lib)
			if err != nil || strings.HasPrefix(rel, "..") {
				continue
			}
			top, _, _ := strings.Cut(filepath.ToSlash(rel), "/")
			if reached[top] {
				continue
			}
			if _, err := os.Stat(lib); err != nil {
				continue
			}
			reached[top] = true
			if strings.HasSuffix(top, ".framework") {
				files, err := machOFiles(filepath.Join(frameworks, top))
				if err != nil {
					return err
				}
				queue = append(queue, files...)
			} else {
				queue = append(queue, lib)
			}
		}
	}

	entries, err := os.ReadDir(frameworks)
	if err != nil {
		if errors.Is(err, fs.ErrNotExist) {
			return nil
		}
		return err
	}
	for _, e := range entries {
		if strings.HasSuffix(e.Name(), ".framework") && !reached[e.Name()] {
			if err := os.RemoveAll(filepath.Join(frameworks, e.Name())); err != nil {
				return err
			}
		}
	}
	return nil
}

func deployQtWindows(c *config, stage, exe string) error {
	info, err := c.qtPaths()
	if err != nil {
		return err
	}
	tool, err := info.tool("windeployqt6", "windeployqt", "windeployqt-qt5")
	if err != nil {
		return err
	}
	var args []string
	if slices.Contains(c.tags, "qml") {
		stub := filepath.Join(filepath.Dir(stage), "qmlstub")
		if err := c.qmlStub(stub); err != nil {
			return err
		}
		args = append(args, "--qmldir", stub)
	}
	if err := run(tool, append(args, exe)...); err != nil {
		return err
	}
	conf := filepath.Join(stage, "qt.conf")
	if _, err := os.Stat(conf); errors.Is(err, fs.ErrNotExist) {
		paths := "[Paths]\nPrefix = .\nPlugins = .\nQmlImports = qml\nQml2Imports = qml\nTranslations = translations\n"
		if err := os.WriteFile(conf, []byte(paths), 0o644); err != nil {
			return err
		}
	}
	return copyDLLs(stage, info.bins)
}

func copyDLLs(stage, bins string) error {
	available := map[string]string{}
	entries, err := os.ReadDir(bins)
	if err != nil {
		return err
	}
	for _, e := range entries {
		if strings.EqualFold(filepath.Ext(e.Name()), ".dll") {
			available[strings.ToLower(e.Name())] = filepath.Join(bins, e.Name())
		}
	}

	present := map[string]bool{}
	var queue []string
	err = filepath.WalkDir(stage, func(path string, d fs.DirEntry, err error) error {
		if err != nil || d.IsDir() {
			return err
		}
		ext := strings.ToLower(filepath.Ext(path))
		if ext == ".dll" || ext == ".exe" {
			present[strings.ToLower(d.Name())] = true
			queue = append(queue, path)
		}
		return nil
	})
	if err != nil {
		return err
	}

	for len(queue) > 0 {
		path := queue[0]
		queue = queue[1:]
		f, err := pe.Open(path)
		if err != nil {
			continue
		}
		syms, err := f.ImportedSymbols()
		f.Close()
		if err != nil {
			return fmt.Errorf("%s: %w", path, err)
		}
		for _, sym := range syms {
			_, lib, ok := strings.Cut(sym, ":")
			if !ok {
				continue
			}
			name := strings.ToLower(lib)
			src, ok := available[name]
			if !ok || present[name] {
				continue
			}
			data, err := os.ReadFile(src)
			if err != nil {
				return err
			}
			dst := filepath.Join(stage, filepath.Base(src))
			if err := os.WriteFile(dst, data, 0o644); err != nil {
				return err
			}
			present[name] = true
			queue = append(queue, dst)
		}
	}
	return nil
}
