package iup

import (
	"net"
	"syscall"
)

var netControl func(network, address string, c syscall.RawConn) error

// NetDialer returns a dialer whose connections use the network selected with the NETWORK global attribute.
// Each new socket is bound when it is created, so changing NETWORK affects the following connections.
// On platforms without NETWORK it is a plain dialer.
//
// https://gen2brain.github.io/iup-go/attrib/iup_globals.html#network
func NetDialer() *net.Dialer {
	return &net.Dialer{Control: netControl}
}
