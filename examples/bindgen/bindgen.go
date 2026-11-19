// Bindgen generates bindings for other languages from the IUP class registry, the public C headers
// and the callback signatures in the docs.
//
// Every language is a directory of text/template files plus a types.json that maps C types to that
// language. Files ending in .tmpl are rendered with the API model, the others are copied as they are.
// A check.json names the command that verifies the generated bindings, run with -check.
//
// Usage:
//
//	bindgen [-lang python] [-templates dir] [-out dir] [-libs dir] [-check] [-json file] [-include dir] [-docs dir]
//
// Build it with the tags of the optional libraries the bindings should cover, for example
// go run -tags "ctrl,gl,plot,web,media" . -lang lua -out /tmp/iup-lua -check
package main

import (
	"bufio"
	"compress/gzip"
	"embed"
	"encoding/json"
	"flag"
	"fmt"
	"go/constant"
	"go/token"
	"go/types"
	"io"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"text/template"
	"unicode"

	"github.com/gen2brain/iup-go/iup"
)

//go:embed templates
var builtin embed.FS

// API is the language-neutral model handed to the templates.
type API struct {
	Version   string
	Date      string
	Libraries []Library
	Functions []Function
	Constants []Constant
	Classes   []Class
	Globals   []Global
}

// Library is one IUP shared library: the core or an optional one with its open function.
type Library struct {
	Name    string
	Open    string
	Classes []string
}

// Function is a C function from the public headers.
type Function struct {
	Name     string
	Library  string
	Return   string
	Params   []Param
	Variadic bool
	VaList   bool
}

// Param is a named and typed parameter.
type Param struct {
	Name string
	Type string
}

// Constant is a #define or enum value with a known type.
type Constant struct {
	Name  string
	Kind  string
	Value string
}

// Class is a registered IUP class.
type Class struct {
	Name        string
	Function    string
	Library     string
	Parent      string
	NativeType  string
	Children    int // -1 any number, 0 none, n at most n
	Interactive bool
	Constructor Constructor
	Attributes  []Attribute
	Callbacks   []Callback
}

// Constructor describes the IupCreatev parameters of a class.
type Constructor struct {
	Format string
	Attr   string
	Params []Param
}

// Attribute is a registered class attribute.
type Attribute struct {
	Name          string
	Default       string
	SystemDefault string
	Flags         []string
	IDs           int
	ReadOnly      bool
	WriteOnly     bool
	Inherit       bool
	Handle        bool
	Pointer       bool
	Kind          string
}

// Callback is a registered class callback.
type Callback struct {
	Name   string
	Format string
	Return string
	Params []Param
}

// Global is a registered global attribute.
type Global struct {
	Name      string
	ReadOnly  bool
	WriteOnly bool
	Pointer   bool
}

type extraLib struct {
	name, open string
	fn         func()
}

var extraOpens []extraLib

var headerLibrary = map[string]string{
	"iup.h": "iup", "iupdraw.h": "iup", "iup_config.h": "iup", "iup_varg.h": "iup",
	"iupctrl.h": "iupctrl", "iupgl.h": "iupgl", "iupplot.h": "iupplot", "iupweb.h": "iupweb", "iupmedia.h": "iupmedia",
}

var attrFlags = []struct {
	bit  int
	name string
}{
	{iup.AF_NO_INHERIT, "NO_INHERIT"}, {iup.AF_NO_DEFAULTVALUE, "NO_DEFAULTVALUE"}, {iup.AF_NO_STRING, "NO_STRING"},
	{iup.AF_NOT_MAPPED, "NOT_MAPPED"}, {iup.AF_HAS_ID, "HAS_ID"}, {iup.AF_READONLY, "READONLY"},
	{iup.AF_WRITEONLY, "WRITEONLY"}, {iup.AF_HAS_ID2, "HAS_ID2"}, {iup.AF_CALLBACK, "CALLBACK"},
	{iup.AF_NO_SAVE, "NO_SAVE"}, {iup.AF_NOT_SUPPORTED, "NOT_SUPPORTED"}, {iup.AF_IHANDLENAME, "IHANDLENAME"},
	{iup.AF_IHANDLE, "IHANDLE"},
}

func init() { iup.EntryPoint(main) }

func main() {
	lang := flag.String("lang", "python", "language template directory")
	tmplDir := flag.String("templates", "", "directory with language template directories, instead of the built-in ones")
	out := flag.String("out", "", "output directory")
	jsonOut := flag.String("json", "", "write the API model as JSON to this file")
	include := flag.String("include", "../../iup/external/include", "directory with the IUP public headers")
	docs := flag.String("docs", "../../docs", "directory with the IUP docs, for callback parameter names")
	libs := flag.String("libs", "../../iup/libs", "directory with the prebuilt IUP libraries copied next to the bindings, empty to skip")
	check := flag.Bool("check", false, "run the check command of the language in the output directory")
	flag.Parse()

	if *out == "" && *jsonOut == "" {
		fmt.Fprintln(os.Stderr, "bindgen: nothing to do, set -out and/or -json")
		os.Exit(2)
	}

	iup.Open()
	defer iup.Close()

	api, err := buildAPI(*include, *docs)
	if err != nil {
		fmt.Fprintln(os.Stderr, "bindgen:", err)
		os.Exit(1)
	}

	if *jsonOut != "" {
		data, _ := json.MarshalIndent(api, "", "  ")
		if err := os.WriteFile(*jsonOut, data, 0o644); err != nil {
			fmt.Fprintln(os.Stderr, "bindgen:", err)
			os.Exit(1)
		}
	}

	if *out != "" {
		var src fs.FS
		if *tmplDir != "" {
			src = os.DirFS(filepath.Join(*tmplDir, *lang))
		} else {
			src, err = fs.Sub(builtin, "templates/"+*lang)
			if err != nil {
				fmt.Fprintln(os.Stderr, "bindgen:", err)
				os.Exit(1)
			}
		}
		if err := render(src, *out, api); err != nil {
			fmt.Fprintln(os.Stderr, "bindgen:", err)
			os.Exit(1)
		}
		if *libs != "" {
			if err := copyLibs(filepath.Join(*libs, runtime.GOOS+"_"+runtime.GOARCH), *out); err != nil {
				fmt.Fprintln(os.Stderr, "bindgen:", err)
				os.Exit(1)
			}
		}
		if *check {
			if err := runCheck(src, *out); err != nil {
				fmt.Fprintln(os.Stderr, "bindgen: check:", err)
				os.Exit(1)
			}
		}
	}
}

func copyLibs(dir, out string) error {
	files, err := filepath.Glob(filepath.Join(dir, "*.gz"))
	if err != nil {
		return err
	}
	if len(files) == 0 {
		return fmt.Errorf("no libraries in %s", dir)
	}
	for _, file := range files {
		if err := gunzip(file, filepath.Join(out, strings.TrimSuffix(filepath.Base(file), ".gz"))); err != nil {
			return err
		}
	}
	return nil
}

func gunzip(src, dst string) error {
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	zr, err := gzip.NewReader(in)
	if err != nil {
		return err
	}
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	if _, err := io.Copy(out, zr); err != nil {
		out.Close()
		return err
	}
	return out.Close()
}

func runCheck(src fs.FS, out string) error {
	data, err := fs.ReadFile(src, "check.json")
	if err != nil {
		return err
	}
	commands := map[string][]string{}
	if err := json.Unmarshal(data, &commands); err != nil {
		return fmt.Errorf("check.json: %w", err)
	}
	args := commands[runtime.GOOS]
	if len(args) == 0 {
		args = commands["run"]
	}
	if len(args) == 0 {
		return fmt.Errorf("check.json has no command")
	}
	dir, err := filepath.Abs(out)
	if err != nil {
		return err
	}
	cmd := exec.Command(args[0], args[1:]...)
	cmd.Dir = dir
	cmd.Env = append(os.Environ(), "IUP_LIBRARY_PATH="+dir)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func buildAPI(include, docs string) (*API, error) {
	api := &API{Version: iup.Version(), Date: iup.VersionDate()}

	classLib := map[string]string{}
	core := Library{Name: "iup"}
	for _, c := range iup.GetAllClasses() {
		classLib[c] = core.Name
		core.Classes = append(core.Classes, c)
	}
	api.Libraries = append(api.Libraries, core)
	for _, e := range extraOpens {
		e.fn()
		lib := Library{Name: e.name, Open: e.open}
		for _, c := range iup.GetAllClasses() {
			if _, ok := classLib[c]; !ok {
				classLib[c] = e.name
				lib.Classes = append(lib.Classes, c)
			}
		}
		api.Libraries = append(api.Libraries, lib)
	}

	var err error
	api.Functions, err = parseFunctions(include)
	if err != nil {
		return nil, err
	}
	api.Constants, err = parseConstants(include)
	if err != nil {
		return nil, err
	}
	signatures := parseDocSignatures(docs)

	ctorFunc := map[string]string{}
	for _, f := range api.Functions {
		ctorFunc[strings.ToLower(f.Name)] = f.Name
	}

	names := iup.GetAllClasses()
	sort.Strings(names)
	for _, name := range names {
		api.Classes = append(api.Classes, buildClass(name, classLib[name], ctorFunc["iup"+name], signatures))
	}

	for _, name := range iup.GetAllGlobals() {
		info, _ := iup.GetGlobalInfo(name)
		api.Globals = append(api.Globals, Global{
			Name:      name,
			ReadOnly:  info.Flags&iup.GF_READONLY != 0,
			WriteOnly: info.Flags&iup.GF_WRITEONLY != 0,
			Pointer:   info.Flags&iup.GF_POINTER != 0,
		})
	}
	sort.Slice(api.Globals, func(i, j int) bool { return api.Globals[i].Name < api.Globals[j].Name })

	return api, nil
}

func buildClass(name, lib, function string, signatures map[string][][]Param) Class {
	info, _ := iup.GetClassInfo(name)
	c := Class{
		Name:        name,
		Function:    function,
		Library:     lib,
		Parent:      info.Parent,
		NativeType:  info.NativeType,
		Children:    childCount(info.ChildType),
		Interactive: info.IsInteractive,
	}

	if ctor, ok := iup.GetClassConstructor(name); ok {
		c.Constructor = Constructor{Format: ctor.Format, Attr: ctor.FormatAttr, Params: constructorParams(ctor)}
	}

	attrs := iup.GetClassAttributes(name)
	sort.Strings(attrs)
	for _, a := range attrs {
		ai, _ := iup.GetClassAttributeInfo(name, a)
		c.Attributes = append(c.Attributes, buildAttribute(a, ai))
	}

	cbs := iup.GetClassCallbacks(name)
	sort.Strings(cbs)
	for _, cb := range cbs {
		format := iup.GetClassCallbackFormat(name, cb)
		ret, types := callbackTypes(format)
		c.Callbacks = append(c.Callbacks, Callback{Name: cb, Format: format, Return: ret, Params: namedParams(types, signatures[cb])})
	}

	return c
}

func childCount(childType int) int {
	if childType < 0 {
		return -1
	}
	return childType
}

func buildAttribute(name string, ai iup.ClassAttributeInfo) Attribute {
	a := Attribute{
		Name:          name,
		Default:       ai.DefaultValue,
		SystemDefault: ai.SystemDefault,
		ReadOnly:      ai.Flags&iup.AF_READONLY != 0,
		WriteOnly:     ai.Flags&iup.AF_WRITEONLY != 0,
		Inherit:       ai.Flags&iup.AF_NO_INHERIT == 0,
		Handle:        ai.Flags&(iup.AF_IHANDLE|iup.AF_IHANDLENAME) != 0,
		Pointer:       ai.Flags&iup.AF_NO_STRING != 0,
	}
	for _, f := range attrFlags {
		if ai.Flags&f.bit != 0 {
			a.Flags = append(a.Flags, f.name)
		}
	}
	switch {
	case ai.Flags&iup.AF_HAS_ID2 != 0:
		a.IDs = 2
	case ai.Flags&iup.AF_HAS_ID != 0:
		a.IDs = 1
	}
	switch {
	case a.Handle:
		a.Kind = "handle"
	case a.Pointer:
		a.Kind = "pointer"
	case strings.EqualFold(a.Default, "YES") || strings.EqualFold(a.Default, "NO"):
		a.Kind = "bool"
	default:
		a.Kind = "string"
	}
	return a
}

func constructorParams(ctor iup.ClassConstructorInfo) []Param {
	var params []Param
	for i, ch := range ctor.Format {
		switch ch {
		case 's', 'a':
			name := "title"
			if i == 0 && ctor.FormatAttr != "" {
				name = strings.ToLower(ctor.FormatAttr)
			}
			params = append(params, Param{Name: name, Type: "char*"})
		case 'h':
			params = append(params, Param{Name: "child", Type: "Ihandle*"})
		case 'g':
			params = append(params, Param{Name: "children", Type: "Ihandle**"})
		case 'i':
			params = append(params, Param{Name: map[bool]string{true: "width", false: "height"}[i == 0], Type: "int"})
		case 'c':
			params = append(params, Param{Name: "pixels", Type: "unsigned char*"})
		case 'f':
			params = append(params, Param{Name: fmt.Sprintf("p%d", i), Type: "float"})
		case 'b':
			params = append(params, Param{Name: fmt.Sprintf("p%d", i), Type: "unsigned char"})
		case 'j':
			params = append(params, Param{Name: fmt.Sprintf("p%d", i), Type: "int*"})
		}
	}
	return params
}

var formatTypes = map[rune]string{
	'c': "unsigned char", 'i': "int", 'I': "int*", 'f': "float", 'd': "double",
	's': "char*", 'V': "void*", 'n': "Ihandle*",
}

func callbackTypes(format string) (string, []string) {
	ret := "int"
	params := format
	if i := strings.IndexByte(format, '='); i >= 0 {
		params = format[:i]
		if len(format) > i+1 {
			ret = formatTypes[rune(format[i+1])]
		}
	}
	types := []string{"Ihandle*"}
	for _, ch := range params {
		types = append(types, formatTypes[ch])
	}
	return ret, types
}

func namedParams(types []string, candidates [][]Param) []Param {
	for _, cand := range candidates {
		if len(cand) != len(types) {
			continue
		}
		match := true
		for i := range cand {
			if cand[i].Type != types[i] {
				match = false
				break
			}
		}
		if match {
			out := make([]Param, len(cand))
			copy(out, cand)
			return out
		}
	}
	out := []Param{{Name: "ih", Type: "Ihandle*"}}
	for i, t := range types[1:] {
		out = append(out, Param{Name: fmt.Sprintf("arg%d", i+1), Type: t})
	}
	return out
}

var protoRe = regexp.MustCompile(`^IUP[A-Z]*_API\s+(.+?)\s*\b(Iup\w+)\s*\((.*)\)\s*;`)

func parseFunctions(include string) ([]Function, error) {
	files := make([]string, 0, len(headerLibrary))
	for h := range headerLibrary {
		files = append(files, h)
	}
	sort.Strings(files)

	var funcs []Function
	seen := map[string]bool{}
	for _, h := range files {
		lines, err := readLines(filepath.Join(include, h))
		if err != nil {
			return nil, err
		}
		for _, line := range lines {
			m := protoRe.FindStringSubmatch(strings.TrimSpace(line))
			if m == nil || seen[m[2]] {
				continue
			}
			seen[m[2]] = true
			f := Function{Name: m[2], Library: headerLibrary[h], Return: normType(m[1])}
			for i, p := range splitParams(m[3]) {
				switch {
				case p == "void":
				case p == "...":
					f.Variadic = true
				case strings.Contains(p, "va_list"):
					f.VaList = true
				default:
					f.Params = append(f.Params, splitParam(p, i))
				}
			}
			funcs = append(funcs, f)
		}
	}
	sort.Slice(funcs, func(i, j int) bool { return funcs[i].Name < funcs[j].Name })
	return funcs, nil
}

func splitParams(s string) []string {
	var out []string
	for _, p := range strings.Split(s, ",") {
		if p = strings.TrimSpace(p); p != "" {
			out = append(out, p)
		}
	}
	return out
}

var identRe = regexp.MustCompile(`^(.*?)(\w+)\s*(\[\s*\])?$`)

func splitParam(p string, i int) Param {
	p = strings.Join(strings.Fields(p), " ")
	m := identRe.FindStringSubmatch(p)
	if m == nil || strings.TrimSpace(m[1]) == "" || strings.HasSuffix(strings.TrimSpace(m[1]), "unsigned") {
		return Param{Name: fmt.Sprintf("p%d", i), Type: normType(p)}
	}
	t := m[1]
	if m[3] != "" {
		t += "*"
	}
	return Param{Name: m[2], Type: normType(t)}
}

func normType(t string) string {
	t = strings.ReplaceAll(t, "const ", "")
	t = strings.Join(strings.Fields(t), " ")
	t = strings.ReplaceAll(t, " *", "*")
	return strings.TrimSpace(t)
}

var (
	defineRe  = regexp.MustCompile(`^#define\s+([A-Za-z_]\w*)\s+(.+?)\s*(/\*.*)?$`)
	macroRe   = regexp.MustCompile(`^#define\s+(\w+)\(\s*(\w+)\s*\)\s+(.+?)\s*(/\*.*)?$`)
	commentRe = regexp.MustCompile(`/\*.*?\*/|//.*$`)
	wordRe    = regexp.MustCompile(`\b[0-9][0-9A-Fa-fxX]*[uUlL]*\b|[A-Za-z_]\w*`)
)

func parseConstants(include string) ([]Constant, error) {
	values := map[string]string{}
	macros := map[string][2]string{}
	var consts []Constant

	add := func(name, kind, value string) {
		if _, ok := values[name]; ok {
			return
		}
		values[name] = value
		consts = append(consts, Constant{Name: name, Kind: kind, Value: value})
	}

	for _, h := range []string{"iup.h", "iupkey.h", "iupdraw.h", "iupctrl.h", "iupgl.h", "iupplot.h", "iupweb.h", "iupmedia.h"} {
		lines, err := readLines(filepath.Join(include, h))
		if err != nil {
			return nil, err
		}
		enumText := ""
		inEnum := false
		for _, line := range lines {
			t := strings.TrimSpace(commentRe.ReplaceAllString(line, ""))
			if m := macroRe.FindStringSubmatch(t); m != nil {
				macros[m[1]] = [2]string{m[2], m[3]}
				continue
			}
			if !inEnum && strings.HasPrefix(t, "enum") && strings.Contains(t, "{") {
				inEnum, enumText = true, ""
			}
			if inEnum {
				enumText += " " + t
				if !strings.Contains(t, "}") {
					continue
				}
				inEnum = false
				body := enumText[strings.Index(enumText, "{")+1 : strings.Index(enumText, "}")]
				next := int64(0)
				for _, entry := range strings.Split(body, ",") {
					name, expr, _ := strings.Cut(strings.TrimSpace(entry), "=")
					name = strings.TrimSpace(name)
					if name == "" {
						continue
					}
					if expr = strings.TrimSpace(expr); expr != "" {
						v, ok := evalInt(expr, values, macros)
						if !ok {
							break
						}
						next = v
					}
					add(name, "int", strconv.FormatInt(next, 10))
					next++
				}
				continue
			}
			m := defineRe.FindStringSubmatch(t)
			if m == nil || strings.HasSuffix(m[1], "_API") {
				continue
			}
			expr := m[2]
			if strings.HasPrefix(expr, `"`) && strings.HasSuffix(expr, `"`) {
				add(m[1], "string", cUnquote(expr[1:len(expr)-1]))
				continue
			}
			if v, ok := evalInt(expr, values, macros); ok {
				add(m[1], "int", strconv.FormatInt(v, 10))
			}
		}
	}
	return consts, nil
}

func cUnquote(s string) string {
	var b strings.Builder
	for i := 0; i < len(s); i++ {
		if s[i] == '\\' && i+1 < len(s) {
			i++
			switch s[i] {
			case 'n':
				b.WriteByte('\n')
			case 't':
				b.WriteByte('\t')
			default:
				b.WriteByte(s[i])
			}
			continue
		}
		b.WriteByte(s[i])
	}
	return b.String()
}

var charRe = regexp.MustCompile(`'(\\.|[^'\\])'`)

func charValue(lit string) string {
	inner := lit[1 : len(lit)-1]
	if len(inner) == 2 && inner[0] == '\\' && strings.ContainsRune(`"'?\\`, rune(inner[1])) {
		return strconv.Itoa(int(inner[1]))
	}
	r, _, _, err := strconv.UnquoteChar(inner, '\'')
	if err != nil {
		return lit
	}
	return strconv.Itoa(int(r))
}

func evalInt(expr string, values map[string]string, macros map[string][2]string) (int64, bool) {
	expr = charRe.ReplaceAllStringFunc(expr, charValue)
	for i := 0; i < 4; i++ {
		for name, m := range macros {
			re := regexp.MustCompile(`\b` + name + `\s*\(([^()]*)\)`)
			expr = re.ReplaceAllStringFunc(expr, func(s string) string {
				arg := re.FindStringSubmatch(s)[1]
				return "(" + regexp.MustCompile(`\b`+m[0]+`\b`).ReplaceAllString(m[1], "("+arg+")") + ")"
			})
		}
	}
	unknown := false
	expr = wordRe.ReplaceAllStringFunc(expr, func(w string) string {
		if v, ok := values[w]; ok {
			return "(" + v + ")"
		}
		if unicode.IsDigit(rune(w[0])) {
			return strings.TrimRight(w, "uUlL")
		}
		unknown = true
		return w
	})
	if unknown || strings.ContainsAny(expr, `"{};`) {
		return 0, false
	}
	expr = regexp.MustCompile(`\(\s*(int|unsigned|long)\s*\)`).ReplaceAllString(expr, "")
	tv, err := types.Eval(token.NewFileSet(), nil, token.NoPos, expr)
	if err != nil || tv.Value == nil {
		return 0, false
	}
	v := constant.ToInt(tv.Value)
	if v.Kind() != constant.Int {
		return 0, false
	}
	i, ok := constant.Int64Val(v)
	return i, ok
}

var docSigRe = regexp.MustCompile(`^\s{4}(int|char\*|double|float|void|Ihandle\*)\s+function\s*\((.*)\)\s*;`)
var docNameRe = regexp.MustCompile(`^(?:##\s+|\*\*)([A-Z][A-Z0-9_]*)\b`)

func parseDocSignatures(docs string) map[string][][]Param {
	sigs := map[string][][]Param{}
	_ = filepath.WalkDir(docs, func(path string, d fs.DirEntry, err error) error {
		if err != nil || d.IsDir() || !strings.HasSuffix(path, ".md") {
			return nil
		}
		lines, err := readLines(path)
		if err != nil {
			return nil
		}
		current := ""
		for _, line := range lines {
			if m := docNameRe.FindStringSubmatch(line); m != nil {
				current = m[1]
				continue
			}
			m := docSigRe.FindStringSubmatch(line)
			if m == nil || current == "" {
				continue
			}
			var params []Param
			for i, p := range splitParams(m[2]) {
				if p != "void" {
					params = append(params, splitParam(p, i))
				}
			}
			sigs[current] = append(sigs[current], params)
		}
		return nil
	})
	return sigs
}

func readLines(path string) ([]string, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	var lines []string
	sc := bufio.NewScanner(f)
	sc.Buffer(make([]byte, 1024*1024), 1024*1024)
	for sc.Scan() {
		lines = append(lines, sc.Text())
	}
	return lines, sc.Err()
}

func render(src fs.FS, out string, api *API) error {
	typeMap := map[string]string{}
	if data, err := fs.ReadFile(src, "types.json"); err == nil {
		if err := json.Unmarshal(data, &typeMap); err != nil {
			return fmt.Errorf("types.json: %w", err)
		}
	}

	funcs := template.FuncMap{
		"type": func(c string) string {
			if t, ok := typeMap[c]; ok {
				return t
			}
			return typeMap["*"]
		},
		"lower":  strings.ToLower,
		"upper":  strings.ToUpper,
		"quote":  strconv.Quote,
		"join":   strings.Join,
		"trim":   strings.TrimPrefix,
		"has":    strings.Contains,
		"prefix": strings.HasPrefix,
		"camel": func(s string) string {
			parts := strings.FieldsFunc(strings.ToLower(s), func(r rune) bool { return r == '_' })
			for i, p := range parts {
				parts[i] = strings.ToUpper(p[:1]) + p[1:]
			}
			return strings.Join(parts, "")
		},
	}

	if err := os.MkdirAll(out, 0o755); err != nil {
		return err
	}
	return fs.WalkDir(src, ".", func(path string, d fs.DirEntry, err error) error {
		if err != nil || d.IsDir() || path == "types.json" || path == "check.json" {
			return err
		}
		data, err := fs.ReadFile(src, path)
		if err != nil {
			return err
		}
		dst := filepath.Join(out, strings.TrimSuffix(path, ".tmpl"))
		if err := os.MkdirAll(filepath.Dir(dst), 0o755); err != nil {
			return err
		}
		if !strings.HasSuffix(path, ".tmpl") {
			return os.WriteFile(dst, data, 0o644)
		}
		t, err := template.New(path).Funcs(funcs).Parse(string(data))
		if err != nil {
			return err
		}
		f, err := os.Create(dst)
		if err != nil {
			return err
		}
		defer f.Close()
		return t.Execute(f, api)
	})
}
