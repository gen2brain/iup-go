package hpkg

import (
	"bytes"
	"compress/zlib"
	"encoding/binary"
	"fmt"
	"image"
	"strconv"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/icon"
)

const (
	hpkgAttrDirEntry     = 0
	hpkgAttrFileType     = 1
	hpkgAttrFilePerms    = 2
	hpkgAttrFileMtime    = 6
	hpkgAttrFileAttr     = 11
	hpkgAttrFileAttrType = 12
	hpkgAttrData         = 13
	hpkgAttrSymlinkPath  = 14
	hpkgAttrPkgName      = 15
	hpkgAttrPkgSummary   = 16
	hpkgAttrPkgDesc      = 17
	hpkgAttrPkgVendor    = 18
	hpkgAttrPkgPackager  = 19
	hpkgAttrPkgArch      = 21
	hpkgAttrVersionMajor = 22
	hpkgAttrVersionMinor = 23
	hpkgAttrVersionMicro = 24
	hpkgAttrVersionRev   = 25
	hpkgAttrPkgCopyright = 26
	hpkgAttrPkgLicense   = 27
	hpkgAttrPkgProvides  = 28
	hpkgAttrPkgRequires  = 29

	hpkgTypeUint   = 2
	hpkgTypeString = 3
	hpkgTypeRaw    = 4

	hpkgFileTypeDir     = 1
	hpkgFileTypeSymlink = 2

	hpkgChunkSize = 64 * 1024

	MimeString    = 0x4D494D53
	LargeIconType = 0x49434F4E
	MiniIconType  = 0x4D49434E
	ArchX86_64    = 4
)

type FileAttr struct {
	Name string
	Type uint32
	Data []byte
}

type Entry struct {
	Name     string
	Dir      bool
	Link     string
	Mode     uint32
	Data     []byte
	Attrs    []FileAttr
	Children []*Entry
}

func (e *Entry) Add(child *Entry) *Entry {
	e.Children = append(e.Children, child)
	return child
}

type Info struct {
	Name        string
	Version     [3]string
	Revision    uint32
	Summary     string
	Description string
	Vendor      string
	Packager    string
	Copyright   string
	License     string
	Provides    []string
	Requires    []string
}

type hpkgWriter struct {
	heap bytes.Buffer
}

func leb128(v uint64) []byte {
	var out []byte
	for {
		b := byte(v & 0x7f)
		v >>= 7
		if v != 0 {
			b |= 0x80
		}
		out = append(out, b)
		if v == 0 {
			return out
		}
	}
}

func hpkgTag(id, typ, enc uint16, children bool) []byte {
	tag := enc<<11 | typ<<7 | id
	if children {
		tag |= 1 << 10
	}
	return leb128(uint64(tag + 1))
}

func hpkgUint(buf *bytes.Buffer, id uint16, v uint64, children bool) {
	switch {
	case v <= 0xff:
		buf.Write(hpkgTag(id, hpkgTypeUint, 0, children))
		buf.WriteByte(byte(v))
	case v <= 0xffff:
		buf.Write(hpkgTag(id, hpkgTypeUint, 1, children))
		buf.Write(binary.BigEndian.AppendUint16(nil, uint16(v)))
	case v <= 0xffffffff:
		buf.Write(hpkgTag(id, hpkgTypeUint, 2, children))
		buf.Write(binary.BigEndian.AppendUint32(nil, uint32(v)))
	default:
		buf.Write(hpkgTag(id, hpkgTypeUint, 3, children))
		buf.Write(binary.BigEndian.AppendUint64(nil, v))
	}
}

func hpkgString(buf *bytes.Buffer, id uint16, s string, children bool) {
	buf.Write(hpkgTag(id, hpkgTypeString, 0, children))
	buf.WriteString(s)
	buf.WriteByte(0)
}

func (w *hpkgWriter) hpkgRaw(buf *bytes.Buffer, id uint16, data []byte, children bool) {
	offset := w.heap.Len()
	w.heap.Write(data)
	buf.Write(hpkgTag(id, hpkgTypeRaw, 1, children))
	buf.Write(leb128(uint64(len(data))))
	buf.Write(leb128(uint64(offset)))
}

func (w *hpkgWriter) writeEntry(buf *bytes.Buffer, e *Entry, mtime uint64) {
	hpkgString(buf, hpkgAttrDirEntry, e.Name, true)
	switch {
	case e.Dir:
		hpkgUint(buf, hpkgAttrFileType, hpkgFileTypeDir, false)
	case e.Link != "":
		hpkgUint(buf, hpkgAttrFileType, hpkgFileTypeSymlink, false)
	}
	if e.Mode != 0 {
		hpkgUint(buf, hpkgAttrFilePerms, uint64(e.Mode), false)
	}
	hpkgUint(buf, hpkgAttrFileMtime, mtime, false)
	switch {
	case e.Dir:
		for _, child := range e.Children {
			w.writeEntry(buf, child, mtime)
		}
	case e.Link != "":
		hpkgString(buf, hpkgAttrSymlinkPath, e.Link, false)
	default:
		w.hpkgRaw(buf, hpkgAttrData, e.Data, false)
	}
	for _, a := range e.Attrs {
		hpkgString(buf, hpkgAttrFileAttr, a.Name, true)
		hpkgUint(buf, hpkgAttrFileAttrType, uint64(a.Type), false)
		w.hpkgRaw(buf, hpkgAttrData, a.Data, false)
		buf.WriteByte(0)
	}
	buf.WriteByte(0)
}

func hpkgVersion(buf *bytes.Buffer, id uint16, v [3]string, rev uint32) {
	hpkgString(buf, id, v[0], true)
	if v[1] != "" {
		hpkgString(buf, hpkgAttrVersionMinor, v[1], false)
		if v[2] != "" {
			hpkgString(buf, hpkgAttrVersionMicro, v[2], false)
		}
	}
	if rev != 0 {
		buf.Write(hpkgTag(hpkgAttrVersionRev, hpkgTypeUint, 2, false))
		buf.Write(binary.BigEndian.AppendUint32(nil, rev))
	}
	buf.WriteByte(0)
}

func Write(root *Entry, info Info, arch uint64) ([]byte, error) {
	w := &hpkgWriter{}
	mtime := uint64(time.Now().Unix())

	var toc bytes.Buffer
	toc.WriteByte(0)
	tocStrings := toc.Len()
	for _, e := range root.Children {
		w.writeEntry(&toc, e, mtime)
	}
	toc.WriteByte(0)

	var attrs bytes.Buffer
	attrs.WriteByte(0)
	attrStrings := attrs.Len()
	hpkgString(&attrs, hpkgAttrPkgName, info.Name, false)
	hpkgString(&attrs, hpkgAttrPkgSummary, info.Summary, false)
	hpkgString(&attrs, hpkgAttrPkgDesc, info.Description, false)
	hpkgString(&attrs, hpkgAttrPkgVendor, info.Vendor, false)
	hpkgString(&attrs, hpkgAttrPkgPackager, info.Packager, false)
	hpkgUint(&attrs, hpkgAttrPkgArch, arch, false)
	hpkgVersion(&attrs, hpkgAttrVersionMajor, info.Version, info.Revision)
	hpkgString(&attrs, hpkgAttrPkgCopyright, info.Copyright, false)
	hpkgString(&attrs, hpkgAttrPkgLicense, info.License, false)
	for _, p := range info.Provides {
		hpkgString(&attrs, hpkgAttrPkgProvides, p, true)
		hpkgVersion(&attrs, hpkgAttrVersionMajor, info.Version, 0)
		attrs.WriteByte(0)
	}
	for _, r := range info.Requires {
		hpkgString(&attrs, hpkgAttrPkgRequires, r, false)
	}
	attrs.WriteByte(0)

	w.heap.Write(toc.Bytes())
	w.heap.Write(attrs.Bytes())
	heap := w.heap.Bytes()

	var out bytes.Buffer
	out.Write(make([]byte, 80))
	var sizes []uint16
	for off := 0; off < len(heap); off += hpkgChunkSize {
		chunk := heap[off:min(off+hpkgChunkSize, len(heap))]
		data := chunk
		if len(chunk) >= 64 {
			var zb bytes.Buffer
			zw, err := zlib.NewWriterLevel(&zb, zlib.BestCompression)
			if err != nil {
				return nil, err
			}
			zw.Write(chunk)
			zw.Close()
			if zb.Len() < len(chunk) {
				data = zb.Bytes()
			}
		}
		out.Write(data)
		sizes = append(sizes, uint16(len(data)-1))
	}
	for _, s := range sizes[:len(sizes)-1] {
		out.Write(binary.BigEndian.AppendUint16(nil, s))
	}

	hdr := out.Bytes()[:80]
	copy(hdr, "hpkg")
	binary.BigEndian.PutUint16(hdr[4:], 80)
	binary.BigEndian.PutUint16(hdr[6:], 2)
	binary.BigEndian.PutUint64(hdr[8:], uint64(out.Len()))
	binary.BigEndian.PutUint16(hdr[16:], 1)
	binary.BigEndian.PutUint16(hdr[18:], 1)
	binary.BigEndian.PutUint32(hdr[20:], hpkgChunkSize)
	binary.BigEndian.PutUint64(hdr[24:], uint64(out.Len()-80))
	binary.BigEndian.PutUint64(hdr[32:], uint64(len(heap)))
	binary.BigEndian.PutUint32(hdr[40:], uint32(attrs.Len()))
	binary.BigEndian.PutUint32(hdr[44:], uint32(attrStrings))
	binary.BigEndian.PutUint64(hdr[56:], uint64(toc.Len()))
	binary.BigEndian.PutUint64(hdr[64:], uint64(tocStrings))
	return out.Bytes(), nil
}

func BitmapIcon(img image.Image, size int) []byte {
	small := icon.Resize(img, size)
	out := make([]byte, size*size)
	for y := range size {
		for x := range size {
			c := small.NRGBAAt(x, y)
			if c.A < 128 {
				out[y*size+x] = 255
				continue
			}
			best, bestDist := 0, 1<<30
			for i, p := range palette[:255] {
				dr, dg, db := int(c.R)-int(p[0]), int(c.G)-int(p[1]), int(c.B)-int(p[2])
				if d := dr*dr + dg*dg + db*db; d < bestDist {
					best, bestDist = i, d
				}
			}
			out[y*size+x] = byte(best)
		}
	}
	return out
}

func PackageName(s string) string {
	var b strings.Builder
	for _, r := range strings.ToLower(s) {
		if r >= 'a' && r <= 'z' || r >= '0' && r <= '9' || r == '_' {
			b.WriteRune(r)
		} else {
			b.WriteByte('_')
		}
	}
	return b.String()
}

func ParseVersion(v string) ([3]string, error) {
	var out [3]string
	parts := strings.SplitN(v, ".", 3)
	for i, p := range parts {
		for _, r := range p {
			if !(r >= 'a' && r <= 'z' || r >= 'A' && r <= 'Z' || r >= '0' && r <= '9' || r == '_' || i == 2 && r == '.') {
				return out, fmt.Errorf("version %q: haiku versions are major.minor.micro of letters, digits and _", v)
			}
		}
		out[i] = p
	}
	if out[0] == "" {
		return out, fmt.Errorf("version %q: empty major", v)
	}
	return out, nil
}

func VersionString(v [3]string) string {
	s := v[0]
	for _, p := range v[1:] {
		if p == "" {
			break
		}
		s += "." + p
	}
	return s
}

func InfoText(info Info, arch string) []byte {
	var b strings.Builder
	q := func(s string) string { return strconv.Quote(s) }
	fmt.Fprintf(&b, "name\t\t%s\n", info.Name)
	fmt.Fprintf(&b, "version\t\t%s-%d\n", VersionString(info.Version), info.Revision)
	fmt.Fprintf(&b, "architecture\t%s\n", arch)
	fmt.Fprintf(&b, "summary\t\t%s\n", q(info.Summary))
	fmt.Fprintf(&b, "description\t%s\n", q(info.Description))
	fmt.Fprintf(&b, "vendor\t\t%s\n", q(info.Vendor))
	fmt.Fprintf(&b, "packager\t%s\n", q(info.Packager))
	fmt.Fprintf(&b, "copyrights\t{ %s }\n", q(info.Copyright))
	fmt.Fprintf(&b, "licenses\t{ %s }\n", q(info.License))
	b.WriteString("provides {\n")
	for _, p := range info.Provides {
		fmt.Fprintf(&b, "\t%s = %s\n", p, VersionString(info.Version))
	}
	b.WriteString("}\nrequires {\n")
	for _, r := range info.Requires {
		fmt.Fprintf(&b, "\t%s\n", r)
	}
	b.WriteString("}\n")
	return []byte(b.String())
}
