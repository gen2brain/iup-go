//go:build plot

package main

import (
	"encoding/csv"
	"encoding/json"
	"fmt"
	"os"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

var categories = []string{"Groceries", "Dining", "Transport", "Shopping", "Bills", "Other", "Income"}
var budgetNames = []string{"Groceries", "Dining", "Transport", "Shopping", "Bills"}

type transaction struct {
	ID       int    `json:"id"`
	Date     string `json:"date"`
	Payee    string `json:"payee"`
	Category string `json:"category"`
	Cents    int64  `json:"cents"`
	Note     string `json:"note"`
}

type dashboard struct {
	dialog, config, table, from, to, search, category, bulk, log iup.Ihandle
	categoryPlot, monthlyPlot, summary                           iup.Ihandle
	budgetBars, budgetLabels, budgetInputs                       []iup.Ihandle
	transactions                                                 []transaction
	visible                                                      []*transaction
	budgets                                                      [5]int64
	nextID, sortCol                                              int
	sortUp                                                       bool
}

func main() {
	iup.Open()
	iup.PlotOpen()
	iup.SetGlobal("UTF8MODE", "YES")
	iup.SetGlobal("APPID", "com.example.PersonalFinanceDashboard")
	iup.SetGlobal("APPNAME", "Personal Finance Dashboard")
	app := &dashboard{sortCol: 1, sortUp: false, nextID: 1}
	app.config = iup.Config().SetAttribute("APP_NAME", "IupPersonalFinanceExample")
	iup.ConfigLoad(app.config)
	app.load()
	app.build()
	app.refresh()
	iup.Show(app.dialog)
	app.log.SetAttribute("APPEND", "Sample ledger ready. Edit a cell, select rows, or change the date range.")
	iup.MainLoop()
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		return
	}
	app.save()
	app.dialog.Destroy()
	app.config.Destroy()
	iup.Close()
}

func phone() bool { return runtime.GOOS == "android" || runtime.GOOS == "ios" }

func fileButton(title string, action func()) iup.Ihandle {
	b := button(title, action)
	if runtime.GOOS == "js" {
		b.SetAttributes("VISIBLE=NO, FLOATING=YES")
	}
	return b
}

func (app *dashboard) load() {
	if json.Unmarshal([]byte(iup.ConfigGetVariableStr(app.config, "Ledger", "Transactions")), &app.transactions) != nil || len(app.transactions) == 0 {
		app.seed()
	}
	for _, item := range app.transactions {
		if item.ID >= app.nextID {
			app.nextID = item.ID + 1
		}
	}
	app.budgets = [5]int64{48000, 22000, 18000, 28000, 75000}
	for i := range app.budgets {
		if cents, err := strconv.ParseInt(iup.ConfigGetVariableStr(app.config, "Budgets", budgetNames[i]), 10, 64); err == nil && cents > 0 {
			app.budgets[i] = cents
		}
	}
	col := iup.ConfigGetVariableIntDef(app.config, "View", "SortColumn", 1)
	if col >= 1 && col <= 6 {
		app.sortCol = col
	}
	app.sortUp = iup.ConfigGetVariableIntDef(app.config, "View", "SortAscending", 0) != 0
}

func (app *dashboard) seed() {
	now := time.Now()
	first := time.Date(now.Year(), now.Month(), 1, 0, 0, 0, 0, time.Local)
	samples := []struct {
		payee, category string
		cents           int64
	}{
		{"Studio payroll", "Income", 345000}, {"Corner market", "Groceries", -6250},
		{"City utilities", "Bills", -14500}, {"Rail pass", "Transport", -7900},
		{"Lunch counter", "Dining", -2380}, {"Book shop", "Shopping", -4250},
		{"Fresh produce", "Groceries", -3520}, {"Internet service", "Bills", -6900},
		{"Coffee house", "Dining", -875}, {"Hardware store", "Other", -3940},
	}
	for month := 5; month >= 0; month-- {
		start := first.AddDate(0, -month, 0)
		for index, sample := range samples {
			day := 2 + index*2
			date := start.AddDate(0, 0, day-1)
			if date.After(now) {
				continue
			}
			amount := sample.cents
			if amount < 0 {
				amount -= int64((month+index)%4) * 175
			}
			app.transactions = append(app.transactions, transaction{ID: app.nextID, Date: date.Format("2006-01-02"), Payee: sample.payee, Category: sample.category, Cents: amount, Note: "Sample entry"})
			app.nextID++
		}
	}
}

func (app *dashboard) build() {
	app.table = iup.Table().SetAttributes("NUMCOL=6, EXPAND=YES, VISIBLELINES=12, SORTABLE=YES, USERRESIZE=YES, ALLOWREORDER=YES, SELECTIONMODE=MULTIPLE, EDITABLE=YES, ALTERNATECOLOR=YES, STRETCHLAST=YES, FOCUSRECT=NO")
	if phone() {
		app.table.SetAttribute("VISIBLELINES", 6)
	}
	for col, title := range []string{"Date", "Payee", "Category", "Amount", "Note", "ID"} {
		iup.SetAttributeId(app.table, "TITLE", col+1, title)
	}
	app.table.SetAttributes("ALIGNMENT4=ARIGHT, ALIGNMENT6=ARIGHT")
	app.table.SetCallback("SORT_CB", iup.TableSortFunc(func(_ iup.Ihandle, col int) int {
		if app.sortCol == col {
			app.sortUp = !app.sortUp
		} else {
			app.sortCol, app.sortUp = col, true
		}
		app.refresh()
		return iup.IGNORE
	}))
	app.table.SetCallback("EDITBEGIN_CB", iup.EditBeginFunc(func(_ iup.Ihandle, _, col int) int {
		if col == 6 {
			return iup.IGNORE
		}
		return iup.DEFAULT
	}))
	app.table.SetCallback("EDITEND_CB", iup.EditEndFunc(app.editCell))
	app.table.SetCallback("MULTISELECTION_CB", iup.MultiSelectionFunc(func(_ iup.Ihandle, _ []int, n int) int {
		app.summary.SetAttribute("TITLE", fmt.Sprintf("%d matching  |  %d selected", len(app.visible), n))
		return iup.DEFAULT
	}))

	now := time.Now()
	first := time.Date(now.Year(), now.Month(), 1, 0, 0, 0, 0, time.Local).AddDate(0, -5, 0)
	app.from = iup.DatePick().SetAttribute("VALUE", iup.ConfigGetVariableStrDef(app.config, "View", "From", first.Format("2006/1/2")))
	app.to = iup.DatePick().SetAttribute("VALUE", time.Now().Format("2006/1/2"))
	for _, picker := range []iup.Ihandle{app.from, app.to} {
		picker.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(iup.Ihandle) int { app.refresh(); return iup.DEFAULT }))
	}
	app.search = iup.Text().SetAttributes("EXPAND=HORIZONTAL, VISIBLECOLUMNS=17, CUEBANNER=Search").SetAttribute("VALUE", iup.ConfigGetVariableStr(app.config, "View", "Search"))
	app.search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(iup.Ihandle) int { app.refresh(); return iup.DEFAULT }))
	app.category = categoryList(true)
	app.category.SetAttribute("VALUE", iup.ConfigGetVariableIntDef(app.config, "View", "Category", 1))
	app.category.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, _, state int) int {
		if state == 1 {
			app.refresh()
		}
		return iup.DEFAULT
	}))
	app.bulk = categoryList(false)
	app.bulk.SetAttribute("VALUE", 1)
	app.summary = iup.Label("").SetAttributes("EXPAND=HORIZONTAL, ALIGNMENT=ARIGHT")
	app.log = iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, WORDWRAP=YES, EXPAND=HORIZONTAL, VISIBLELINES=2")
	app.categoryPlot = financePlot("Spending by category")
	app.monthlyPlot = financePlot("Monthly cash flow")

	var transactionPanel iup.Ihandle
	if phone() {
		filters := iup.GridBox(iup.Label("From"), app.from, iup.Label("To"), app.to).
			SetAttributes("NUMDIV=2, SIZELIN=-1, SIZECOL=-1, ALIGNMENTLIN=ACENTER, NGAPCOL=5, NGAPLIN=4")
		actions := iup.Vbox(
			iup.Hbox(fileButton("Import CSV", app.importCSV), fileButton("Export CSV", app.exportCSV)).SetAttributes("NGAP=5"),
			app.bulk,
			button("Categorize selected", app.bulkCategory),
		).SetAttributes("NGAP=5")
		transactionPanel = iup.Vbox(filters, app.category, app.search, actions, app.table, app.summary).SetAttributes("NGAP=6, NMARGIN=8x8")
	} else {
		filters := iup.Hbox(iup.Label("From"), app.from, iup.Label("To"), app.to, app.category, app.search).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")
		actions := iup.Hbox(fileButton("Import CSV", app.importCSV), fileButton("Export CSV", app.exportCSV), iup.Label("Categorize"), app.bulk, button("Apply to selected", app.bulkCategory)).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")
		transactionPanel = iup.Vbox(filters, actions, app.table, app.summary).SetAttributes("NGAP=6, NMARGIN=8x8")
	}
	var chartPanel iup.Ihandle
	if phone() {
		app.monthlyPlot.SetAttribute("LEGENDSHOW", "NO")
		chartPanel = iup.Vbox(iup.Label("Income: green   Expenses: blue"), app.categoryPlot, app.monthlyPlot).SetAttributes("EXPAND=YES, NGAP=6")
	} else {
		chartPanel = iup.Vbox(app.categoryPlot, app.monthlyPlot).SetAttributes("EXPAND=YES, NGAP=6")
	}
	budgetPanel := app.budgetPanel()
	var center iup.Ihandle
	if phone() {
		transactionPanel.SetAttribute("TABTITLE", "Transactions")
		chartPanel.SetAttribute("TABTITLE", "Charts")
		budgetPanel.SetAttribute("TABTITLE", "Budgets")
		dashboardPanel := iup.Tabs(chartPanel, budgetPanel).SetAttributes("TABTITLE=Dashboard, EXPAND=YES")
		center = iup.Tabs(transactionPanel, dashboardPanel).SetAttribute("EXPAND", "YES")
	} else {
		dashboardPanel := iup.Vbox(chartPanel, budgetPanel).SetAttributes("EXPAND=YES, NGAP=6, NMARGIN=8x8")
		center = iup.Split(transactionPanel, dashboardPanel).SetAttributes("ORIENTATION=VERTICAL, VALUE=570, MINMAX=430:780")
	}
	content := iup.Vbox(center, app.log).SetAttributes("NGAP=0")
	app.dialog = iup.Dialog(content).SetAttribute("TITLE", "Personal Finance Dashboard")
	if !phone() {
		app.dialog.SetAttribute("PLACEMENT", "MAXIMIZED")
	}
	app.dialog.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(_ iup.Ihandle, message string, _ int, _ any) int {
		if message == "refresh" {
			app.refresh()
			app.save()
		}
		return iup.DEFAULT
	}))
	app.dialog.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int { app.save(); return iup.CLOSE }))
	app.dialog.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int {
		app.refreshPlots()
		return iup.DEFAULT
	}))
}

func categoryList(all bool) iup.Ihandle {
	list := iup.List().SetAttributes("DROPDOWN=YES, VISIBLECOLUMNS=11")
	index := 1
	if all {
		list.SetAttributeId("", index, "All categories")
		index++
	}
	for _, name := range categories {
		list.SetAttributeId("", index, name)
		index++
	}
	list.SetAttribute("VALUE", 1)
	return list
}

func (app *dashboard) budgetPanel() iup.Ihandle {
	app.budgetBars = make([]iup.Ihandle, len(budgetNames))
	app.budgetLabels = make([]iup.Ihandle, len(budgetNames))
	app.budgetInputs = make([]iup.Ihandle, len(budgetNames))
	rows := []iup.Ihandle{iup.Label("This month's budgets").SetAttribute("FONTSTYLE", "Bold")}
	for i, name := range budgetNames {
		index := i
		app.budgetBars[i] = iup.ProgressBar().SetAttributes("MIN=0, MAX=100, EXPAND=HORIZONTAL")
		app.budgetLabels[i] = iup.Label("")
		app.budgetInputs[i] = iup.Text().SetAttributes("VISIBLECOLUMNS=7").SetAttribute("VALUE", amount(app.budgets[i]))
		app.budgetInputs[i].SetCallback("KILLFOCUS_CB", iup.KillFocusFunc(func(ih iup.Ihandle) int {
			cents, err := parseAmount(ih.GetAttribute("VALUE"))
			if err != nil || cents <= 0 {
				ih.SetAttribute("VALUE", amount(app.budgets[index]))
				app.log.SetAttribute("APPEND", "Budget must be a positive amount")
			} else {
				app.budgets[index] = cents
				ih.SetAttribute("VALUE", amount(cents))
				app.refreshBudgets()
				app.save()
			}
			return iup.DEFAULT
		}))
		if phone() {
			rows = append(rows, iup.Vbox(
				iup.Hbox(iup.Label(name), iup.Fill(), app.budgetInputs[i]).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"),
				app.budgetBars[i], app.budgetLabels[i],
			).SetAttributes("NGAP=2"))
		} else {
			rows = append(rows, iup.Hbox(iup.Label(name), app.budgetBars[i], app.budgetLabels[i], app.budgetInputs[i]).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"))
		}
	}
	return iup.Vbox(rows...).SetAttributes("NGAP=5")
}

func financePlot(title string) iup.Ihandle {
	return iup.Plot().SetAttributes(map[string]string{
		"TITLE": title, "EXPAND": "YES", "GRID": "YES", "AXS_YAUTOMIN": "NO", "AXS_YMIN": "0", "LEGENDSHOW": "NO",
	})
}

func (app *dashboard) refresh() {
	if app.table == 0 {
		return
	}
	selected := app.selectedIDs()
	from := parsePicker(app.from.GetAttribute("VALUE"))
	to := parsePicker(app.to.GetAttribute("VALUE"))
	if from.After(to) {
		app.log.SetAttribute("APPEND", "Start date must be on or before end date")
		return
	}
	query := strings.ToLower(strings.TrimSpace(app.search.GetAttribute("VALUE")))
	category := app.category.GetInt("VALUE")
	app.visible = app.visible[:0]
	for i := range app.transactions {
		item := &app.transactions[i]
		date, err := time.Parse("2006-01-02", item.Date)
		if err != nil || date.Before(from) || date.After(to) {
			continue
		}
		if category >= 2 && category-2 < len(categories) && item.Category != categories[category-2] {
			continue
		}
		if query != "" && !strings.Contains(strings.ToLower(item.Payee+" "+item.Note+" "+item.Category), query) {
			continue
		}
		app.visible = append(app.visible, item)
	}
	sort.SliceStable(app.visible, func(i, j int) bool {
		a, b := app.visible[i], app.visible[j]
		var cmp int
		switch app.sortCol {
		case 1:
			cmp = strings.Compare(a.Date, b.Date)
		case 2:
			cmp = strings.Compare(strings.ToLower(a.Payee), strings.ToLower(b.Payee))
		case 3:
			cmp = strings.Compare(a.Category, b.Category)
		case 4:
			if a.Cents < b.Cents {
				cmp = -1
			} else if a.Cents > b.Cents {
				cmp = 1
			}
		case 5:
			cmp = strings.Compare(strings.ToLower(a.Note), strings.ToLower(b.Note))
		case 6:
			if a.ID < b.ID {
				cmp = -1
			} else if a.ID > b.ID {
				cmp = 1
			}
		}
		if app.sortUp {
			return cmp < 0
		}
		return cmp > 0
	})
	app.table.SetAttribute("NUMLIN", len(app.visible))
	marks := make([]byte, len(app.visible))
	for i, item := range app.visible {
		marks[i] = '-'
		if selected[item.ID] {
			marks[i] = '+'
		}
		values := []string{item.Date, item.Payee, item.Category, amount(item.Cents), item.Note, strconv.Itoa(item.ID)}
		for col, value := range values {
			iup.SetAttributeId2(app.table, "", i+1, col+1, value)
		}
	}
	if len(marks) > 0 {
		app.table.SetAttribute("SELECTEDLINES", string(marks))
	}
	iup.SetAttributeId(app.table, "SORTSIGN", app.sortCol, map[bool]string{true: "UP", false: "DOWN"}[app.sortUp])
	app.summary.SetAttribute("TITLE", fmt.Sprintf("%d matching  |  %d selected", len(app.visible), strings.Count(string(marks), "+")))
	app.refreshPlots()
	app.refreshBudgets()
}

func (app *dashboard) selectedIDs() map[int]bool {
	selected := make(map[int]bool)
	if app.table == 0 {
		return selected
	}
	marks := app.table.GetAttribute("SELECTEDLINES")
	for i, mark := range marks {
		if mark == '+' && i < len(app.visible) {
			selected[app.visible[i].ID] = true
		}
	}
	return selected
}

func (app *dashboard) editCell(_ iup.Ihandle, line, col int, value string, apply int) int {
	if apply == 0 || line < 1 || line > len(app.visible) || col == 6 {
		return iup.DEFAULT
	}
	item := app.visible[line-1]
	value = strings.TrimSpace(value)
	switch col {
	case 1:
		if _, err := time.Parse("2006-01-02", value); err != nil {
			app.log.SetAttribute("APPEND", "Date must be YYYY-MM-DD")
			return iup.IGNORE
		}
		item.Date = value
	case 2:
		if value == "" {
			app.log.SetAttribute("APPEND", "Payee cannot be empty")
			return iup.IGNORE
		}
		item.Payee = value
	case 3:
		found := false
		for _, category := range categories {
			if strings.EqualFold(value, category) {
				item.Category = category
				found = true
				break
			}
		}
		if !found {
			app.log.SetAttribute("APPEND", "Choose a listed category")
			return iup.IGNORE
		}
	case 4:
		cents, err := parseAmount(value)
		if err != nil || cents == 0 {
			app.log.SetAttribute("APPEND", "Amount must be a nonzero number with at most two decimals")
			return iup.IGNORE
		}
		item.Cents = cents
	case 5:
		item.Note = value
	}
	iup.PostMessage(app.dialog, "refresh", 0, nil)
	return iup.IGNORE
}

func (app *dashboard) bulkCategory() {
	selected := app.selectedIDs()
	if len(selected) == 0 {
		if phone() {
			app.log.SetAttribute("APPEND", "Select transactions first")
		} else {
			app.log.SetAttribute("APPEND", "Select transactions with Ctrl+Click or Shift+Click first")
		}
		return
	}
	index := app.bulk.GetInt("VALUE") - 1
	if index < 0 || index >= len(categories) {
		return
	}
	for i := range app.transactions {
		if selected[app.transactions[i].ID] {
			app.transactions[i].Category = categories[index]
		}
	}
	app.refresh()
	app.save()
	app.log.SetAttribute("APPEND", fmt.Sprintf("Categorized %d transactions as %s", len(selected), categories[index]))
}

func (app *dashboard) refreshPlots() {
	if app.categoryPlot == 0 {
		return
	}
	var expenses [6]float64
	monthly := make(map[string][2]float64)
	for _, item := range app.visible {
		month := item.Date[:7]
		pair := monthly[month]
		if item.Cents < 0 {
			pair[1] += float64(-item.Cents) / 100
			for i, name := range categories[:6] {
				if name == item.Category {
					expenses[i] += float64(-item.Cents) / 100
				}
			}
		} else {
			pair[0] += float64(item.Cents) / 100
		}
		monthly[month] = pair
	}
	app.categoryPlot.SetAttribute("CLEAR", "YES")
	iup.PlotBegin(app.categoryPlot, 1)
	for i, name := range categories[:6] {
		iup.PlotAddStr(app.categoryPlot, name, expenses[i])
	}
	iup.PlotEnd(app.categoryPlot)
	app.categoryPlot.SetAttributes("DS_MODE=BAR, DS_BARMULTICOLOR=YES, DS_BARSPACING=35")
	months := make([]string, 0, len(monthly))
	for month := range monthly {
		months = append(months, month)
	}
	sort.Strings(months)
	if len(months) > 6 {
		months = months[len(months)-6:]
	}
	app.monthlyPlot.SetAttribute("CLEAR", "YES")
	for series, title := range []string{"Income", "Expenses"} {
		iup.PlotBegin(app.monthlyPlot, 1)
		for _, month := range months {
			iup.PlotAddStr(app.monthlyPlot, month[5:], monthly[month][series])
		}
		iup.PlotEnd(app.monthlyPlot)
		app.monthlyPlot.SetAttributes(map[string]string{"DS_MODE": "MULTIBAR", "DS_LEGEND": title})
	}
	if !phone() {
		app.monthlyPlot.SetAttribute("LEGENDSHOW", "YES")
	}
	app.themePlots()
}

func (app *dashboard) themePlots() {
	bg, fg, grid := "255 255 255", "38 45 55", "222 225 232"
	colors := []string{"52 125 214", "61 163 136", "235 156 77", "153 111 201", "77 172 204", "202 111 133"}
	if iup.GetGlobalBool("DARKMODE") {
		bg, fg, grid = "32 36 43", "230 233 239", "69 76 87"
		colors = []string{"116 177 251", "98 208 174", "244 185 111", "190 152 239", "115 204 231", "229 148 167"}
	}
	for _, plot := range []iup.Ihandle{app.categoryPlot, app.monthlyPlot} {
		plot.SetAttributes(map[string]string{"BACKCOLOR": bg, "FGCOLOR": fg, "AXS_XCOLOR": fg, "AXS_YCOLOR": fg, "GRIDCOLOR": grid})
	}
	for i, color := range colors {
		iup.SetAttributeId(app.categoryPlot, "SAMPLECOLOR", i, color)
	}
	for i, color := range []string{colors[1], colors[0]} {
		app.monthlyPlot.SetAttribute("CURRENT", i)
		app.monthlyPlot.SetAttribute("DS_COLOR", color)
	}
	app.categoryPlot.SetAttribute("REDRAW", "YES")
	app.monthlyPlot.SetAttribute("REDRAW", "YES")
}

func (app *dashboard) refreshBudgets() {
	month := time.Now().Format("2006-01")
	var spent [5]int64
	for _, item := range app.transactions {
		if item.Cents >= 0 || !strings.HasPrefix(item.Date, month) {
			continue
		}
		for i, name := range budgetNames {
			if item.Category == name {
				spent[i] -= item.Cents
			}
		}
	}
	for i := range spent {
		if app.budgetBars[i] == 0 {
			continue
		}
		percent := 100 * float64(spent[i]) / float64(app.budgets[i])
		app.budgetBars[i].SetAttribute("VALUE", min(percent, 100))
		app.budgetLabels[i].SetAttribute("TITLE", amount(spent[i])+" / "+amount(app.budgets[i]))
	}
}

func (app *dashboard) importCSV() {
	dialog := iup.FileDlg().SetAttributes(`DIALOGTYPE=OPEN, TITLE="Import transactions", EXTFILTER="CSV files|*.csv|All files|*.*"`)
	iup.SetAttributeHandle(dialog, "PARENTDIALOG", app.dialog)
	iup.Popup(dialog, iup.CENTERPARENT, iup.CENTERPARENT)
	path := dialog.GetAttribute("VALUE")
	ok := dialog.GetInt("STATUS") != -1
	var data []byte
	var err error
	if ok {
		data, err = os.ReadFile(path)
	}
	dialog.Destroy()
	if !ok {
		return
	}
	if err != nil {
		app.log.SetAttribute("APPEND", "Import failed: "+err.Error())
		return
	}
	reader := csv.NewReader(strings.NewReader(strings.TrimPrefix(string(data), "\ufeff")))
	reader.FieldsPerRecord = -1
	rows, err := reader.ReadAll()
	if err != nil || len(rows) == 0 || len(rows[0]) != 5 || !strings.EqualFold(strings.Join(rows[0], ","), "date,payee,category,amount,note") {
		app.log.SetAttribute("APPEND", "CSV requires Date,Payee,Category,Amount,Note columns")
		return
	}
	var incoming []transaction
	for index, row := range rows[1:] {
		if len(row) != 5 {
			app.log.SetAttribute("APPEND", fmt.Sprintf("Invalid CSV row %d", index+2))
			return
		}
		if _, err := time.Parse("2006-01-02", strings.TrimSpace(row[0])); err != nil {
			app.log.SetAttribute("APPEND", fmt.Sprintf("Invalid date at row %d", index+2))
			return
		}
		category := ""
		for _, name := range categories {
			if strings.EqualFold(strings.TrimSpace(row[2]), name) {
				category = name
			}
		}
		cents, err := parseAmount(row[3])
		if strings.TrimSpace(row[1]) == "" || category == "" || err != nil || cents == 0 {
			app.log.SetAttribute("APPEND", fmt.Sprintf("Invalid transaction at row %d", index+2))
			return
		}
		incoming = append(incoming, transaction{Date: strings.TrimSpace(row[0]), Payee: strings.TrimSpace(row[1]), Category: category, Cents: cents, Note: row[4]})
	}
	for _, item := range incoming {
		item.ID = app.nextID
		app.nextID++
		app.transactions = append(app.transactions, item)
	}
	app.refresh()
	app.save()
	app.log.SetAttribute("APPEND", fmt.Sprintf("Imported %d transactions", len(incoming)))
}

func (app *dashboard) exportCSV() {
	dialog := iup.FileDlg().SetAttributes(`DIALOGTYPE=SAVE, TITLE="Export visible transactions", EXTFILTER="CSV files|*.csv|All files|*.*", EXTDEFAULT=csv`)
	iup.SetAttributeHandle(dialog, "PARENTDIALOG", app.dialog)
	iup.Popup(dialog, iup.CENTERPARENT, iup.CENTERPARENT)
	defer dialog.Destroy()
	if dialog.GetInt("STATUS") == -1 {
		return
	}
	file, err := os.Create(dialog.GetAttribute("VALUE"))
	if err != nil {
		app.log.SetAttribute("APPEND", "Export failed: "+err.Error())
		return
	}
	writer := csv.NewWriter(file)
	err = writer.Write([]string{"Date", "Payee", "Category", "Amount", "Note"})
	for _, item := range app.visible {
		if err == nil {
			err = writer.Write([]string{item.Date, item.Payee, item.Category, amount(item.Cents), item.Note})
		}
	}
	writer.Flush()
	if err == nil {
		err = writer.Error()
	}
	if closeErr := file.Close(); err == nil {
		err = closeErr
	}
	if err != nil {
		app.log.SetAttribute("APPEND", "Export failed: "+err.Error())
	} else {
		app.log.SetAttribute("APPEND", fmt.Sprintf("Exported %d visible transactions", len(app.visible)))
	}
}

func (app *dashboard) save() {
	iup.ConfigSetVariableStr(app.config, "View", "From", app.from.GetAttribute("VALUE"))
	iup.ConfigSetVariableStr(app.config, "View", "Search", app.search.GetAttribute("VALUE"))
	iup.ConfigSetVariableInt(app.config, "View", "Category", app.category.GetInt("VALUE"))
	iup.ConfigSetVariableInt(app.config, "View", "SortColumn", app.sortCol)
	if app.sortUp {
		iup.ConfigSetVariableInt(app.config, "View", "SortAscending", 1)
	} else {
		iup.ConfigSetVariableInt(app.config, "View", "SortAscending", 0)
	}
	for i, name := range budgetNames {
		iup.ConfigSetVariableStr(app.config, "Budgets", name, strconv.FormatInt(app.budgets[i], 10))
	}
	if data, err := json.Marshal(app.transactions); err == nil {
		iup.ConfigSetVariableStr(app.config, "Ledger", "Transactions", string(data))
	}
	iup.ConfigSave(app.config)
}

func parsePicker(value string) time.Time {
	date, err := time.Parse("2006/1/2", value)
	if err != nil {
		return time.Time{}
	}
	return date
}

func parseAmount(value string) (int64, error) {
	value = strings.ReplaceAll(strings.TrimSpace(value), ",", "")
	sign := int64(1)
	if strings.HasPrefix(value, "-") {
		sign = -1
		value = value[1:]
	} else if strings.HasPrefix(value, "+") {
		value = value[1:]
	}
	value = strings.TrimPrefix(value, "$")
	parts := strings.Split(value, ".")
	if len(parts) > 2 || !digits(parts[0]) || len(parts) == 2 && !digits(parts[1]) {
		return 0, fmt.Errorf("invalid amount")
	}
	whole, err := strconv.ParseInt(parts[0], 10, 64)
	if err != nil || whole > (1<<63-1)/100 {
		return 0, fmt.Errorf("invalid amount")
	}
	var fractional int64
	if len(parts) == 2 {
		if len(parts[1]) < 1 || len(parts[1]) > 2 {
			return 0, fmt.Errorf("invalid amount")
		}
		fractional, err = strconv.ParseInt(parts[1], 10, 64)
		if err != nil {
			return 0, err
		}
		if len(parts[1]) == 1 {
			fractional *= 10
		}
	}
	if whole > ((1<<63-1)-fractional)/100 {
		return 0, fmt.Errorf("invalid amount")
	}
	return sign * (whole*100 + fractional), nil
}

func digits(value string) bool {
	if value == "" {
		return false
	}
	for _, r := range value {
		if r < '0' || r > '9' {
			return false
		}
	}
	return true
}

func amount(cents int64) string {
	if cents < 0 {
		return fmt.Sprintf("-$%d.%02d", -cents/100, -cents%100)
	}
	return fmt.Sprintf("$%d.%02d", cents/100, cents%100)
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttribute("PADDING", "5x3")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int { action(); return iup.DEFAULT }))
	return b
}
