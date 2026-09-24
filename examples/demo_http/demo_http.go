//go:build web

package main

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"html"
	"io"
	"maps"
	"net"
	"net/http"
	"net/url"
	"runtime"
	"slices"
	"strconv"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type entry struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

type requestData struct {
	ID      int     `json:"id"`
	Group   string  `json:"group"`
	Name    string  `json:"name"`
	Method  string  `json:"method"`
	Path    string  `json:"path"`
	Params  []entry `json:"params"`
	Headers []entry `json:"headers"`
	Body    string  `json:"body"`
}

type session struct {
	Requests []*requestData `json:"requests"`
	Open     []int          `json:"open"`
	Selected int            `json:"selected"`
	NextID   int            `json:"next_id"`
}

type response struct {
	ID          int
	Serial      int
	Status      string
	Headers     string
	Body        string
	ContentType string
	Elapsed     time.Duration
	Err         error
}

type requestTab struct {
	data            *requestData
	page            iup.Ihandle
	method          iup.Ihandle
	path            iup.Ihandle
	params          iup.Ihandle
	headers         iup.Ihandle
	body            iup.Ihandle
	response        iup.Ihandle
	responseHeaders iup.Ihandle
	resultTabs      iup.Ihandle
	previewPage     iup.Ihandle
	previewBox      iup.Ihandle
	web             iup.Ihandle
	status          iup.Ihandle
	cancel          context.CancelFunc
	serial          int
	last            *response
}

type workbench struct {
	config   iup.Ihandle
	dialog   iup.Ihandle
	tree     iup.Ihandle
	tabs     iup.Ihandle
	views    iup.Ihandle
	status   iup.Ihandle
	server   *http.Server
	listener net.Listener
	baseURL  string
	state    session
	open     []*requestTab
	nodes    map[int]int
	active   int
	closing  bool
	webReady bool
}

func main() {
	iup.Open()
	iup.SetGlobal("UTF8MODE", "YES")
	iup.SetGlobal("APPID", "com.example.HTTPWorkbench")
	iup.SetGlobal("APPNAME", "HTTP API Workbench")
	app := &workbench{}
	app.config = iup.Config().SetAttribute("APP_NAME", "IupHTTPWorkbenchExample")
	iup.ConfigLoad(app.config)
	app.load()
	if err := app.startServer(); err != nil {
		app.config.Destroy()
		iup.Close()
		return
	}
	iup.WebBrowserOpen()
	app.webReady = iup.GetGlobal("IUP_WEBBROWSER_MISSING_LIB") == ""
	app.build()
	for _, id := range app.state.Open {
		app.openRequest(id)
	}
	if len(app.open) == 0 {
		app.openRequest(app.state.Requests[0].ID)
	}
	iup.Show(app.dialog)
	app.fillTree()
	if app.state.Selected >= 0 && app.state.Selected < len(app.open) {
		app.tabs.SetAttribute("VALUEPOS", app.state.Selected)
	}
	app.status.SetAttribute("TITLE", "Local sample server ready at "+app.baseURL)
	iup.MainLoop()
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		return
	}
	app.listener.Close()
	app.dialog.Destroy()
	app.config.Destroy()
	iup.Close()
}

func (app *workbench) startServer() error {
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		return err
	}
	app.listener = listener
	app.baseURL = "http://" + listener.Addr().String()
	mux := http.NewServeMux()
	mux.HandleFunc("/api/users", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json; charset=utf-8")
		json.NewEncoder(w).Encode(map[string]any{"users": []map[string]any{{"id": 1, "name": "Ada", "role": "admin"}, {"id": 2, "name": "Lin", "role": "editor"}}, "filter": r.URL.Query().Get("role")})
	})
	mux.HandleFunc("/api/echo", func(w http.ResponseWriter, r *http.Request) {
		body, err := io.ReadAll(io.LimitReader(r.Body, 1024*1024))
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		w.Header().Set("Content-Type", "application/json; charset=utf-8")
		w.Header().Set("X-Sample-Endpoint", "echo")
		json.NewEncoder(w).Encode(map[string]any{"method": r.Method, "path": r.URL.Path, "query": r.URL.Query(), "headers": r.Header, "body": string(body)})
	})
	mux.HandleFunc("/report", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		fmt.Fprint(w, `<!doctype html><html><head><meta charset="utf-8"><style>body{font:16px sans-serif;max-width:48em;margin:3em auto;color:#293047}table{border-collapse:collapse;width:100%}td,th{padding:.7em;border-bottom:1px solid #ddd;text-align:left}h1{color:#51439a}</style></head><body><h1>Service report</h1><p>Rendered by the embedded WebBrowser.</p><table><tr><th>Endpoint</th><th>State</th></tr><tr><td>Users</td><td>Healthy</td></tr><tr><td>Echo</td><td>Healthy</td></tr></table></body></html>`)
	})
	mux.HandleFunc("/api/slow", func(w http.ResponseWriter, r *http.Request) {
		select {
		case <-r.Context().Done():
			return
		case <-time.After(2200 * time.Millisecond):
		}
		w.Header().Set("Content-Type", "application/json")
		fmt.Fprint(w, `{"status":"completed","delay_ms":2200}`)
	})
	app.server = &http.Server{Handler: mux}
	go app.server.Serve(listener)
	return nil
}

func (app *workbench) load() {
	if err := json.Unmarshal([]byte(iup.ConfigGetVariableStr(app.config, "Session", "State")), &app.state); err != nil || len(app.state.Requests) == 0 {
		app.state = session{Selected: 0, NextID: 5, Requests: []*requestData{
			{ID: 1, Group: "Basics", Name: "List users", Method: "GET", Path: "/api/users", Params: []entry{{Key: "role", Value: "editor"}}},
			{ID: 2, Group: "Basics", Name: "Echo JSON", Method: "POST", Path: "/api/echo", Headers: []entry{{Key: "Content-Type", Value: "application/json"}}, Body: "{\n  \"hello\": \"world\"\n}"},
			{ID: 3, Group: "Examples", Name: "HTML report", Method: "GET", Path: "/report"},
			{ID: 4, Group: "Examples", Name: "Slow request", Method: "GET", Path: "/api/slow"},
		}, Open: []int{1, 2, 3}}
	}
	for i := range app.state.Requests {
		if app.state.Requests[i].ID >= app.state.NextID {
			app.state.NextID = app.state.Requests[i].ID + 1
		}
	}
}

func (app *workbench) save() {
	app.state.Open = app.state.Open[:0]
	for _, tab := range app.open {
		tab.capture()
		app.state.Open = append(app.state.Open, tab.data.ID)
	}
	app.state.Selected = app.tabs.GetInt("VALUEPOS")
	data, err := json.Marshal(app.state)
	if err == nil {
		iup.ConfigSetVariableStr(app.config, "Session", "State", string(data))
	}
	iup.ConfigSave(app.config)
}

func (app *workbench) build() {
	app.tree = iup.Tree().SetAttributes("ADDROOT=NO, EXPAND=YES, VISIBLELINES=14, VISIBLECOLUMNS=20")
	app.tree.SetCallback("EXECUTELEAF_CB", iup.ExecuteLeafFunc(func(_ iup.Ihandle, node int) int {
		app.openRequest(app.nodes[node])
		return iup.DEFAULT
	}))
	app.tabs = iup.Tabs().SetAttributes("EXPAND=YES, SHOWCLOSE=YES, ALLOWREORDER=YES")
	app.tabs.SetCallback("TABCLOSE_CB", iup.TabCloseFunc(func(_ iup.Ihandle, pos int) int {
		if pos < 0 || pos >= len(app.open) {
			return iup.IGNORE
		}
		tab := app.open[pos]
		tab.capture()
		tab.stop()
		app.open = append(app.open[:pos], app.open[pos+1:]...)
		app.status.SetAttribute("TITLE", "Closed "+tab.data.Name)
		return iup.CONTINUE
	}))
	app.tabs.SetCallback("REORDER_CB", iup.ReorderFunc(func(_ iup.Ihandle, from, to int) int {
		if from < 0 || from >= len(app.open) || to < 0 || to >= len(app.open) {
			return iup.IGNORE
		}
		tab := app.open[from]
		app.open = append(app.open[:from], app.open[from+1:]...)
		app.open = append(app.open[:to], append([]*requestTab{tab}, app.open[to:]...)...)
		return iup.DEFAULT
	}))
	app.status = iup.Label("Starting local server...").SetAttributes("EXPAND=HORIZONTAL, PADDING=8x4")
	collection := iup.Vbox(
		iup.Hbox(iup.Label("Collection").SetAttribute("EXPAND", "HORIZONTAL"), button("New", app.addRequest)).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"),
		app.tree,
	).SetAttributes("NGAP=6, NMARGIN=8x8")
	var center iup.Ihandle
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		collection.SetAttribute("TABTITLE", "Collection")
		app.tabs.SetAttribute("TABTITLE", "Requests")
		app.views = iup.Tabs(collection, app.tabs).SetAttribute("EXPAND", "YES")
		center = app.views
	} else {
		center = iup.Split(collection, app.tabs).SetAttributes("ORIENTATION=VERTICAL, VALUE=240, MINMAX=170:420")
	}
	app.dialog = iup.Dialog(iup.Vbox(center, app.status).SetAttribute("NGAP", 0))
	app.dialog.SetAttributes(map[string]string{"TITLE": "HTTP API Workbench", "SHRINK": "YES"})
	app.dialog.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int {
		if app.closing {
			return iup.IGNORE
		}
		app.closing = true
		app.save()
		for _, tab := range app.open {
			tab.stop()
		}
		app.server.Close()
		if app.active == 0 {
			iup.ExitLoop()
		}
		return iup.IGNORE
	}))
	app.dialog.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(_ iup.Ihandle, kind string, _ int, payload any) int {
		switch kind {
		case "result":
			result := payload.(response)
			if !app.closing {
				for _, tab := range app.open {
					if tab.data.ID == result.ID && tab.serial == result.Serial {
						tab.showResult(result)
						break
					}
				}
				app.save()
			}
		case "done":
			app.active--
			if app.closing && app.active == 0 {
				iup.ExitLoop()
			}
		}
		return iup.DEFAULT
	}))
}

func (app *workbench) fillTree() {
	app.tree.SetAttribute("DELNODE", "ALL")
	app.nodes = make(map[int]int)
	groups := []string{"Basics", "Examples", "My requests"}
	previous := -1
	for _, group := range groups {
		if previous < 0 {
			app.tree.SetAttribute("ADDBRANCH-1", group)
		} else {
			app.tree.SetAttribute(fmt.Sprintf("INSERTBRANCH%d", previous), group)
		}
		branch := app.tree.GetInt("LASTADDNODE")
		previous = branch
		leaf := -1
		for _, item := range app.state.Requests {
			if item.Group != group {
				continue
			}
			if leaf < 0 {
				app.tree.SetAttribute(fmt.Sprintf("ADDLEAF%d", branch), item.Method+"  "+item.Name)
			} else {
				app.tree.SetAttribute(fmt.Sprintf("INSERTLEAF%d", leaf), item.Method+"  "+item.Name)
			}
			leaf = app.tree.GetInt("LASTADDNODE")
			app.nodes[leaf] = item.ID
		}
	}
}

func (app *workbench) addRequest() {
	id := app.state.NextID
	app.state.NextID++
	app.state.Requests = append(app.state.Requests, &requestData{ID: id, Group: "My requests", Name: fmt.Sprintf("Request %d", id), Method: "GET", Path: "/api/echo"})
	app.fillTree()
	app.openRequest(id)
}

func (app *workbench) openRequest(id int) {
	if id == 0 {
		return
	}
	for pos, tab := range app.open {
		if tab.data.ID == id {
			app.tabs.SetAttribute("VALUEPOS", pos)
			app.showRequests()
			return
		}
	}
	for i := range app.state.Requests {
		if app.state.Requests[i].ID != id {
			continue
		}
		tab := &requestTab{data: app.state.Requests[i]}
		tab.build(app)
		app.open = append(app.open, tab)
		iup.Append(app.tabs, tab.page)
		if app.dialog.GetAttribute("WID") != "" {
			iup.Map(tab.page)
			iup.Refresh(app.dialog)
		}
		app.tabs.SetAttribute("VALUEPOS", len(app.open)-1)
		app.showRequests()
		return
	}
}

func (app *workbench) showRequests() {
	if app.views != 0 && app.dialog.GetAttribute("WID") != "" {
		app.views.SetAttribute("VALUEPOS", 1)
	}
}

func (tab *requestTab) build(app *workbench) {
	tab.method = iup.List().SetAttributes("DROPDOWN=YES, VISIBLECOLUMNS=5")
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		tab.method.SetAttribute("VISIBLECOLUMNS", 3)
	}
	for i, method := range []string{"GET", "POST", "PUT", "PATCH", "DELETE"} {
		tab.method.SetAttributeId("", i+1, method)
		if method == tab.data.Method {
			tab.method.SetAttribute("VALUE", i+1)
		}
	}
	if tab.method.GetInt("VALUE") == 0 {
		tab.method.SetAttribute("VALUE", 1)
	}
	tab.path = iup.Text().SetAttributes("EXPAND=HORIZONTAL, VISIBLECOLUMNS=30").SetAttribute("VALUE", tab.data.Path)
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		tab.path.SetAttribute("VISIBLECOLUMNS", 14)
	}
	tab.params = pairTable(&tab.data.Params)
	tab.headers = pairTable(&tab.data.Headers)
	lines := 5
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		lines = 3
	}
	tab.body = iup.Text().SetAttributes(fmt.Sprintf("MULTILINE=YES, EXPAND=YES, VISIBLELINES=%d, VISIBLECOLUMNS=42, FONT=Courier", lines)).SetAttribute("VALUE", tab.data.Body)
	tab.response = iup.Text().SetAttributes(fmt.Sprintf("MULTILINE=YES, READONLY=YES, WORDWRAP=YES, EXPAND=YES, VISIBLELINES=%d, VISIBLECOLUMNS=48, FONT=Courier", lines))
	tab.responseHeaders = iup.Text().SetAttributes(fmt.Sprintf("MULTILINE=YES, READONLY=YES, EXPAND=YES, VISIBLELINES=%d, VISIBLECOLUMNS=48, FONT=Courier", lines))
	tab.previewBox = iup.Vbox().SetAttribute("EXPAND", "YES")
	tab.previewPage = tab.previewBox.SetAttribute("TABTITLE", "HTML preview")
	tab.resultTabs = iup.Tabs(
		iup.Vbox(tab.response).SetAttributes("EXPAND=YES, TABTITLE=Response"),
		iup.Vbox(tab.responseHeaders).SetAttributes("EXPAND=YES, TABTITLE=Headers"),
		tab.previewPage,
	).SetAttribute("EXPAND", "YES")
	tab.resultTabs.SetCallback("TABCHANGE_CB", iup.TabChangeFunc(func(_ iup.Ihandle, next, previous iup.Ihandle) int {
		if previous == tab.previewPage {
			tab.destroyBrowser(app)
		}
		if next == tab.previewPage {
			tab.createBrowser(app)
		}
		return iup.DEFAULT
	}))
	tab.status = iup.Label("Ready to send").SetAttribute("EXPAND", "HORIZONTAL")
	requestEditors := iup.Tabs(
		iup.Vbox(tab.params, pairButtons(tab.params, &tab.data.Params)).SetAttributes("EXPAND=YES, TABTITLE=Parameters"),
		iup.Vbox(tab.headers, pairButtons(tab.headers, &tab.data.Headers)).SetAttributes("EXPAND=YES, TABTITLE=Headers"),
		iup.Vbox(tab.body).SetAttributes("EXPAND=YES, TABTITLE=Body"),
	).SetAttribute("EXPAND", "YES")
	send := button("Send", func() { tab.send(app) })
	cancel := button("Cancel", func() {
		tab.stop()
		tab.status.SetAttribute("TITLE", "Request canceled")
	})
	var toolbar iup.Ihandle
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		toolbar = iup.Vbox(
			iup.Hbox(tab.method, tab.path).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"),
			iup.Hbox(send, cancel, iup.Fill()).SetAttributes("NGAP=6"),
		).SetAttributes("NGAP=5")
	} else {
		toolbar = iup.Vbox(
			iup.Label(tab.data.Name).SetAttributes("FONTSTYLE=Bold, EXPAND=HORIZONTAL"),
			iup.Hbox(tab.method, tab.path, send, cancel).SetAttributes("NGAP=6, ALIGNMENT=ACENTER"),
		).SetAttributes("NGAP=5")
	}
	content := iup.Split(requestEditors, tab.resultTabs).SetAttributes("ORIENTATION=HORIZONTAL, VALUE=590, MINMAX=210:760")
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		tab.page = iup.Vbox(toolbar, tab.status, content).SetAttributes("NGAP=7, NMARGIN=9x8, EXPAND=YES").SetAttribute("TABTITLE", tab.data.Name)
	} else {
		tab.page = iup.Vbox(toolbar, content, tab.status).SetAttributes("NGAP=7, NMARGIN=9x8, EXPAND=YES").SetAttribute("TABTITLE", tab.data.Name)
	}
}

func pairTable(values *[]entry) iup.Ihandle {
	lines := 4
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		lines = 2
	}
	table := iup.Table().SetAttributes(fmt.Sprintf("NUMCOL=2, VISIBLELINES=%d, EXPAND=YES, EDITABLE=YES, STRETCHLAST=YES, USERRESIZE=YES, ALTERNATECOLOR=YES", lines))
	iup.SetAttributeId(table, "TITLE", 1, "Key")
	iup.SetAttributeId(table, "TITLE", 2, "Value")
	updatePairs(table, *values)
	table.SetCallback("EDITEND_CB", iup.EditEndFunc(func(_ iup.Ihandle, line, col int, value string, apply int) int {
		if apply != 0 && line > 0 && line <= len(*values) && col >= 1 && col <= 2 {
			if col == 1 {
				(*values)[line-1].Key = value
			} else {
				(*values)[line-1].Value = value
			}
		}
		return iup.DEFAULT
	}))
	return table
}

func updatePairs(table iup.Ihandle, values []entry) {
	table.SetAttribute("NUMLIN", len(values))
	for i, pair := range values {
		iup.SetAttributeId2(table, "", i+1, 1, pair.Key)
		iup.SetAttributeId2(table, "", i+1, 2, pair.Value)
	}
}

func pairButtons(table iup.Ihandle, values *[]entry) iup.Ihandle {
	return iup.Hbox(button("Add row", func() {
		*values = append(*values, entry{})
		updatePairs(table, *values)
		table.SetAttribute("FOCUSCELL", fmt.Sprintf("%d:1", len(*values)))
	}), button("Remove row", func() {
		line := 0
		fmt.Sscanf(table.GetAttribute("FOCUSCELL"), "%d:", &line)
		if line > 0 && line <= len(*values) {
			*values = append((*values)[:line-1], (*values)[line:]...)
			updatePairs(table, *values)
		}
	})).SetAttributes("NGAP=5")
}

func (tab *requestTab) capture() {
	methods := []string{"GET", "POST", "PUT", "PATCH", "DELETE"}
	method := tab.method.GetInt("VALUE")
	if method >= 1 && method <= len(methods) {
		tab.data.Method = methods[method-1]
	}
	tab.data.Path = tab.path.GetAttribute("VALUE")
	tab.data.Body = tab.body.GetAttribute("VALUE")
}

func (tab *requestTab) stop() {
	tab.serial++
	if tab.cancel != nil {
		tab.cancel()
		tab.cancel = nil
	}
}

func (tab *requestTab) send(app *workbench) {
	tab.capture()
	tab.stop()
	endpoint, err := url.Parse(strings.TrimSpace(tab.data.Path))
	if err != nil || endpoint == nil || endpoint.IsAbs() || endpoint.Host != "" || !strings.HasPrefix(endpoint.Path, "/") || strings.HasPrefix(endpoint.Path, "//") {
		tab.status.SetAttribute("TITLE", "Enter a local path starting with /, such as /api/users")
		return
	}
	values := endpoint.Query()
	for _, param := range tab.data.Params {
		if strings.TrimSpace(param.Key) != "" {
			values.Add(param.Key, param.Value)
		}
	}
	endpoint.RawQuery = values.Encode()
	ctx, cancel := context.WithCancel(context.Background())
	tab.cancel = cancel
	serial := tab.serial
	id := tab.data.ID
	method, body := tab.data.Method, tab.data.Body
	headers := append([]entry(nil), tab.data.Headers...)
	tab.status.SetAttribute("TITLE", "Sending "+method+" "+endpoint.String())
	app.active++
	go func() {
		defer iup.PostMessage(app.dialog, "done", 0, nil)
		start := time.Now()
		result := response{ID: id, Serial: serial}
		request, err := http.NewRequestWithContext(ctx, method, app.baseURL+endpoint.String(), strings.NewReader(body))
		if err == nil {
			for _, header := range headers {
				if strings.TrimSpace(header.Key) != "" {
					request.Header.Add(header.Key, header.Value)
				}
			}
			client := &http.Client{Transport: localTransport, Timeout: 20 * time.Second, CheckRedirect: func(_ *http.Request, _ []*http.Request) error { return http.ErrUseLastResponse }}
			var reply *http.Response
			reply, err = client.Do(request)
			if err == nil {
				result.Status = reply.Status
				result.ContentType = reply.Header.Get("Content-Type")
				var content []byte
				content, err = io.ReadAll(io.LimitReader(reply.Body, 2*1024*1024+1))
				reply.Body.Close()
				if len(content) > 2*1024*1024 {
					err = errors.New("response exceeds 2 MiB")
				} else {
					result.Body = string(content)
					if strings.Contains(result.ContentType, "json") {
						var formatted bytes.Buffer
						if json.Indent(&formatted, content, "", "  ") == nil {
							result.Body = formatted.String()
						}
					}
				}
				var lines []string
				for _, key := range slices.Sorted(maps.Keys(reply.Header)) {
					for _, value := range reply.Header[key] {
						lines = append(lines, key+": "+value)
					}
				}
				result.Headers = strings.Join(lines, "\n")
			}
		}
		result.Err = err
		result.Elapsed = time.Since(start)
		iup.PostMessage(app.dialog, "result", 0, result)
	}()
}

var localTransport = &http.Transport{DialContext: (&net.Dialer{}).DialContext}

func (tab *requestTab) showResult(result response) {
	if tab.cancel != nil {
		tab.cancel()
		tab.cancel = nil
	}
	if result.Err != nil {
		tab.last = nil
		tab.response.SetAttribute("VALUE", "")
		tab.responseHeaders.SetAttribute("VALUE", "")
		if tab.web != 0 {
			tab.web.SetAttribute("HTML", "")
		}
		tab.status.SetAttribute("TITLE", result.Err.Error())
		return
	}
	tab.last = &result
	tab.response.SetAttribute("VALUE", result.Body)
	tab.responseHeaders.SetAttribute("VALUE", result.Headers)
	tab.status.SetAttribute("TITLE", fmt.Sprintf("%s  |  %s  |  %s", result.Status, result.Elapsed.Round(time.Millisecond), formatBytes(len(result.Body))))
	if tab.web != 0 {
		tab.web.SetAttribute("HTML", previewHTML(result))
	}
}

func (tab *requestTab) createBrowser(app *workbench) {
	if tab.web != 0 || app.closing {
		return
	}
	if !app.webReady {
		tab.status.SetAttribute("TITLE", "WebBrowser runtime is unavailable")
		return
	}
	tab.web = iup.WebBrowser()
	if tab.web == 0 {
		app.webReady = false
		tab.status.SetAttribute("TITLE", "WebBrowser runtime is unavailable")
		return
	}
	tab.web.SetAttribute("EXPAND", "YES")
	iup.Append(tab.previewBox, tab.web)
	if iup.Map(tab.web) != iup.NOERROR {
		tab.web.Destroy()
		tab.web = 0
		app.webReady = false
		tab.status.SetAttribute("TITLE", "WebBrowser could not be mapped")
		return
	}
	iup.Refresh(app.dialog)
	if tab.last != nil {
		tab.web.SetAttribute("HTML", previewHTML(*tab.last))
	}
}

func (tab *requestTab) destroyBrowser(app *workbench) {
	if tab.web != 0 {
		tab.web.Destroy()
		tab.web = 0
		if !app.closing {
			iup.Refresh(app.dialog)
		}
	}
}

func previewHTML(result response) string {
	if strings.HasPrefix(strings.ToLower(result.ContentType), "text/html") {
		return result.Body
	}
	return "<!doctype html><html><head><meta charset=\"utf-8\"></head><body><pre>" + html.EscapeString(result.Body) + "</pre></body></html>"
}

func formatBytes(n int) string {
	if n >= 1024 {
		return strconv.Itoa(n/1024) + " KiB"
	}
	return strconv.Itoa(n) + " B"
}

func button(title string, action func()) iup.Ihandle {
	item := iup.Button(title).SetAttribute("PADDING", "5x3")
	item.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return item
}
