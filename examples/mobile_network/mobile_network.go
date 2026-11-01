package main

import (
	"context"
	"io"
	"net"
	"net/http"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func publicIP(dialer *net.Dialer) string {
	transport := &http.Transport{
		DialContext: func(ctx context.Context, network, address string) (net.Conn, error) {
			return dialer.DialContext(ctx, network, address)
		},
		DisableKeepAlives: true,
	}
	client := &http.Client{Transport: transport, Timeout: 15 * time.Second}

	resp, err := client.Get("https://api.ipify.org")
	if err != nil {
		return "error: " + err.Error()
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "error: " + err.Error()
	}
	return strings.TrimSpace(string(body))
}

func main() {
	iup.Open()
	defer iup.Close()

	networks := []string{"DEFAULT", "CELLULAR", "WIFI"}
	titles := []string{"Default", "Mobile data", "Wi-Fi"}

	handle := iup.Label("").SetAttributes("EXPAND=HORIZONTAL")
	address := iup.Label("").SetAttributes("EXPAND=HORIZONTAL")

	updateHandle := func() {
		value := iup.GetGlobal("NETWORKHANDLE")
		if value == "" {
			value = "none"
		}
		handle.SetAttribute("TITLE", "Network handle: "+value)
	}

	toggles := iup.Vbox().SetAttributes("GAP=4")
	for i, title := range titles {
		network := networks[i]
		toggle := iup.Toggle(title)
		toggle.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
			if state == 1 {
				iup.SetGlobal("NETWORK", network)
				updateHandle()
			}
			return iup.DEFAULT
		}))
		iup.Append(toggles, toggle)
	}

	address.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(ih iup.Ihandle, s string, i int, p any) int {
		ih.SetAttribute("TITLE", "Public IP: "+s)
		updateHandle()
		return iup.DEFAULT
	}))

	btnCheck := iup.Button("Get Public IP").SetAttributes("PADDING=10x4")
	btnCheck.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		address.SetAttribute("TITLE", "Public IP: ...")
		dialer := iup.NetDialer()
		go func() {
			iup.PostMessage(address, publicIP(dialer), 0, nil)
		}()
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Frame(iup.Radio(toggles)).SetAttribute("TITLE", "Network"),
			handle,
			btnCheck,
			address,
		).SetAttributes("MARGIN=10x10, GAP=10"),
	).SetAttribute("TITLE", "Mobile Network")

	updateHandle()
	iup.Show(dlg)
	iup.MainLoop()
}
