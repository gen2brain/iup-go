package rpm

import (
	"bytes"
	"compress/gzip"
	"crypto/md5"
	"crypto/sha1"
	"crypto/sha256"
	"encoding/binary"
	"fmt"
	"path"
	"slices"
	"time"

	"github.com/gen2brain/iup-go/cmd/iupkg/internal/pkgtree"
)

type Package struct {
	Name        string
	Version     string
	Release     string
	Arch        string
	Summary     string
	Description string
	License     string
	Vendor      string
	URL         string
	Files       []pkgtree.File
}

const (
	typeInt16       = 3
	typeInt32       = 4
	typeString      = 6
	typeBin         = 7
	typeStringArray = 8
	typeI18NString  = 9

	tagHeaderSignatures = 62
	tagHeaderImmutable  = 63
	tagHeaderI18NTable  = 100
	tagSigSize          = 1000
	tagSigMD5           = 1004
	tagSigSHA1          = 269
	tagSigSHA256        = 273

	tagName             = 1000
	tagVersion          = 1001
	tagRelease          = 1002
	tagSummary          = 1004
	tagDescription      = 1005
	tagBuildTime        = 1006
	tagBuildHost        = 1007
	tagSize             = 1009
	tagVendor           = 1011
	tagLicense          = 1014
	tagGroup            = 1016
	tagURL              = 1020
	tagOS               = 1021
	tagArch             = 1022
	tagFileSizes        = 1028
	tagFileModes        = 1030
	tagFileRdevs        = 1033
	tagFileMtimes       = 1034
	tagFileDigests      = 1035
	tagFileLinktos      = 1036
	tagFileFlags        = 1037
	tagFileUsername     = 1039
	tagFileGroupname    = 1040
	tagSourceRPM        = 1044
	tagFileVerifyFlags  = 1045
	tagProvideName      = 1047
	tagRequireFlags     = 1048
	tagRequireName      = 1049
	tagRequireVersion   = 1050
	tagFileDevices      = 1095
	tagFileInodes       = 1096
	tagFileLangs        = 1097
	tagProvideFlags     = 1112
	tagProvideVersion   = 1113
	tagDirIndexes       = 1116
	tagBasenames        = 1117
	tagDirnames         = 1118
	tagPayloadFormat    = 1124
	tagPayloadCompress  = 1125
	tagPayloadFlags     = 1126
	tagFileDigestAlgo   = 5011
	tagPayloadDigest    = 5092
	tagPayloadDigestAlg = 5093
	tagPayloadDigestAlt = 5097

	senseLess   = 2
	senseEqual  = 8
	senseRPMLib = 1 << 24
)

type entry struct {
	tag   int32
	typ   int32
	count int32
	data  []byte
}

func str(tag int32, s string) entry { return entry{tag, typeString, 1, append([]byte(s), 0)} }

func i18n(tag int32, s string) entry { return entry{tag, typeI18NString, 1, append([]byte(s), 0)} }

func strs(tag int32, list []string) entry {
	var b []byte
	for _, s := range list {
		b = append(append(b, s...), 0)
	}
	return entry{tag, typeStringArray, int32(len(list)), b}
}

func int32s(tag int32, list []int32) entry {
	var b []byte
	for _, v := range list {
		b = binary.BigEndian.AppendUint32(b, uint32(v))
	}
	return entry{tag, typeInt32, int32(len(list)), b}
}

func int16s(tag int32, list []int16) entry {
	var b []byte
	for _, v := range list {
		b = binary.BigEndian.AppendUint16(b, uint16(v))
	}
	return entry{tag, typeInt16, int32(len(list)), b}
}

func bin(tag int32, b []byte) entry { return entry{tag, typeBin, int32(len(b)), b} }

func align(typ int32) int {
	switch typ {
	case typeInt16:
		return 2
	case typeInt32:
		return 4
	}
	return 1
}

func header(regionTag int32, entries []entry) []byte {
	slices.SortFunc(entries, func(a, b entry) int { return int(a.tag - b.tag) })

	var data bytes.Buffer
	var index bytes.Buffer
	for _, e := range entries {
		for data.Len()%align(e.typ) != 0 {
			data.WriteByte(0)
		}
		index.Write(binary.BigEndian.AppendUint32(nil, uint32(e.tag)))
		index.Write(binary.BigEndian.AppendUint32(nil, uint32(e.typ)))
		index.Write(binary.BigEndian.AppendUint32(nil, uint32(data.Len())))
		index.Write(binary.BigEndian.AppendUint32(nil, uint32(e.count)))
		data.Write(e.data)
	}

	nindex := int32(len(entries) + 1)
	trailer := binary.BigEndian.AppendUint32(nil, uint32(regionTag))
	trailer = binary.BigEndian.AppendUint32(trailer, typeBin)
	trailer = binary.BigEndian.AppendUint32(trailer, uint32(-nindex*16))
	trailer = binary.BigEndian.AppendUint32(trailer, 16)
	region := binary.BigEndian.AppendUint32(nil, uint32(regionTag))
	region = binary.BigEndian.AppendUint32(region, typeBin)
	region = binary.BigEndian.AppendUint32(region, uint32(data.Len()))
	region = binary.BigEndian.AppendUint32(region, 16)
	data.Write(trailer)

	var out bytes.Buffer
	out.Write([]byte{0x8e, 0xad, 0xe8, 0x01, 0, 0, 0, 0})
	out.Write(binary.BigEndian.AppendUint32(nil, uint32(nindex)))
	out.Write(binary.BigEndian.AppendUint32(nil, uint32(data.Len())))
	out.Write(region)
	out.Write(index.Bytes())
	out.Write(data.Bytes())
	return out.Bytes()
}

func Write(p Package) ([]byte, error) {
	now := int32(time.Now().Unix())
	files := p.Files

	var dirnames []string
	dirIndex := map[string]int32{}
	var basenames, digests, linktos, users, groups, langs []string
	var sizes, mtimes, flags, verify, devices, inodes, dirIndexes []int32
	var modes, rdevs []int16
	var total int32

	var cpio bytes.Buffer
	for i, f := range files {
		dir := path.Dir(f.Path) + "/"
		if dir == "//" {
			dir = "/"
		}
		if _, ok := dirIndex[dir]; !ok {
			dirIndex[dir] = int32(len(dirnames))
			dirnames = append(dirnames, dir)
		}
		dirIndexes = append(dirIndexes, dirIndex[dir])
		basenames = append(basenames, path.Base(f.Path))
		users = append(users, "root")
		groups = append(groups, "root")
		langs = append(langs, "")
		mtimes = append(mtimes, now)
		flags = append(flags, 0)
		verify = append(verify, -1)
		devices = append(devices, 1)
		inodes = append(inodes, int32(i+1))
		rdevs = append(rdevs, 0)

		mode := f.Mode & 0o7777
		var body []byte
		switch {
		case f.Dir:
			mode |= 0o40000
			sizes = append(sizes, 4096)
			digests = append(digests, "")
			linktos = append(linktos, "")
		case f.Link != "":
			mode |= 0o120000
			body = []byte(f.Link)
			sizes = append(sizes, int32(len(body)))
			digests = append(digests, "")
			linktos = append(linktos, f.Link)
		default:
			mode |= 0o100000
			body = f.Data
			sizes = append(sizes, int32(len(body)))
			digests = append(digests, fmt.Sprintf("%x", sha256.Sum256(body)))
			linktos = append(linktos, "")
			total += int32(len(body))
		}
		modes = append(modes, int16(mode))
		cpioEntry(&cpio, "."+f.Path, mode, int32(i+1), now, body)
	}
	cpioEntry(&cpio, "TRAILER!!!", 0, 0, 0, nil)
	for cpio.Len()%512 != 0 {
		cpio.WriteByte(0)
	}

	var payload bytes.Buffer
	gw, err := gzip.NewWriterLevel(&payload, gzip.BestCompression)
	if err != nil {
		return nil, err
	}
	gw.Write(cpio.Bytes())
	if err := gw.Close(); err != nil {
		return nil, err
	}

	evr := p.Version + "-" + p.Release
	hdr := header(tagHeaderImmutable, []entry{
		strs(tagHeaderI18NTable, []string{"C"}),
		str(tagName, p.Name),
		str(tagVersion, p.Version),
		str(tagRelease, p.Release),
		i18n(tagSummary, p.Summary),
		i18n(tagDescription, p.Description),
		int32s(tagBuildTime, []int32{now}),
		str(tagBuildHost, "iupkg"),
		int32s(tagSize, []int32{total}),
		str(tagVendor, p.Vendor),
		str(tagLicense, p.License),
		i18n(tagGroup, "Applications"),
		str(tagURL, p.URL),
		str(tagOS, "linux"),
		str(tagArch, p.Arch),
		int32s(tagFileSizes, sizes),
		int16s(tagFileModes, modes),
		int16s(tagFileRdevs, rdevs),
		int32s(tagFileMtimes, mtimes),
		strs(tagFileDigests, digests),
		strs(tagFileLinktos, linktos),
		int32s(tagFileFlags, flags),
		strs(tagFileUsername, users),
		strs(tagFileGroupname, groups),
		str(tagSourceRPM, p.Name+"-"+evr+".src.rpm"),
		int32s(tagFileVerifyFlags, verify),
		strs(tagProvideName, []string{p.Name}),
		int32s(tagProvideFlags, []int32{senseEqual}),
		strs(tagProvideVersion, []string{evr}),
		strs(tagRequireName, []string{"rpmlib(CompressedFileNames)", "rpmlib(FileDigests)", "rpmlib(PayloadFilesHavePrefix)"}),
		int32s(tagRequireFlags, []int32{senseRPMLib | senseLess | senseEqual, senseRPMLib | senseLess | senseEqual, senseRPMLib | senseLess | senseEqual}),
		strs(tagRequireVersion, []string{"3.0.4-1", "4.6.0-1", "4.0-1"}),
		int32s(tagFileDevices, devices),
		int32s(tagFileInodes, inodes),
		strs(tagFileLangs, langs),
		int32s(tagDirIndexes, dirIndexes),
		strs(tagBasenames, basenames),
		strs(tagDirnames, dirnames),
		str(tagPayloadFormat, "cpio"),
		str(tagPayloadCompress, "gzip"),
		str(tagPayloadFlags, "9"),
		int32s(tagFileDigestAlgo, []int32{8}),
		strs(tagPayloadDigest, []string{fmt.Sprintf("%x", sha256.Sum256(payload.Bytes()))}),
		int32s(tagPayloadDigestAlg, []int32{8}),
		strs(tagPayloadDigestAlt, []string{fmt.Sprintf("%x", sha256.Sum256(cpio.Bytes()))}),
	})

	sumMD5 := md5.New()
	sumMD5.Write(hdr)
	sumMD5.Write(payload.Bytes())
	sig := header(tagHeaderSignatures, []entry{
		int32s(tagSigSize, []int32{int32(len(hdr) + payload.Len())}),
		bin(tagSigMD5, sumMD5.Sum(nil)),
		str(tagSigSHA1, fmt.Sprintf("%x", sha1.Sum(hdr))),
		str(tagSigSHA256, fmt.Sprintf("%x", sha256.Sum256(hdr))),
	})

	var out bytes.Buffer
	out.Write([]byte{0xed, 0xab, 0xee, 0xdb, 3, 0, 0, 0})
	out.Write(binary.BigEndian.AppendUint16(nil, 1))
	name := make([]byte, 66)
	copy(name, p.Name+"-"+evr)
	out.Write(name)
	out.Write(binary.BigEndian.AppendUint16(nil, 1))
	out.Write(binary.BigEndian.AppendUint16(nil, 5))
	out.Write(make([]byte, 16))
	out.Write(sig)
	for out.Len()%8 != 0 {
		out.WriteByte(0)
	}
	out.Write(hdr)
	out.Write(payload.Bytes())
	return out.Bytes(), nil
}

func cpioEntry(out *bytes.Buffer, name string, mode uint32, ino, mtime int32, body []byte) {
	nlink := 1
	if mode&0o40000 != 0 {
		nlink = 2
	}
	fmt.Fprintf(out, "070701%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x",
		ino, mode, 0, 0, nlink, mtime, len(body), 0, 0, 0, 0, len(name)+1, 0)
	out.WriteString(name)
	out.WriteByte(0)
	for out.Len()%4 != 0 {
		out.WriteByte(0)
	}
	out.Write(body)
	for out.Len()%4 != 0 {
		out.WriteByte(0)
	}
}
