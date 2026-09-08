package main

import (
	"fmt"
	"math"
	"strconv"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type message struct {
	from  string
	body  string
	stamp string
	day   string
}

type conversation struct {
	name    string
	people  int
	stamp   string
	preview string
	unread  int
	msgs    []message
}

var conversations = []conversation{
	{name: "Alex Rivera", people: 2, stamp: "Yesterday", preview: "See you then", msgs: []message{
		{day: "Monday, August 10", from: "Alex Rivera", body: "Did the tickets arrive?", stamp: "6:41 PM"},
		{body: "Both of them, seats **4C** and **4D** :)", stamp: "6:43 PM"},
		{from: "Alex Rivera", body: "See you then <3", stamp: "6:44 PM"},
	}},
	{name: "Priya Nair", people: 2, stamp: "Sunday", preview: "Sounds good", unread: 2, msgs: []message{
		{day: "Sunday, August 9", from: "Priya Nair", body: "Is the meeting still at eleven?", stamp: "11:02 AM"},
		{from: "Priya Nair", body: "Sounds good :)", stamp: "11:03 AM"},
	}},
	{name: "Sam Okonkwo", people: 2, stamp: "Saturday", preview: "The wind never stopped", msgs: []message{
		{day: "Saturday, August 8", from: "Sam Okonkwo", body: "The wind never stopped", stamp: "10:09 AM"},
		{body: "Stay inside then :(", stamp: "10:12 AM"},
	}},
	{name: "Lena Fischer", people: 2, stamp: "Friday", preview: "Call me when you land", msgs: []message{
		{day: "Friday, August 7", from: "Lena Fischer", body: "Call me when you land", stamp: "8:15 PM"},
	}},
	{name: "Corner Bookshop", people: 2, stamp: "9/1/26", preview: "Your order is ready", msgs: []message{
		{day: "Tuesday, September 1", from: "Corner Bookshop", body: "Your order is ready for pickup until Sunday. Details at [our shop](https://example.com/shop).", stamp: "9:00 AM"},
	}},
	{name: "Tomas Novak", people: 2, stamp: "8/30/26", preview: "Changed phone number", msgs: []message{
		{day: "Sunday, August 30", from: "Tomas Novak", body: "Changed phone number", stamp: "3:20 PM"},
	}},
	{name: "Weekend Trip", people: 4, stamp: "8/29/26", preview: "Mira: Ferry is cancelled", unread: 4, msgs: []message{
		{day: "Wednesday, August 12", from: "Jonas Weber", body: "Are you all still up?", stamp: "7:14 PM"},
		{from: "Mira Sandoval", body: "Where is the map? We have *no* idea ;)", stamp: "9:03 PM"},
		{day: "Saturday, August 29", from: "Mira Sandoval", body: "Storm over the island, the ferry is cancelled", stamp: "10:09 AM"},
		{body: "That looks rough, stay inside", stamp: "10:20 AM"},
		{from: "Jonas Weber", body: "We are fine, the power came back an hour ago :D", stamp: "10:24 AM"},
		{body: "Send the timetable when you have it:\n\n```\n07:15  Harbour\n11:40  Old Town\n```", stamp: "10:31 AM"},
	}},
	{name: "Account Alerts", people: 2, stamp: "8/28/26", preview: "Your statement is ready", msgs: []message{
		{day: "Friday, August 28", from: "Account Alerts", body: "Your monthly statement is ready in the app.", stamp: "7:45 AM"},
	}},
	{name: "Design Review", people: 5, stamp: "8/27/26", preview: "Iris: Ask where she got it", msgs: []message{
		{day: "Thursday, August 27", from: "Iris Lambert", body: "Ask where she got it", stamp: "1:12 PM"},
	}},
	{name: "Marco Bianchi", people: 2, stamp: "8/26/26", preview: "Thanks!", msgs: []message{
		{day: "Wednesday, August 26", from: "Marco Bianchi", body: "Thanks!", stamp: "4:05 PM"},
	}},
}

var emoticons = []string{":)", ":D", ";)", ":(", "<3"}

type palette struct {
	bg, panel, rowSel, rowHover  string
	text, dim                    string
	bubbleIn, bubbleOut          string
	accent, accentText, onAccent string
}

func global(name, fallback string) string {
	if v := iup.GetGlobal(name); v != "" {
		return v
	}
	return fallback
}

func luma(c string) float64 {
	r, g, b := rgb(c)
	return (0.299*r + 0.587*g + 0.114*b) / 255
}

var (
	pal      palette
	avatarBg = []string{
		"196 92 106", "94 132 214", "88 158 120", "202 142 74",
		"142 106 200", "80 152 172", "184 108 158", "116 138 92",
	}

	convCv                   iup.Ihandle
	thread, search, composer iup.Ihandle
	title, subtitle          iup.Ihandle
	dlg                      iup.Ihandle

	shown    []int
	selected int
	hovered  = -1

	images  []iup.Ihandle
	imaged  = map[iup.Ihandle]string{}
	iconGen int
)

func main() {
	iup.Open()
	defer iup.Close()

	setPalette()
	makeIcons()
	makeEmoticons()
	filter("")

	convCv = iup.Canvas().SetAttributes(map[string]string{
		"BORDER":    "NO",
		"EXPAND":    "YES",
		"SCROLLBAR": "VERTICAL",
		"YAUTOHIDE": "YES",
		"CANFOCUS":  "NO",
		"SIZE":      fmt.Sprintf("x%d", 3*8*len(conversations)),
	})
	convCv.SetCallback("ACTION", iup.ActionFunc(drawConversations))
	convCv.SetCallback("RESIZE_CB", iup.ResizeFunc(resized))
	convCv.SetCallback("SCROLL_CB", iup.ScrollFunc(scrolled))
	convCv.SetCallback("WHEEL_CB", iup.WheelFunc(wheel))
	convCv.SetCallback("BUTTON_CB", iup.ButtonFunc(convClick))
	convCv.SetCallback("MOTION_CB", iup.MotionFunc(convHover))
	convCv.SetCallback("LEAVEWINDOW_CB", iup.LeaveWindowFunc(convLeave))

	thread = iup.Text().SetAttributes(map[string]string{
		"MULTILINE":  "YES",
		"FORMATTING": "YES",
		"WORDWRAP":   "YES",
		"READONLY":   "YES",
		"BORDER":     "NO",
		"EXPAND":     "YES",
		"PADDING":    "10x8",
	})
	thread.SetCallback("TEXTLINK_CB", iup.TextLinkFunc(linkClicked))

	search = iup.Text().SetAttributes(map[string]string{
		"VISIBLECOLUMNS": "20",
		"EXPAND":         "HORIZONTAL",
		"PADDING":        "6x5",
		"CUEBANNER":      "Search",
	})
	search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(searchChanged))

	composer = iup.Text().SetAttributes(map[string]string{
		"VISIBLECOLUMNS": "40",
		"EXPAND":         "HORIZONTAL",
		"PADDING":        "8x6",
		"CUEBANNER":      "Type a message, **bold** and :) work",
	})

	sendBtn := iconButton("send", "Send", iup.ActionFunc(send))
	sendBtn.SetHandle("sendButton")
	smile := iconButton("smile", "Emoticon", nil)
	emoticonPopover(smile)

	title = iup.Label("").SetAttributes("FONTSTYLE=Bold, EXPAND=HORIZONTAL")
	subtitle = iup.Label("").SetAttributes(map[string]string{"FGCOLOR": pal.accent, "EXPAND": "HORIZONTAL"})

	sidebar := iup.Vbox(
		iup.Hbox(search, iconButton("compose", "New chat", nil)).SetAttributes("MARGIN=10x10, GAP=6, ALIGNMENT=ACENTER"),
		convCv,
	).SetAttribute("EXPAND", "VERTICAL")

	right := iup.Vbox(
		iup.Hbox(
			iup.Vbox(title, subtitle).SetAttribute("EXPAND", "HORIZONTAL"),
			iconButton("search", "Search in conversation", nil),
			iconButton("person", "Add participant", nil),
			iconButton("call", "Call", nil),
			iconButton("info", "Details", nil),
			iconButton("theme", "Light or dark", iup.ActionFunc(toggleAppearance)),
		).SetAttributes("MARGIN=12x10, GAP=4, ALIGNMENT=ACENTER"),
		thread,
		iup.Hbox(
			iconButton("plus", "Attach", nil),
			smile,
			composer,
			sendBtn,
		).SetAttributes("MARGIN=12x12, GAP=8, ALIGNMENT=ACENTER"),
	).SetAttributes("EXPAND=YES, GAP=4")

	dlg = iup.Dialog(iup.Hbox(sidebar, right)).SetAttributes(map[string]string{
		"TITLE":        "Chat",
		"DEFAULTENTER": "sendButton",
	})
	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(themeChanged))

	iup.Show(dlg)
	open(0)
	iup.SetFocus(composer)
	iup.MainLoop()
}

func accentText(accent, bg, fg string) string {
	if math.Abs(luma(accent)-luma(bg)) < 0.35 {
		return mix(accent, fg, 0.5)
	}
	return accent
}

func setPalette() {
	panel := global("DLGBGCOLOR", "240 240 240")
	bg := global("TXTBGCOLOR", "255 255 255")
	fg := global("TXTFGCOLOR", "0 0 0")
	accent := global("ACCENTCOLOR", global("TXTHLCOLOR", "60 120 220"))

	onAccent := "255 255 255"
	if luma(accentText(accent, bg, fg)) > 0.6 {
		onAccent = "0 0 0"
	}
	pal = palette{
		bg: bg, panel: panel,
		rowSel:   mix(panel, accentText(accent, panel, fg), 0.26),
		rowHover: mix(panel, fg, 0.08),
		text:     fg,
		dim:      mix(fg, bg, 0.45),
		bubbleIn: mix(bg, fg, 0.09), bubbleOut: mix(bg, accent, 0.24),
		accent: accent, accentText: accentText(accent, bg, fg), onAccent: onAccent,
	}
}

func themeChanged(ih iup.Ihandle, darkMode int) int {
	retheme()
	return iup.DEFAULT
}

func toggleAppearance(ih iup.Ihandle) int {
	if iup.GetGlobal("DARKMODE") == "YES" {
		iup.SetGlobal("APPEARANCE", "LIGHT")
	} else {
		iup.SetGlobal("APPEARANCE", "DARK")
	}
	retheme()
	return iup.DEFAULT
}

func retheme() {
	setPalette()
	makeIcons()
	subtitle.SetAttribute("FGCOLOR", pal.accentText)
	open(selected)
}

func rgb(c string) (float64, float64, float64) {
	f := strings.Fields(c)
	if len(f) < 3 {
		return 0, 0, 0
	}
	var v [3]float64
	for i := 0; i < 3; i++ {
		n, _ := strconv.Atoi(f[i])
		v[i] = float64(n)
	}
	return v[0], v[1], v[2]
}

func mix(a, b string, t float64) string {
	ar, ag, ab := rgb(a)
	br, bg, bb := rgb(b)
	return fmt.Sprintf("%d %d %d", int(ar+(br-ar)*t), int(ag+(bg-ag)*t), int(ab+(bb-ab)*t))
}

func open(idx int) {
	selected = idx
	c := &conversations[idx]
	c.unread = 0

	title.SetAttribute("TITLE", c.name)
	if c.people > 2 {
		subtitle.SetAttribute("TITLE", fmt.Sprintf("%d participants", c.people))
	} else {
		subtitle.SetAttribute("TITLE", "online")
	}

	thread.SetAttribute("READONLY", "NO")
	thread.SetAttribute("VALUE", "")
	for i := range c.msgs {
		appendMessage(&c.msgs[i])
	}
	thread.SetAttribute("READONLY", "YES")
	thread.SetAttribute("SCROLLTO", fmt.Sprintf("%d:1", thread.GetInt("LINECOUNT")))
	iup.Update(convCv)
}

func appendMessage(m *message) {
	if m.day != "" {
		ln := appendLine(m.day)
		tag(ln, 1, len([]rune(m.day))+1, map[string]string{
			"ALIGNMENT": "CENTER", "FGCOLOR": pal.dim,
			"FONTSCALE": "SMALL", "SPACEBEFORE": "10", "SPACEAFTER": "6",
		})
	}

	mine := m.from == ""
	who, align := m.from, "LEFT"
	if mine {
		who, align = "You", "RIGHT"
	}
	head := who + "   " + m.stamp

	ln := appendLine(head)
	tag(ln, 1, len([]rune(who))+1, map[string]string{
		"WEIGHT": "BOLD", "FGCOLOR": pal.accentText, "ALIGNMENT": align, "SPACEBEFORE": "8",
	})
	tag(ln, len([]rune(who))+1, len([]rune(head))+1, map[string]string{
		"FGCOLOR": pal.dim, "FONTSCALE": "SMALL", "ALIGNMENT": align,
	})

	first := thread.GetInt("LINECOUNT") + 1
	thread.SetAttribute("APPENDMARKDOWN", m.body)
	last := thread.GetInt("LINECOUNT")
	if last < first {
		thread.SetAttribute("APPEND", plain(m.body))
		last = thread.GetInt("LINECOUNT")
	}

	bubble := pal.bubbleIn
	if mine {
		bubble = pal.bubbleOut
	}
	for l := first; l <= last; l++ {
		body := lineText(l)
		tag(l, 1, len([]rune(body))+1, map[string]string{
			"BGCOLOR": bubble, "ALIGNMENT": align, "SPACEAFTER": "4", "INDENT": "6",
		})
		for _, e := range emoticons {
			cols := columns(body, e)
			for i := len(cols) - 1; i >= 0; i-- {
				tag(l, cols[i], cols[i]+len([]rune(e)), map[string]string{"IMAGE": "emo" + e})
			}
		}
	}
}

func appendLine(s string) int {
	thread.SetAttribute("APPEND", s)
	return thread.GetInt("LINECOUNT")
}

func lineText(n int) string {
	lines := strings.Split(thread.GetAttribute("VALUE"), "\n")
	if n-1 < 0 || n-1 >= len(lines) {
		return ""
	}
	return lines[n-1]
}

func columns(line, token string) []int {
	var out []int
	for i, off := 0, 0; ; {
		j := strings.Index(line[off:], token)
		if j < 0 {
			return out
		}
		i = off + j
		out = append(out, len([]rune(line[:i]))+1)
		off = i + len(token)
	}
}

func tag(line, col, endCol int, attrs map[string]string) {
	ft := iup.User()
	for k, v := range attrs {
		ft.SetAttribute(k, v)
	}
	ft.SetAttribute("SELECTION", fmt.Sprintf("%d,%d:%d,%d", line, col, line, endCol))
	iup.SetAttributeHandle(thread, "ADDFORMATTAG", ft)
}

func linkClicked(ih iup.Ihandle, url string) int {
	iup.Message("Link", url)
	return iup.DEFAULT
}

func filter(q string) {
	q = strings.ToLower(strings.TrimSpace(q))
	shown = shown[:0]
	for i := range conversations {
		c := &conversations[i]
		if q == "" || strings.Contains(strings.ToLower(c.name), q) ||
			strings.Contains(strings.ToLower(c.preview), q) {
			shown = append(shown, i)
		}
	}
}

func searchChanged(ih iup.Ihandle) int {
	filter(ih.GetAttribute("VALUE"))
	hovered = -1
	iup.Update(convCv)
	return iup.DEFAULT
}

type metrics struct {
	line, pad, gap, avatar, radius int
	base, bold, small, tiny        string
}

func metricsOf(ih iup.Ihandle) metrics {
	base := ih.GetAttribute("FONT")
	ih.SetAttribute("DRAWFONT", base)
	_, _, line := iup.DrawGetTextMetrics(ih)
	return metrics{
		line: line, pad: line / 2, gap: line / 4,
		avatar: line * 5 / 2, radius: line / 2,
		base: base, bold: variant(base, "Bold", 0),
		small: variant(base, "", -1), tiny: variant(base, "Bold", -2),
	}
}

func variant(base, style string, delta int) string {
	family, rest := base, ""
	if i := strings.LastIndex(base, ","); i >= 0 {
		family, rest = base[:i], strings.TrimSpace(base[i+1:])
	}
	size := 10
	if f := strings.Fields(rest); len(f) > 0 {
		if n, err := strconv.Atoi(f[len(f)-1]); err == nil && n != 0 {
			size = n
		}
	}
	if size < 0 {
		size -= delta
	} else {
		size += delta
	}
	if style == "" {
		return fmt.Sprintf("%s, %d", family, size)
	}
	return fmt.Sprintf("%s, %s %d", family, style, size)
}

func fill(ih iup.Ihandle, c string, x0, y0, x1, y1, r int) {
	ih.SetAttribute("DRAWCOLOR", c)
	ih.SetAttribute("DRAWSTYLE", "FILL")
	if r > 0 {
		iup.DrawRoundedRectangle(ih, x0, y0, x1, y1, r)
		return
	}
	iup.DrawRectangle(ih, x0, y0, x1, y1)
}

func drawString(ih iup.Ihandle, s, font, color, align string, x, y, w, h int) {
	ih.SetAttribute("DRAWFONT", font)
	ih.SetAttribute("DRAWCOLOR", color)
	ih.SetAttribute("DRAWTEXTALIGNMENT", align)
	ih.SetAttribute("DRAWTEXTELLIPSIS", "YES")
	iup.DrawText(ih, s, x, y, w, h)
}

func avatar(ih iup.Ihandle, name string, x, y, d int, m metrics) {
	seed := 0
	for _, r := range name {
		seed += int(r)
	}
	ih.SetAttribute("DRAWCOLOR", avatarBg[seed%len(avatarBg)])
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawEllipse(ih, x, y, x+d, y+d)

	ih.SetAttribute("DRAWFONT", m.bold)
	ih.SetAttribute("DRAWCOLOR", "255 255 255")
	tw, th := iup.DrawGetTextSize(ih, initials(name))
	iup.DrawText(ih, initials(name), x+(d-tw)/2, y+(d-th)/2, tw, th)
}

func initials(name string) string {
	out := ""
	for _, w := range strings.Fields(name) {
		out += string([]rune(w)[:1])
		if len(out) == 2 {
			break
		}
	}
	return strings.ToUpper(out)
}

func drawConversations(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)

	m := metricsOf(ih)
	w, h := iup.DrawGetSize(ih)
	rowH := m.avatar + m.pad
	pos := ih.GetInt("POSY")

	iup.DrawParentBackground(ih)

	for i, idx := range shown {
		c := &conversations[idx]
		y := i*rowH - pos
		if y > h || y+rowH < 0 {
			continue
		}

		switch {
		case idx == selected:
			fill(ih, pal.rowSel, m.gap, y+m.gap/2, w-m.gap, y+rowH-m.gap/2, m.radius)
		case i == hovered:
			fill(ih, pal.rowHover, m.gap, y+m.gap/2, w-m.gap, y+rowH-m.gap/2, m.radius)
		}

		avatar(ih, c.name, m.pad, y+m.pad/2, m.avatar-m.pad/2, m)

		tx := m.pad + m.avatar + m.gap
		tw := w - tx - m.pad

		ih.SetAttribute("DRAWFONT", m.small)
		sw, _ := iup.DrawGetTextSize(ih, c.stamp)
		drawString(ih, c.stamp, m.small, pal.dim, "ARIGHT", tx+tw-sw, y+m.pad, sw, m.line)
		drawString(ih, c.name, m.bold, pal.text, "ALEFT", tx, y+m.pad-m.gap/2, tw-sw-m.gap, m.line+m.gap)

		badge := 0
		if c.unread > 0 {
			ih.SetAttribute("DRAWFONT", m.tiny)
			n := strconv.Itoa(c.unread)
			nw, nh := iup.DrawGetTextSize(ih, n)
			badge = nw + m.pad
			if badge < m.line {
				badge = m.line
			}
			by := y + m.pad + m.line + m.gap/2
			fill(ih, pal.accentText, tx+tw-badge, by, tx+tw, by+m.line, m.line/2)
			drawString(ih, n, m.tiny, pal.onAccent, "ACENTER", tx+tw-badge, by+(m.line-nh)/2, badge, nh)
			badge += m.gap
		}
		drawString(ih, c.preview, m.base, pal.dim, "ALEFT", tx, y+m.pad+m.line+m.gap/2, tw-badge, m.line)
	}

	total := len(shown) * rowH
	if ih.GetInt("YMAX") != total {
		ih.SetAttribute("YMAX", total)
	}
	if ih.GetInt("DY") != h {
		ih.SetAttribute("DY", h)
	}
	return iup.DEFAULT
}

func resized(ih iup.Ihandle, w, h int) int {
	ih.SetAttribute("DY", h)
	return iup.DEFAULT
}

func scrolled(ih iup.Ihandle, op int, posx, posy float64) int {
	iup.Update(ih)
	return iup.DEFAULT
}

func wheel(ih iup.Ihandle, delta float64, x, y int, status string) int {
	_, _, line := iup.DrawGetTextMetrics(ih)
	max := float64(ih.GetInt("YMAX") - ih.GetInt("DY"))
	pos := float64(ih.GetInt("POSY")) - delta*3*float64(line)
	ih.SetAttribute("POSY", math.Max(0, math.Min(pos, math.Max(0, max))))
	iup.Update(ih)
	return iup.DEFAULT
}

func convClick(ih iup.Ihandle, button, pressed, x, y int, status string) int {
	if button != iup.BUTTON1 || pressed == 0 {
		return iup.DEFAULT
	}
	if i := rowAt(ih, y); i >= 0 {
		open(shown[i])
	}
	return iup.DEFAULT
}

func convHover(ih iup.Ihandle, x, y int, status string) int {
	if i := rowAt(ih, y); i != hovered {
		hovered = i
		iup.Update(ih)
	}
	return iup.DEFAULT
}

func convLeave(ih iup.Ihandle) int {
	hovered = -1
	iup.Update(ih)
	return iup.DEFAULT
}

func rowAt(ih iup.Ihandle, y int) int {
	m := metricsOf(ih)
	i := (y + ih.GetInt("POSY")) / (m.avatar + m.pad)
	if i < 0 || i >= len(shown) {
		return -1
	}
	return i
}

func emoticonPopover(anchor iup.Ihandle) {
	var pop iup.Ihandle
	keys := make([]iup.Ihandle, 0, len(emoticons))
	for _, e := range emoticons {
		token := e
		b := iup.Button("").SetAttributes(map[string]string{
			"IMAGE": "emo" + token, "FLAT": "YES", "CANFOCUS": "NO", "PADDING": "5x5", "TIP": token,
		})
		b.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
			pop.SetAttribute("VISIBLE", "NO")
			composer.SetAttribute("INSERT", token+" ")
			iup.SetFocus(composer)
			return iup.DEFAULT
		}))
		keys = append(keys, b)
	}
	pop = iup.Popover(iup.Hbox(keys...).SetAttributes("MARGIN=6x6, GAP=4, ALIGNMENT=ACENTER"))
	pop.SetAttribute("POSITION", "TOP")
	iup.SetAttributeHandle(pop, "ANCHOR", anchor)
	anchor.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		pop.SetAttribute("VISIBLE", "YES")
		return iup.DEFAULT
	}))
}

func plain(s string) string {
	s = strings.ReplaceAll(s, "\n", " ")
	for _, mark := range []string{"**", "__", "```", "`", "*", "_"} {
		s = strings.ReplaceAll(s, mark, "")
	}
	return strings.Join(strings.Fields(s), " ")
}

func send(ih iup.Ihandle) int {
	body := strings.TrimSpace(composer.GetAttribute("VALUE"))
	if body == "" {
		return iup.DEFAULT
	}
	c := &conversations[selected]
	c.msgs = append(c.msgs, message{body: body, stamp: time.Now().Format("3:04 PM")})
	c.preview = plain(body)
	c.stamp = "now"

	composer.SetAttribute("VALUE", "")
	thread.SetAttribute("READONLY", "NO")
	appendMessage(&c.msgs[len(c.msgs)-1])
	thread.SetAttribute("READONLY", "YES")
	thread.SetAttribute("SCROLLTO", fmt.Sprintf("%d:1", thread.GetInt("LINECOUNT")))
	iup.Update(convCv)
	return iup.DEFAULT
}

type sdf func(x, y float64) float64

type layer struct {
	shape sdf
	color string
}

func cut(a, b sdf) sdf {
	return func(x, y float64) float64 { return math.Max(a(x, y), -b(x, y)) }
}

func union(fs ...sdf) sdf {
	return func(x, y float64) float64 {
		d := math.MaxFloat64
		for _, f := range fs {
			d = math.Min(d, f(x, y))
		}
		return d
	}
}

func disc(cx, cy, r float64) sdf {
	return func(x, y float64) float64 { return math.Hypot(x-cx, y-cy) - r }
}

func ring(cx, cy, r, t float64) sdf {
	return func(x, y float64) float64 { return math.Abs(math.Hypot(x-cx, y-cy)-r) - t/2 }
}

func bar(x0, y0, x1, y1, t float64) sdf {
	return func(x, y float64) float64 {
		dx, dy := x1-x0, y1-y0
		l := dx*dx + dy*dy
		u := 0.0
		if l > 0 {
			u = math.Max(0, math.Min(1, ((x-x0)*dx+(y-y0)*dy)/l))
		}
		return math.Hypot(x-(x0+u*dx), y-(y0+u*dy)) - t/2
	}
}

func arc(cx, cy, r, t, a0, a1 float64) sdf {
	band := ring(cx, cy, r, t)
	return func(x, y float64) float64 {
		a := math.Atan2(y-cy, x-cx)
		for a < a0 {
			a += 2 * math.Pi
		}
		if a <= a1 {
			return band(x, y)
		}
		return math.Min(
			math.Hypot(x-(cx+r*math.Cos(a0)), y-(cy+r*math.Sin(a0))),
			math.Hypot(x-(cx+r*math.Cos(a1)), y-(cy+r*math.Sin(a1)))) - t/2
	}
}

func wedge(x0, y0, x1, y1, x2, y2 float64) sdf {
	side := func(ax, ay, bx, by, px, py float64) float64 {
		return (bx-ax)*(py-ay) - (by-ay)*(px-ax)
	}
	return func(x, y float64) float64 {
		a := side(x0, y0, x1, y1, x, y)
		b := side(x1, y1, x2, y2, x, y)
		c := side(x2, y2, x0, y0, x, y)
		if (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0) {
			return -1
		}
		return 1
	}
}

func iconName(base string) string {
	return fmt.Sprintf("icon_%s_%d", base, iconGen)
}

func makeIcons() {
	old := images
	images = nil
	iconGen++
	size := imageSize()

	icons := map[string]sdf{
		"search":  union(ring(0.42, 0.42, 0.24, 0.09), bar(0.60, 0.60, 0.84, 0.84, 0.11)),
		"compose": union(bar(0.24, 0.76, 0.70, 0.28, 0.14), bar(0.18, 0.82, 0.26, 0.74, 0.10)),
		"call":    arc(0.50, 0.30, 0.34, 0.15, 0.30*math.Pi, 0.70*math.Pi),
		"info":    union(ring(0.5, 0.5, 0.36, 0.09), disc(0.5, 0.33, 0.055), bar(0.5, 0.46, 0.5, 0.68, 0.10)),
		"person":  union(disc(0.36, 0.32, 0.16), arc(0.36, 0.80, 0.26, 0.15, math.Pi, 2*math.Pi), bar(0.80, 0.30, 0.80, 0.54, 0.09), bar(0.68, 0.42, 0.92, 0.42, 0.09)),
		"plus":    union(bar(0.5, 0.22, 0.5, 0.78, 0.10), bar(0.22, 0.5, 0.78, 0.5, 0.10)),
		"smile":   union(ring(0.5, 0.5, 0.38, 0.09), disc(0.37, 0.40, 0.06), disc(0.63, 0.40, 0.06), arc(0.5, 0.50, 0.22, 0.09, 0.15*math.Pi, 0.85*math.Pi)),
		"send":    wedge(0.14, 0.14, 0.90, 0.5, 0.14, 0.86),
		"theme":   cut(disc(0.5, 0.5, 0.36), disc(0.68, 0.34, 0.32)),
	}
	for name, shape := range icons {
		img := iup.ImageRGBA(size, size, raster(size, layer{shape, pal.text}))
		img.SetHandle(iconName(name))
		images = append(images, img)
	}

	for handle, base := range imaged {
		handle.SetAttribute("IMAGE", iconName(base))
	}
	for _, img := range old {
		iup.Destroy(img)
	}
}

func imageSize() int {
	size := 0
	if n, err := strconv.Atoi(iup.GetGlobal("DEFAULTFONTSIZE")); err == nil {
		size = 2 * n
	}
	if size < 16 {
		size = 16
	}
	return size
}

func makeEmoticons() {
	size := imageSize()

	face := "250 196 62"
	eyes := "62 52 40"
	emo := map[string][]layer{
		":)": {{disc(0.5, 0.5, 0.46), face}, {disc(0.35, 0.40, 0.07), eyes}, {disc(0.65, 0.40, 0.07), eyes}, {arc(0.5, 0.48, 0.26, 0.10, 0.15*math.Pi, 0.85*math.Pi), eyes}},
		":D": {{disc(0.5, 0.5, 0.46), face}, {disc(0.35, 0.38, 0.07), eyes}, {disc(0.65, 0.38, 0.07), eyes}, {arc(0.5, 0.44, 0.30, 0.16, 0.10*math.Pi, 0.90*math.Pi), eyes}},
		";)": {{disc(0.5, 0.5, 0.46), face}, {bar(0.28, 0.40, 0.42, 0.40, 0.09), eyes}, {disc(0.65, 0.40, 0.07), eyes}, {arc(0.5, 0.48, 0.26, 0.10, 0.15*math.Pi, 0.85*math.Pi), eyes}},
		":(": {{disc(0.5, 0.5, 0.46), face}, {disc(0.35, 0.40, 0.07), eyes}, {disc(0.65, 0.40, 0.07), eyes}, {arc(0.5, 0.92, 0.26, 0.10, 1.15*math.Pi, 1.85*math.Pi), eyes}},
		"<3": {{union(disc(0.32, 0.36, 0.24), disc(0.68, 0.36, 0.24), wedge(0.08, 0.42, 0.92, 0.42, 0.5, 0.92)), "222 76 96"}},
	}
	for name, layers := range emo {
		iup.ImageRGBA(size, size, raster(size, layers...)).SetHandle("emo" + name)
	}

}

func raster(size int, layers ...layer) []byte {
	pix := make([]byte, size*size*4)
	const ss = 3
	for _, l := range layers {
		r, g, b := rgb(l.color)
		for y := 0; y < size; y++ {
			for x := 0; x < size; x++ {
				hits := 0
				for sy := 0; sy < ss; sy++ {
					for sx := 0; sx < ss; sx++ {
						u := (float64(x) + (float64(sx)+0.5)/ss) / float64(size)
						v := (float64(y) + (float64(sy)+0.5)/ss) / float64(size)
						if l.shape(u, v) <= 0 {
							hits++
						}
					}
				}
				if hits == 0 {
					continue
				}
				a := float64(hits) / (ss * ss)
				i := (y*size + x) * 4
				old := float64(pix[i+3]) / 255
				out := a + old*(1-a)
				pix[i] = byte((r*a + float64(pix[i])*old*(1-a)) / out)
				pix[i+1] = byte((g*a + float64(pix[i+1])*old*(1-a)) / out)
				pix[i+2] = byte((b*a + float64(pix[i+2])*old*(1-a)) / out)
				pix[i+3] = byte(out * 255)
			}
		}
	}
	return pix
}

func iconButton(name, tip string, cb iup.ActionFunc) iup.Ihandle {
	b := iup.Button("").SetAttributes(map[string]string{
		"IMAGE":    iconName(name),
		"TIP":      tip,
		"FLAT":     "YES",
		"CANFOCUS": "NO",
		"PADDING":  "4x4",
	})
	if cb != nil {
		b.SetCallback("ACTION", cb)
	}
	imaged[b] = name
	return b
}
