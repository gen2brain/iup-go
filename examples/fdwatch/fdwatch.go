package main

import (
	"fmt"
	"net"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

var (
	reader net.Conn
	writer net.Conn
	label  iup.Ihandle
	count  int
)

func init() { iup.EntryPoint(main) }

func main() {
	iup.Open()
	defer iup.Close()

	label = iup.Label("Read 0 chunk(s): waiting").SetAttribute("EXPAND", "HORIZONTAL")

	dlg := iup.Dialog(iup.Vbox(label).SetAttributes("MARGIN=10x10")).
		SetAttribute("TITLE", "FdWatch")
	iup.Show(dlg)

	watch := iup.FdWatch()

	fd, err := socketPair()
	if err == nil {
		iup.SetAttribute(watch, "FD", int(fd))
		iup.SetAttribute(watch, "MODE", "READ")
		watch.On(iup.FdWatchActionCB, readCb)
		iup.SetAttribute(watch, "RUN", "YES")
	}

	if err != nil || iup.GetAttribute(watch, "RUN") != "YES" {
		iup.SetAttribute(label, "TITLE", "FdWatch is not supported by this driver")
	} else {
		go func() {
			for i := 1; i <= 5; i++ {
				time.Sleep(time.Second)
				fmt.Fprintf(writer, "line %d\n", i)
			}
			writer.Close()
		}()
	}

	iup.MainLoop()

	// The watch is NOT automatically destroyed, must be done manually; Android and iOS skip this since MainLoop returns immediately inside ENTRY_POINT.
	driver := iup.GetGlobal("DRIVER")
	if driver != "Android" && driver != "CocoaTouch" {
		iup.Destroy(watch)
		if reader != nil {
			reader.Close()
		}
	}
}

func socketPair() (uintptr, error) {
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		return 0, err
	}
	defer listener.Close()

	dialed := make(chan net.Conn, 1)
	failed := make(chan error, 1)
	go func() {
		conn, err := net.Dial("tcp", listener.Addr().String())
		if err != nil {
			failed <- err
			return
		}
		dialed <- conn
	}()

	reader, err = listener.Accept()
	if err != nil {
		return 0, err
	}

	select {
	case writer = <-dialed:
	case err = <-failed:
		return 0, err
	}

	raw, err := reader.(*net.TCPConn).SyscallConn()
	if err != nil {
		return 0, err
	}

	var fd uintptr
	if err = raw.Control(func(handle uintptr) { fd = handle }); err != nil {
		return 0, err
	}

	return fd, nil
}

func readCb(ih iup.Ihandle) int {
	buf := make([]byte, 256)
	n, err := reader.Read(buf)
	if n > 0 {
		count++
		text := strings.TrimRight(string(buf[:n]), "\r\n")
		iup.SetAttribute(label, "TITLE", fmt.Sprintf("Read %d chunk(s): %s", count, text))
	}
	if err != nil {
		iup.SetAttribute(ih, "RUN", "NO")
	}
	return iup.DEFAULT
}
