// iupinfo dumps the IUP class and global registries: every class with its
// constructor, parent, native type, child constraints, focus interactivity,
// attributes (defaults + flags) and callbacks (parameter format), plus every
// registered global with its flags and supporting drivers.
//
// Usage:
//
//	iupinfo                          one line per class (default)
//	iupinfo <class>...               full dump of the named classes
//	iupinfo --full                   full dump of every class + globals
//	iupinfo --globals                dump only the globals section
//	iupinfo --callbacks [name...]    aggregated callback view, optional filter
//	iupinfo --attributes [name...]   aggregated attribute view, optional filter
//
// Examples:
//
//	iupinfo button text                  full dump of two classes
//	iupinfo --callbacks ACTION           show every variant of ACTION
//	iupinfo --attributes BGCOLOR FONT    show only those two attributes
//	iupinfo --globals                    list every registered global
//
// Build tags ctrl, gl, plot, web pull in the matching additional-controls
// libraries (Flat*, Matrix, Cells, Gauge, DropButton, Plot, GLCanvas,
// WebBrowser); their classes only appear in the dump when the binary is
// built with the corresponding tag, e.g. `go build -tags "ctrl,gl,plot,web"`.
package main

import (
	"flag"
	"fmt"
	"os"
	"sort"
	"strings"
	"text/tabwriter"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

// extraOpens holds the additional-control Open functions that get pulled in
// by build tags (ctrl, gl, plot, web). Tagged files in this package add to it.
var extraOpens []func()

var attribFlagName = []struct {
	bit  int
	name string
}{
	{iup.AttribNoInherit, "NO_INHERIT"},
	{iup.AttribNoDefaultValue, "NO_DEFAULTVALUE"},
	{iup.AttribNoString, "NO_STRING"},
	{iup.AttribNotMapped, "NOT_MAPPED"},
	{iup.AttribHasID, "HAS_ID"},
	{iup.AttribReadOnly, "READONLY"},
	{iup.AttribWriteOnly, "WRITEONLY"},
	{iup.AttribHasID2, "HAS_ID2"},
	{iup.AttribCallback, "CALLBACK"},
	{iup.AttribNoSave, "NO_SAVE"},
	{iup.AttribNotSupported, "NOT_SUPPORTED"},
	{iup.AttribIhandleName, "IHANDLENAME"},
	{iup.AttribIhandle, "IHANDLE"},
}

var globalFlagName = []struct {
	bit  int
	name string
}{
	{iup.GlobalReadOnly, "READONLY"},
	{iup.GlobalPointer, "POINTER"},
}

var driverName = []struct {
	bit  int
	name string
}{
	{iup.DriverWin, "Win32"},
	{iup.DriverMotif, "Motif"},
	{iup.DriverGTK, "GTK"},
	{iup.DriverCocoa, "Cocoa"},
	{iup.DriverQt, "Qt"},
	{iup.DriverGTK4, "GTK4"},
	{iup.DriverEFL, "EFL"},
	{iup.DriverWinUI, "WinUI"},
	{iup.DriverFLTK, "FLTK"},
	{iup.DriverAndroid, "Android"},
	{iup.DriverCocoaTouch, "CocoaTouch"},
}

var ctorChar = map[byte]string{
	'b': "uchar",
	'j': "int*",
	'f': "float",
	'i': "int",
	'c': "uchar*",
	's': "char*",
	'a': "char* /*action*/",
	'h': "Ihandle*",
	'g': "Ihandle**",
}

var formatChar = map[byte]string{
	'c': "uchar",
	'i': "int",
	'I': "int*",
	'f': "float",
	'd': "double",
	's': "char*",
	'V': "void*",
	'n': "Ihandle*",
}

func flagsString(flags int) string {
	if flags == iup.AttribDefault {
		return "DEFAULT"
	}
	parts := make([]string, 0, 4)
	for _, f := range attribFlagName {
		if flags&f.bit != 0 {
			parts = append(parts, f.name)
		}
	}
	return strings.Join(parts, "|")
}

func globalFlagsString(flags int) string {
	if flags == iup.GlobalNormal {
		return "-"
	}
	parts := make([]string, 0, 2)
	for _, f := range globalFlagName {
		if flags&f.bit != 0 {
			parts = append(parts, f.name)
		}
	}
	return strings.Join(parts, "|")
}

func driversString(drivers int) string {
	parts := make([]string, 0, 11)
	for _, d := range driverName {
		if drivers&d.bit != 0 {
			parts = append(parts, d.name)
		}
	}
	if len(parts) == 0 {
		return "-"
	}
	if len(parts) == len(driverName) {
		return "all"
	}
	return strings.Join(parts, ",")
}

func childString(child int) string {
	switch {
	case child == 0:
		return "none"
	case child < 0:
		return "unlimited"
	default:
		return fmt.Sprintf("max=%d", child)
	}
}

func boolString(b bool) string {
	if b {
		return "yes"
	}
	return "no"
}

func constructorString(className string, c iup.ClassConstructor) string {
	parts := make([]string, 0, len(c.Format))
	for i := 0; i < len(c.Format); i++ {
		ch := c.Format[i]
		if name, ok := ctorChar[ch]; ok {
			// format_attr only applies to the first parameter (Iclass.format_attr docs).
			if i == 0 && (ch == 's' || ch == 'a') && c.FormatAttr != "" {
				parts = append(parts, fmt.Sprintf("%s /*%s*/", name, c.FormatAttr))
			} else {
				parts = append(parts, name)
			}
		} else {
			parts = append(parts, string(ch))
		}
	}
	cap := className
	if cap != "" {
		cap = strings.ToUpper(cap[:1]) + cap[1:]
	}
	return fmt.Sprintf("Iup%s(%s) -> Ihandle*", cap, strings.Join(parts, ", "))
}

func formatSignature(format string) string {
	if format == "" {
		return "(Ihandle*) -> int"
	}

	params, ret := format, "int"
	if i := strings.Index(format, "="); i >= 0 {
		params = format[:i]
		ret = format[i+1:]
		if len(ret) == 1 {
			if r, ok := formatChar[ret[0]]; ok {
				ret = r
			}
		}
	}

	parts := []string{"Ihandle*"}
	for i := 0; i < len(params); i++ {
		if name, ok := formatChar[params[i]]; ok {
			parts = append(parts, name)
		} else {
			parts = append(parts, string(params[i]))
		}
	}
	return fmt.Sprintf("(%s) -> %s", strings.Join(parts, ", "), ret)
}

func dumpClass(w *tabwriter.Writer, name string) {
	info, ok := iup.GetClassInfo(name)
	if !ok {
		fmt.Fprintf(os.Stderr, "class %q not registered\n", name)
		return
	}

	fmt.Fprintf(w, "\n=== %s ===\n", name)
	parent := info.Parent
	if parent == "" {
		parent = "-"
	}
	fmt.Fprintf(w, "  parent\t%s\n", parent)
	fmt.Fprintf(w, "  native type\t%s\n", info.NativeType)
	fmt.Fprintf(w, "  child type\t%s\n", childString(info.ChildType))
	fmt.Fprintf(w, "  interactive\t%s\n", boolString(info.IsInteractive))
	fmt.Fprintf(w, "  has attrib id\t%d\n", info.HasAttribID)
	if ctor, ok := iup.GetClassConstructor(name); ok {
		fmt.Fprintf(w, "  constructor\t%s\n", constructorString(name, ctor))
	}
	w.Flush()

	attrs := iup.GetClassAttributes(name)
	sort.Strings(attrs)
	if len(attrs) > 0 {
		fmt.Fprintf(w, "\n  Attributes (%d):\n", len(attrs))
		fmt.Fprintln(w, "    NAME\tDEFAULT\tSYSTEM DEFAULT\tFLAGS")
		for _, a := range attrs {
			ai, _ := iup.GetClassAttributeInfo(name, a)
			fmt.Fprintf(w, "    %s\t%s\t%s\t%s\n",
				a, quoteOrDash(ai.DefaultValue), quoteOrDash(ai.SystemDefault), flagsString(ai.Flags))
		}
		w.Flush()
	}

	cbs := iup.GetClassCallbacks(name)
	sort.Strings(cbs)
	if len(cbs) > 0 {
		fmt.Fprintf(w, "\n  Callbacks (%d):\n", len(cbs))
		fmt.Fprintln(w, "    NAME\tFORMAT\tSIGNATURE")
		for _, c := range cbs {
			format := iup.GetClassCallbackFormat(name, c)
			fmt.Fprintf(w, "    %s\t%s\t%s\n", c, quoteOrDash(format), formatSignature(format))
		}
		w.Flush()
	}
}

// nameSet builds a lookup set from positional CLI args. Returns nil when no filter was given.
func nameSet(args []string) map[string]bool {
	if len(args) == 0 {
		return nil
	}
	m := make(map[string]bool, len(args))
	for _, a := range args {
		m[a] = true
	}
	return m
}

func quoteOrDash(s string) string {
	if s == "" {
		return "-"
	}
	return fmt.Sprintf("%q", s)
}

func dumpSummary(w *tabwriter.Writer, classes []string) {
	fmt.Fprintln(w, "CLASS\tPARENT\tNATIVE\tCHILDREN\tFOCUS\tIDS\tATTRS\tCBS\tCONSTRUCTOR")
	for _, name := range classes {
		info, ok := iup.GetClassInfo(name)
		if !ok {
			continue
		}
		parent := info.Parent
		if parent == "" {
			parent = "-"
		}
		ctorSig := "-"
		if ctor, ok := iup.GetClassConstructor(name); ok {
			ctorSig = constructorString(name, ctor)
		}
		fmt.Fprintf(w, "%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%s\n",
			name, parent, info.NativeType, childString(info.ChildType),
			boolString(info.IsInteractive), info.HasAttribID,
			len(iup.GetClassAttributes(name)), len(iup.GetClassCallbacks(name)), ctorSig)
	}
	w.Flush()
}

// dumpCallbacks aggregates callback names across every registered class.
// A name may have several signatures (e.g. ACTION is "" on IupButton,
// "i" on IupToggle, "sii" on IupList); each format gets its own row.
func dumpCallbacks(w *tabwriter.Writer, classes []string, filter []string) {
	wanted := nameSet(filter)
	variants := make(map[string]map[string][]string) // name -> format -> classes
	for _, cls := range classes {
		for _, cb := range iup.GetClassCallbacks(cls) {
			if wanted != nil && !wanted[cb] {
				continue
			}
			format := iup.GetClassCallbackFormat(cls, cb)
			byFormat, ok := variants[cb]
			if !ok {
				byFormat = make(map[string][]string)
				variants[cb] = byFormat
			}
			byFormat[format] = append(byFormat[format], cls)
		}
	}

	names := make([]string, 0, len(variants))
	rows := 0
	for name, byFormat := range variants {
		names = append(names, name)
		rows += len(byFormat)
	}
	sort.Strings(names)

	fmt.Fprintf(w, "\n=== callbacks (%d unique names, %d signatures) ===\n", len(names), rows)
	fmt.Fprintln(w, "NAME\tFORMAT\tSIGNATURE\tCLASSES")
	for _, name := range names {
		byFormat := variants[name]
		formats := make([]string, 0, len(byFormat))
		for f := range byFormat {
			formats = append(formats, f)
		}
		sort.Strings(formats)
		for _, f := range formats {
			fmt.Fprintf(w, "%s\t%s\t%s\t%s\n",
				name, quoteOrDash(f), formatSignature(f), strings.Join(byFormat[f], ","))
		}
	}
	w.Flush()
}

// dumpAttributes aggregates attribute names across every registered class.
// A name may have several flag variants (e.g. an attribute that is
// NO_INHERIT on one class but DEFAULT on another); each flag set gets its own row.
func dumpAttributes(w *tabwriter.Writer, classes []string, filter []string) {
	wanted := nameSet(filter)
	variants := make(map[string]map[int][]string) // name -> flags -> classes
	for _, cls := range classes {
		for _, a := range iup.GetClassAttributes(cls) {
			if wanted != nil && !wanted[a] {
				continue
			}
			info, ok := iup.GetClassAttributeInfo(cls, a)
			if !ok {
				continue
			}
			byFlags, ok := variants[a]
			if !ok {
				byFlags = make(map[int][]string)
				variants[a] = byFlags
			}
			byFlags[info.Flags] = append(byFlags[info.Flags], cls)
		}
	}

	names := make([]string, 0, len(variants))
	rows := 0
	for name, byFlags := range variants {
		names = append(names, name)
		rows += len(byFlags)
	}
	sort.Strings(names)

	fmt.Fprintf(w, "\n=== attributes (%d unique names, %d signatures) ===\n", len(names), rows)
	fmt.Fprintln(w, "NAME\tFLAGS\tCLASSES")
	for _, name := range names {
		byFlags := variants[name]
		flagSets := make([]int, 0, len(byFlags))
		for f := range byFlags {
			flagSets = append(flagSets, f)
		}
		sort.Ints(flagSets)
		for _, f := range flagSets {
			fmt.Fprintf(w, "%s\t%s\t%s\n", name, flagsString(f), strings.Join(byFlags[f], ","))
		}
	}
	w.Flush()
}

func dumpGlobals(w *tabwriter.Writer) {
	names := iup.GetAllGlobals()
	sort.Strings(names)

	fmt.Fprintf(w, "\n=== globals (%d) ===\n", len(names))
	fmt.Fprintln(w, "NAME\tVALUE\tFLAGS\tDRIVERS")
	for _, name := range names {
		info, ok := iup.GetGlobalInfo(name)
		flags, drivers := "ad-hoc", "-"
		if ok {
			flags = globalFlagsString(info.Flags)
			drivers = driversString(info.Drivers)
		}
		fmt.Fprintf(w, "%s\t%s\t%s\t%s\n", name, quoteOrDash(iup.GetGlobal(name)), flags, drivers)
	}
	w.Flush()
}

func usage() {
	out := flag.CommandLine.Output()
	fmt.Fprint(out, `iupinfo dumps the IUP class and global registries.

Usage:
  iupinfo                          one line per class (default)
  iupinfo <class>...               full dump of the named classes
  iupinfo --full                   full dump of every class + globals
  iupinfo --globals                dump only the globals section
  iupinfo --callbacks [name...]    aggregated callback view, optional filter
  iupinfo --attributes [name...]   aggregated attribute view, optional filter

Examples:
  iupinfo button text                  full dump of two classes
  iupinfo --callbacks ACTION           show every variant of ACTION
  iupinfo --attributes BGCOLOR FONT    show only those two attributes
  iupinfo --globals                    list every registered global
`)
}

func main() {
	flag.Usage = usage
	full := flag.Bool("full", false, "full dump of every class and the globals registry")
	globalsOnly := flag.Bool("globals", false, "dump only the globals section")
	callbacksOnly := flag.Bool("callbacks", false, "dump every callback name aggregated across all classes")
	attributesOnly := flag.Bool("attributes", false, "dump every attribute name aggregated across all classes")
	flag.Parse()

	iup.Open()
	defer iup.Close()

	for _, fn := range extraOpens {
		fn()
	}

	w := tabwriter.NewWriter(os.Stdout, 0, 2, 2, ' ', 0)

	classes := iup.GetAllClasses()
	sort.Strings(classes)

	if *globalsOnly {
		dumpGlobals(w)
		return
	}
	if *callbacksOnly {
		dumpCallbacks(w, classes, flag.Args())
		return
	}
	if *attributesOnly {
		dumpAttributes(w, classes, flag.Args())
		return
	}

	if flag.NArg() > 0 {
		for _, name := range flag.Args() {
			dumpClass(w, name)
		}
		return
	}

	fmt.Fprintf(w, "IUP %s, %d classes, %d globals\n",
		iup.Version(), len(classes), len(iup.GetAllGlobals()))

	if !*full {
		fmt.Fprintln(w)
		dumpSummary(w, classes)
		return
	}

	for _, name := range classes {
		dumpClass(w, name)
	}

	dumpGlobals(w)
}
