package deb

import (
	"archive/tar"
	"bytes"
	"compress/gzip"
	"crypto/md5"
	"fmt"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/pkgtree"
)

type Package struct {
	Name        string
	Version     string
	Arch        string
	Maintainer  string
	Description string
	Section     string
	Files       []pkgtree.File
}

func Write(p Package) ([]byte, error) {
	now := time.Now()
	files := pkgtree.WithDirs(p.Files)

	var size int64
	var sums bytes.Buffer
	for _, f := range files {
		if f.Dir || f.Link != "" {
			continue
		}
		size += int64(len(f.Data))
		fmt.Fprintf(&sums, "%x  %s\n", md5.Sum(f.Data), strings.TrimPrefix(f.Path, "/"))
	}

	control := fmt.Sprintf("Package: %s\nVersion: %s\nArchitecture: %s\nMaintainer: %s\nInstalled-Size: %d\nSection: %s\nPriority: optional\nDescription: %s\n",
		p.Name, p.Version, p.Arch, p.Maintainer, (size+1023)/1024, p.Section, p.Description)

	controlTar, err := tarGz([]pkgtree.File{
		{Path: "/control", Mode: 0o644, Data: []byte(control)},
		{Path: "/md5sums", Mode: 0o644, Data: sums.Bytes()},
	}, now)
	if err != nil {
		return nil, err
	}
	dataTar, err := tarGz(files, now)
	if err != nil {
		return nil, err
	}

	var out bytes.Buffer
	out.WriteString("!<arch>\n")
	arMember(&out, "debian-binary", []byte("2.0\n"), now)
	arMember(&out, "control.tar.gz", controlTar, now)
	arMember(&out, "data.tar.gz", dataTar, now)
	return out.Bytes(), nil
}

func arMember(out *bytes.Buffer, name string, data []byte, now time.Time) {
	fmt.Fprintf(out, "%-16s%-12d%-6d%-6d%-8s%-10d`\n", name, now.Unix(), 0, 0, "100644", len(data))
	out.Write(data)
	if len(data)%2 == 1 {
		out.WriteByte('\n')
	}
}

func tarGz(files []pkgtree.File, now time.Time) ([]byte, error) {
	var buf bytes.Buffer
	gw := gzip.NewWriter(&buf)
	tw := tar.NewWriter(gw)
	for _, f := range files {
		hdr := &tar.Header{Name: "." + f.Path, Mode: int64(f.Mode), ModTime: now, Uname: "root", Gname: "root", Format: tar.FormatGNU}
		switch {
		case f.Dir:
			hdr.Typeflag = tar.TypeDir
			hdr.Name += "/"
		case f.Link != "":
			hdr.Typeflag = tar.TypeSymlink
			hdr.Linkname = f.Link
		default:
			hdr.Typeflag = tar.TypeReg
			hdr.Size = int64(len(f.Data))
		}
		if err := tw.WriteHeader(hdr); err != nil {
			return nil, err
		}
		if hdr.Typeflag == tar.TypeReg {
			if _, err := tw.Write(f.Data); err != nil {
				return nil, err
			}
		}
	}
	if err := tw.Close(); err != nil {
		return nil, err
	}
	if err := gw.Close(); err != nil {
		return nil, err
	}
	return buf.Bytes(), nil
}
