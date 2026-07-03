// Command genversion propagates IUP_VERSION from external/include/iup.h into
// external/VERSION, the extlib sysLibNames soversion, and the nocgo/wasm consts.
// With a version argument (go run ./internal/genversion 4.0.0) it first sets
// IUP_VERSION, IUP_VERSION_NUMBER, and IUP_VERSION_DATE (today) in iup.h, then propagates.
package main

import (
	"fmt"
	"go/format"
	"os"
	"regexp"
	"strconv"
	"strings"
	"time"
)

const header = "external/include/iup.h"

var macros = [][2]string{
	{"NAME", "IUP_NAME"},
	{"DESCRIPTION", "IUP_DESCRIPTION"},
	{"COPYRIGHT", "IUP_COPYRIGHT"},
	{"VERSION", "IUP_VERSION"},
	{"VERSION_NUMBER", "IUP_VERSION_NUMBER"},
	{"VERSION_DATE", "IUP_VERSION_DATE"},
}

func main() {
	if len(os.Args) > 1 {
		bumpHeader(os.Args[1])
	}

	h, err := os.ReadFile(header)
	check(err)
	src := string(h)

	var b strings.Builder
	b.WriteString("const (\n")
	for _, m := range macros {
		fmt.Fprintf(&b, "%s = %s\n", m[0], macro(src, m[1]))
	}
	b.WriteString(")")
	blockRe := regexp.MustCompile(`(?s)const \(\s*\n\s*NAME\b.*?\n\)`)
	for _, f := range []string{"nocgo_constants.go", "wasm_constants.go"} {
		replace(f, blockRe, b.String())
	}

	ver := strings.Trim(macro(src, "IUP_VERSION"), `"`)
	check(os.WriteFile("external/VERSION", []byte(ver+"\n"), 0o644))
	fmt.Println("wrote external/VERSION", ver)

	major := ver[:strings.IndexByte(ver, '.')]
	replace("nocgo_purego_linux.go", regexp.MustCompile(`"\.so\.\d+"`), `".so.`+major+`"`)
	replace("nocgo_purego_darwin.go", regexp.MustCompile(`"\.\d+\.dylib"`), `".`+major+`.dylib"`)
}

// bumpHeader sets IUP_VERSION and IUP_VERSION_NUMBER in iup.h to ver.
func bumpHeader(ver string) {
	if !regexp.MustCompile(`^\d+(\.\d+)*$`).MatchString(ver) {
		fail("invalid version: " + ver)
	}
	num := 0
	mul := []int{100000, 1000, 1}
	for i, p := range strings.Split(ver, ".") {
		if i >= len(mul) {
			break
		}
		n, _ := strconv.Atoi(p)
		num += n * mul[i]
	}

	date := time.Now().Format("2006/01/02")

	h, err := os.ReadFile(header)
	check(err)
	out := regexp.MustCompile(`#define IUP_VERSION "[^"]*"`).ReplaceAllLiteralString(string(h), `#define IUP_VERSION "`+ver+`"`)
	out = regexp.MustCompile(`#define IUP_VERSION_NUMBER \d+`).ReplaceAllLiteralString(out, `#define IUP_VERSION_NUMBER `+strconv.Itoa(num))
	out = regexp.MustCompile(`#define IUP_VERSION_DATE "[^"]*"`).ReplaceAllLiteralString(out, `#define IUP_VERSION_DATE "`+date+`"`)
	check(os.WriteFile(header, []byte(out), 0o644))
	fmt.Printf("set iup.h IUP_VERSION %q (%d) %s\n", ver, num, date)
}

func macro(h, name string) string {
	g := regexp.MustCompile(`#define ` + name + `\s+("[^"]*"|\S+)`).FindStringSubmatch(h)
	if g == nil {
		fail(name + " not found in iup.h")
	}
	return g[1]
}

func replace(f string, re *regexp.Regexp, repl string) {
	src, err := os.ReadFile(f)
	check(err)
	if !re.Match(src) {
		fail("pattern not found in " + f)
	}
	out, err := format.Source([]byte(re.ReplaceAllLiteralString(string(src), repl)))
	check(err)
	check(os.WriteFile(f, out, 0o644))
	fmt.Println("synced", f)
}

func check(err error) {
	if err != nil {
		fail(err.Error())
	}
}

func fail(msg string) {
	fmt.Fprintln(os.Stderr, "genversion:", msg)
	os.Exit(1)
}
