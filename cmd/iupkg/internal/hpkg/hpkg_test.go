package hpkg

import (
	"bytes"
	"compress/zlib"
	"encoding/binary"
	"image"
	"image/color"
	"io"
	"testing"
)

func readLEB128(b []byte, off int) (uint64, int) {
	var v uint64
	for shift := 0; ; shift += 7 {
		c := b[off]
		off++
		v |= uint64(c&0x7f) << shift
		if c&0x80 == 0 {
			return v, off
		}
	}
}

type hpkgNode struct {
	id       uint16
	str      string
	num      uint64
	raw      []byte
	children []hpkgNode
}

func parseHPKGAttrs(heap, b []byte, off int) ([]hpkgNode, int) {
	var nodes []hpkgNode
	for {
		tag, next := readLEB128(b, off)
		off = next
		if tag == 0 {
			return nodes, off
		}
		tag--
		n := hpkgNode{id: uint16(tag & 0x7f)}
		typ, enc, children := (tag>>7)&7, (tag>>11)&3, tag&(1<<10) != 0
		switch typ {
		case hpkgTypeUint:
			size := 1 << enc
			for i := range size {
				n.num = n.num<<8 | uint64(b[off+i])
			}
			off += size
		case hpkgTypeString:
			end := bytes.IndexByte(b[off:], 0)
			n.str = string(b[off : off+end])
			off += end + 1
		case hpkgTypeRaw:
			size, next := readLEB128(b, off)
			hoff, next := readLEB128(b, next)
			off = next
			n.raw = heap[hoff : hoff+size]
		}
		if children {
			n.children, off = parseHPKGAttrs(heap, b, off)
		}
		nodes = append(nodes, n)
	}
}

func TestWriteHPKG(t *testing.T) {
	root := &Entry{Dir: true}
	apps := root.Add(&Entry{Name: "apps", Dir: true})
	big := bytes.Repeat([]byte("iup "), 40000)
	apps.Add(&Entry{Name: "demo", Mode: 0o755, Data: big, Attrs: []FileAttr{{"BEOS:APP_SIG", MimeString, []byte("application/x-vnd.demo\x00")}}})
	root.Add(&Entry{Name: "link", Link: "../apps/demo"})
	info := Info{Name: "demo", Version: [3]string{"1", "2", "3"}, Revision: 4, Summary: "s", Description: "d", Vendor: "v", Packager: "p",
		Copyright: "c", License: "l", Provides: []string{"demo", "app:demo"}, Requires: []string{"haiku"}}
	data, err := Write(root, info, ArchX86_64)
	if err != nil {
		t.Fatal(err)
	}

	hdr := data[:80]
	if string(hdr[:4]) != "hpkg" || binary.BigEndian.Uint64(hdr[8:]) != uint64(len(data)) {
		t.Fatal("bad header")
	}
	heapC := int(binary.BigEndian.Uint64(hdr[24:]))
	heapU := int(binary.BigEndian.Uint64(hdr[32:]))
	attrsLen := int(binary.BigEndian.Uint32(hdr[40:]))
	tocLen := int(binary.BigEndian.Uint64(hdr[56:]))
	chunks := (heapU + hpkgChunkSize - 1) / hpkgChunkSize
	table := data[80+heapC-2*(chunks-1) : 80+heapC]
	var heap []byte
	off := 80
	for i := range chunks {
		size := heapC - 2*(chunks-1) - (off - 80)
		if i < chunks-1 {
			size = int(binary.BigEndian.Uint16(table[2*i:])) + 1
		}
		chunk := data[off : off+size]
		off += size
		want := min(hpkgChunkSize, heapU-len(heap))
		if size == want {
			heap = append(heap, chunk...)
			continue
		}
		zr, err := zlib.NewReader(bytes.NewReader(chunk))
		if err != nil {
			t.Fatalf("chunk %d: %v", i, err)
		}
		dec, err := io.ReadAll(zr)
		if err != nil || len(dec) != want {
			t.Fatalf("chunk %d: %v, %d bytes", i, err, len(dec))
		}
		heap = append(heap, dec...)
	}
	if len(heap) != heapU {
		t.Fatalf("heap %d, want %d", len(heap), heapU)
	}

	toc := heap[heapU-attrsLen-tocLen : heapU-attrsLen]
	entries, _ := parseHPKGAttrs(heap, toc, 1)
	if len(entries) != 2 || entries[0].str != "apps" || entries[1].str != "link" {
		t.Fatalf("toc entries %+v", entries)
	}
	demo := entries[0].children[len(entries[0].children)-1]
	if demo.children[len(demo.children)-1].id != hpkgAttrFileAttr {
		t.Error("file attributes must follow the data")
	}
	if demo.str != "demo" {
		t.Fatalf("demo entry %+v", demo)
	}
	var gotData, gotAttr []byte
	for _, n := range demo.children {
		switch n.id {
		case hpkgAttrData:
			gotData = n.raw
		case hpkgAttrFileAttr:
			gotAttr = n.children[1].raw
		}
	}
	if !bytes.Equal(gotData, big) || string(gotAttr) != "application/x-vnd.demo\x00" {
		t.Error("file data or attribute mismatch")
	}
	link := entries[1].children[len(entries[1].children)-1]
	if link.id != hpkgAttrSymlinkPath || link.str != "../apps/demo" {
		t.Errorf("symlink %+v", link)
	}

	pkg, _ := parseHPKGAttrs(heap, heap[heapU-attrsLen:], 1)
	names := map[uint16]string{}
	for _, n := range pkg {
		names[n.id] = n.str
	}
	if names[hpkgAttrPkgName] != "demo" || names[hpkgAttrPkgVendor] != "v" {
		t.Errorf("package attrs %+v", names)
	}
	for _, n := range pkg {
		if n.id == hpkgAttrVersionMajor {
			if n.str != "1" || n.children[0].str != "2" || n.children[1].str != "3" || n.children[2].num != 4 {
				t.Errorf("version %+v", n)
			}
		}
	}
}

func TestHaikuBitmapIcon(t *testing.T) {
	img := image.NewNRGBA(image.Rect(0, 0, 4, 4))
	img.Set(0, 0, color.NRGBA{255, 0, 0, 255})
	icon := BitmapIcon(img, 16)
	if len(icon) != 256 || icon[0] == 255 || icon[255] != 255 {
		t.Errorf("icon = len %d, first %d, last %d", len(icon), icon[0], icon[255])
	}
	p := palette[icon[0]]
	if p[0] < 200 || p[1] > 60 || p[2] > 60 {
		t.Errorf("red mapped to %v", p)
	}
}
