package main

import (
	"errors"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"slices"
	"strconv"
	"strings"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/apple"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/authenticode"
	"github.com/gen2brain/iup-go/cmd/iupkg/internal/keys"
)

const usageText = `iupkg builds and packages IUP-Go applications.

Usage:

	iupkg package [flags] [package]
	iupkg sign [flags] <file>
	iupkg serve [--addr localhost:8000] <directory>
	iupkg staple <app>

Targets (--os):

	windows  .exe with icon, version info and the IUP manifest; Authenticode with --sign
	darwin   .app bundle and a zip of it; signed, notarized and stapled with --sign and --notary-*
	linux    .tar.gz with .desktop file, hicolor icons and a Makefile; --format deb,rpm
	android  .apk from the template in the iup module, needs only the NDK; signed
	ios      .ipa signed with --sign and --profile
	js       directory with the wasm module and loader, served by iupkg serve
	haiku    .hpkg package, active when copied into the packages directory

Signing is done by iupkg itself on every platform; on macOS --signer codesign
uses a keychain identity instead. Password of a .p12 file: IUPKG_P12_PASSWORD.
sign takes an existing .exe, .app, .ipa, .apk or Mach-O file; run
'iupkg sign -h' for its flags.

Flags:
`

type config struct {
	goos        string
	goarch      string
	out         string
	name        string
	id          string
	version     string
	build       int
	icon        string
	tags        []string
	ldflags     string
	cgo         bool
	release     bool
	console     bool
	install     bool
	simulator   bool
	minOS       string
	profile     string
	category    string
	formats     []string
	vendor      string
	copyright   string
	license     string
	permissions []string
	sign        string
	signer      string
	timestamp   bool
	tsaURL      string
	notaryKey   string
	notaryID    string
	notaryIssue string

	idSet   bool
	archSet bool
	pkg     string
	pkgDir  string
	exe     string
	iupDir  string
}

func main() {
	if len(os.Args) < 2 {
		usage(nil)
		os.Exit(2)
	}

	var err error
	switch os.Args[1] {
	case "package":
		err = runPackage(os.Args[2:])
	case "sign":
		err = runSign(os.Args[2:])
	case "serve":
		err = runServe(os.Args[2:])
	case "staple":
		if len(os.Args) != 3 {
			err = errors.New("usage: iupkg staple <app>")
		} else {
			err = apple.Staple(os.Args[2], nil)
		}
	case "help", "-h", "-help", "--help":
		usage(newFlagSet(&config{}))
	default:
		fmt.Fprintf(os.Stderr, "iupkg: unknown command %q\n", os.Args[1])
		usage(nil)
		os.Exit(2)
	}

	if err != nil {
		if errors.Is(err, flag.ErrHelp) {
			return
		}
		fmt.Fprintln(os.Stderr, "iupkg:", err)
		os.Exit(1)
	}
}

func usage(fs *flag.FlagSet) {
	fmt.Fprint(os.Stderr, usageText)
	if fs != nil {
		printDefaults(fs)
	} else {
		fmt.Fprintln(os.Stderr, "\trun 'iupkg help' for the flag list")
	}
}

func printDefaults(fs *flag.FlagSet) {
	fs.VisitAll(func(f *flag.Flag) {
		name, usage := flag.UnquoteUsage(f)
		line := "  --" + f.Name
		if name != "" {
			line += " " + name
		}
		line += "\n    \t" + usage
		if f.DefValue != "" && f.DefValue != "false" && f.DefValue != "0" {
			if fmt.Sprintf("%T", f.Value) == "*flag.stringValue" {
				line += fmt.Sprintf(" (default %q)", f.DefValue)
			} else {
				line += " (default " + f.DefValue + ")"
			}
		}
		fmt.Fprintln(os.Stderr, line)
	})
}

func newFlagSet(c *config) *flag.FlagSet {
	fs := flag.NewFlagSet("package", flag.ContinueOnError)
	fs.StringVar(&c.goos, "os", runtime.GOOS, "target `os`: windows, darwin, linux, android, ios, js, haiku")
	fs.StringVar(&c.goarch, "arch", runtime.GOARCH, "target `arch`; darwin also accepts universal, android a comma-separated list; android and ios use arm64 unless set")
	fs.StringVar(&c.out, "out", ".", "output `directory`")
	fs.StringVar(&c.name, "name", "", "application `name` (default: the executable name)")
	fs.StringVar(&c.id, "id", "", "application `identifier` in reverse DNS form, the APPID the application sets (default: com.example.<executable>; linux: the executable name; haiku: the IUP signature)")
	fs.StringVar(&c.version, "version", "1.0.0", "application `version`")
	fs.IntVar(&c.build, "build", 1, "build `number`")
	fs.StringVar(&c.icon, "icon", "", "square PNG `file`, 1024 px recommended (default: the IUP icon)")
	fs.Func("tags", "comma-separated list of build `tags`", func(s string) error {
		c.tags = splitList(s)
		return nil
	})
	fs.StringVar(&c.ldflags, "ldflags", "", "extra linker `flags`")
	fs.BoolVar(&c.cgo, "cgo", false, "build with cgo (default: go env CGO_ENABLED for the host platform, off when cross-compiling, on with a driver tag)")
	fs.BoolVar(&c.release, "release", false, "strip symbols and file paths")
	fs.BoolVar(&c.console, "console", false, "windows: build a console application")
	fs.BoolVar(&c.install, "install", false, "android, ios: install on the connected device")
	fs.BoolVar(&c.simulator, "simulator", false, "ios: build for the simulator")
	fs.StringVar(&c.minOS, "minos", "15.0", "ios: minimum iOS `version`")
	fs.StringVar(&c.profile, "profile", "", "ios: provisioning profile `file` (.mobileprovision)")
	fs.StringVar(&c.category, "category", "Utility", "linux: desktop entry `categories`, separated by ;")
	fs.Func("format", "linux: comma-separated `formats`: targz, deb, rpm (default targz)", func(s string) error {
		c.formats = splitList(s)
		return nil
	})
	fs.StringVar(&c.vendor, "vendor", "", "`vendor`: Windows company name, Debian maintainer, RPM and Haiku vendor (default: the application name)")
	fs.StringVar(&c.copyright, "copyright", "", "copyright `line` for Windows and Haiku (haiku default: the application name)")
	fs.StringVar(&c.license, "license", "Unknown", "license `name` for rpm and haiku")
	fs.Func("permissions", "comma-separated `list`: camera, microphone, location, notifications, internet", func(s string) error {
		c.permissions = splitList(s)
		return nil
	})
	fs.StringVar(&c.sign, "sign", "", "signing `identity`: a .p12 or PEM file with the key and certificate, a keychain identity with --signer codesign (default: android a debug key, darwin ad-hoc, others unsigned)")
	fs.StringVar(&c.signer, "signer", "", "darwin, ios: signing `tool`: iupkg or codesign (default: codesign on macOS, else iupkg)")
	fs.BoolVar(&c.timestamp, "timestamp", true, "add a trusted timestamp when signing with a certificate")
	fs.StringVar(&c.tsaURL, "timestamp-url", authenticode.DefaultTimestampURL, "windows: RFC 3161 timestamp server `url`")
	fs.StringVar(&c.notaryKey, "notary-key", "", "darwin: App Store Connect API private key `file` (AuthKey_<id>.p8), enables notarization")
	fs.StringVar(&c.notaryID, "notary-key-id", "", "darwin: App Store Connect API key `id` (default: from the key file name)")
	fs.StringVar(&c.notaryIssue, "notary-issuer", "", "darwin: App Store Connect issuer `id`")
	fs.Usage = func() { usage(fs) }
	return fs
}

func runPackage(args []string) error {
	c := &config{}
	fs := newFlagSet(c)
	if err := fs.Parse(args); err != nil {
		return err
	}
	if fs.NArg() > 1 {
		return errors.New("only one package can be packaged at a time")
	}

	c.pkg = "."
	if fs.NArg() == 1 {
		c.pkg = fs.Arg(0)
	}

	cgoSet := false
	fs.Visit(func(f *flag.Flag) {
		switch f.Name {
		case "cgo":
			cgoSet = true
		case "id":
			c.idSet = true
		case "arch":
			c.archSet = true
		}
	})
	if !cgoSet {
		c.cgo = defaultCgo(c.goos, c.goarch)
	}
	for _, tag := range c.tags {
		if slices.Contains(driverTags, tag) {
			c.cgo = true
		}
	}
	if c.goos == "wasm" {
		c.goos = "js"
	}
	if c.goos == "js" {
		c.cgo = false
		c.goarch = "wasm"
	}
	if c.goos == "android" || c.goos == "ios" {
		c.cgo = true
		if !c.archSet {
			c.goarch = "arm64"
		}
	}
	if c.goos == "haiku" {
		c.cgo = true
	}

	if c.signer == "" {
		c.signer = "iupkg"
		if runtime.GOOS == "darwin" {
			c.signer = "codesign"
		}
	}
	if c.signer != "iupkg" && c.signer != "codesign" {
		return fmt.Errorf("unknown signer %q", c.signer)
	}
	if c.signer == "codesign" && runtime.GOOS != "darwin" {
		return errors.New("--signer codesign needs a macOS host")
	}

	for _, p := range c.permissions {
		if !slices.Contains([]string{"camera", "microphone", "location", "notifications", "internet"}, p) {
			return fmt.Errorf("unknown permission %q", p)
		}
	}

	if err := c.resolve(); err != nil {
		return err
	}

	if err := os.MkdirAll(c.out, 0o755); err != nil {
		return err
	}

	switch c.goos {
	case "windows":
		return packageWindows(c)
	case "darwin":
		return packageDarwin(c)
	case "linux":
		return packageLinux(c)
	case "android":
		return packageAndroid(c)
	case "ios":
		return packageIOS(c)
	case "js":
		return packageJS(c)
	case "haiku":
		return packageHaiku(c)
	}
	return fmt.Errorf("unsupported target os %q", c.goos)
}

func (c *config) resolve() error {
	out, err := goList(c, "-f", "{{.Name}}\t{{.Dir}}", c.pkg)
	if err != nil {
		return err
	}
	name, dir, _ := strings.Cut(strings.TrimSpace(out), "\t")
	if name != "main" {
		return fmt.Errorf("%s is not a main package", c.pkg)
	}
	c.pkgDir = dir
	c.exe = filepath.Base(dir)

	out, err = goList(c, "-m", "-f", "{{.Dir}}", "github.com/gen2brain/iup-go/iup")
	if err != nil {
		return fmt.Errorf("github.com/gen2brain/iup-go/iup is not a dependency of %s", c.pkg)
	}
	c.iupDir = strings.TrimSpace(out)

	if c.name == "" {
		c.name = c.exe
	}
	if c.id == "" {
		c.id = "com.example." + identifierPart(c.exe)
	}
	if c.icon == "" {
		c.icon = filepath.Join(c.iupDir, "external", "ios", "iupapp", "Icon-1024.png")
	}
	if c.vendor == "" {
		c.vendor = c.name
	}
	if len(c.formats) == 0 {
		c.formats = []string{"targz"}
	}
	return nil
}

func (c *config) identity() (*keys.Identity, error) {
	if c.sign == "" {
		return nil, errors.New("--sign is required")
	}
	return keys.Load(c.sign)
}

var driverTags = []string{"winui", "gtk", "gtk2", "gtk4", "qt", "qt5", "motif", "fltk", "efl", "gnustep"}

func appleSigner(c *config) (*apple.Signer, error) {
	if c.sign == "" {
		return &apple.Signer{}, nil
	}
	id, err := c.identity()
	if err != nil {
		return nil, err
	}
	chain, err := apple.Chain(id.Cert, id.Extra)
	if err != nil {
		return nil, err
	}
	return &apple.Signer{Key: id.Key, Chain: chain, Timestamp: c.timestamp}, nil
}

func defaultCgo(goos, goarch string) bool {
	if goos != runtime.GOOS || goarch != runtime.GOARCH {
		return os.Getenv("CGO_ENABLED") == "1"
	}
	out, err := exec.Command("go", "env", "CGO_ENABLED").Output()
	return err == nil && strings.TrimSpace(string(out)) == "1"
}

func goList(c *config, args ...string) (string, error) {
	goarch, _, _ := strings.Cut(c.goarch, ",")
	if goarch == "universal" {
		goarch = "arm64"
	}
	list := []string{"list"}
	if len(c.tags) > 0 {
		list = append(list, "-tags", strings.Join(c.tags, ","))
	}
	args = append(list, args...)
	cmd := exec.Command("go", args...)
	cmd.Env = append(os.Environ(), c.goEnv(goarch)...)
	cmd.Stderr = os.Stderr
	out, err := cmd.Output()
	return string(out), err
}

func (c *config) goEnv(goarch string) []string {
	return []string{"GOOS=" + c.goos, "GOARCH=" + goarch, "CGO_ENABLED=" + strconv.Itoa(boolInt(c.cgo))}
}

func goBuild(c *config, goarch, out string, tags, ldflags []string) error {
	return goBuildWith(c, c.goEnv(goarch), out, tags, ldflags, nil)
}

func goBuildWith(c *config, env []string, out string, tags, ldflags, extra []string) error {
	return goBuildCmd(c, "go", env, out, tags, ldflags, extra)
}

func goBuildCmd(c *config, goCmd string, env []string, out string, tags, ldflags, extra []string) error {
	args := append([]string{"build", "-o", out}, extra...)
	tags = append(slices.Clone(c.tags), tags...)
	if len(tags) > 0 {
		args = append(args, "-tags", strings.Join(tags, ","))
	}
	if c.release {
		args = append(args, "-trimpath")
		ldflags = append(ldflags, "-s", "-w")
	}
	if c.ldflags != "" {
		ldflags = append(ldflags, c.ldflags)
	}
	if len(ldflags) > 0 {
		args = append(args, "-ldflags", strings.Join(ldflags, " "))
	}
	args = append(args, c.pkg)

	cmd := exec.Command(goCmd, args...)
	cmd.Env = append(os.Environ(), env...)
	cmd.Stdout = os.Stderr
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func (c *config) hasPermission(p string) bool {
	return slices.Contains(c.permissions, p)
}

func (c *config) iupMajor() (string, error) {
	data, err := os.ReadFile(filepath.Join(c.iupDir, "external", "VERSION"))
	if err != nil {
		return "", err
	}
	major, _, _ := strings.Cut(strings.TrimSpace(string(data)), ".")
	return major, nil
}

func splitList(s string) []string {
	var list []string
	for f := range strings.SplitSeq(s, ",") {
		if f = strings.TrimSpace(f); f != "" {
			list = append(list, f)
		}
	}
	return list
}

func identifierPart(s string) string {
	var b strings.Builder
	for _, r := range strings.ToLower(s) {
		if r >= 'a' && r <= 'z' || r >= '0' && r <= '9' {
			b.WriteRune(r)
		}
	}
	id := b.String()
	if id == "" || id[0] >= '0' && id[0] <= '9' {
		id = "app" + id
	}
	return id
}

func boolInt(b bool) int {
	if b {
		return 1
	}
	return 0
}

func run(name string, args ...string) error {
	cmd := exec.Command(name, args...)
	cmd.Stdout = os.Stderr
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("%s: %w", name, err)
	}
	return nil
}
