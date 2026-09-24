package main

import (
	"context"
	"fmt"
	"net/mail"
	"runtime"
	"strings"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type step struct {
	title string
	panel iup.Ihandle
	first iup.Ihandle
}

type wizard struct {
	dialog, pages, heading, eventLog, back, next, cancel        iup.Ihandle
	name, email, username, password, confirm                    iup.Ihandle
	mode, encrypted, file, date, pin, consent                   iup.Ihandle
	choose, progress, progressText                              iup.Ihandle
	welcome, account, options, custom, security, review, finish *step
	current, popover                                            iup.Ihandle
	jobCancel                                                   context.CancelFunc
	jobSerial, pending                                          int
	working, finished, closing                                  bool
}

func main() {
	iup.Open()
	iup.SetGlobal("UTF8MODE", "YES")
	iup.SetGlobal("APPID", "com.example.SetupWizard")
	iup.SetGlobal("APPNAME", "Setup Wizard")
	app := &wizard{}
	app.build()
	iup.Show(app.dialog)
	app.show(app.welcome, false)
	app.log("Wizard ready. Tab moves between fields; Enter advances.")
	iup.MainLoop()
	if runtime.GOOS == "android" || runtime.GOOS == "ios" {
		return
	}
	app.hideError()
	app.dialog.Destroy()
	iup.Close()
}

func mobile() bool { return runtime.GOOS == "android" || runtime.GOOS == "ios" }

func (app *wizard) build() {
	app.name = app.field("Full name", "Alex Morgan")
	app.email = app.field("Email address", "alex@example.test")
	app.username = app.field("User name", "alexmorgan")
	app.username.SetAttribute("MASK", "/l/w*")
	app.password = app.field("Password", "examplepass").SetAttribute("PASSWORD", "YES")
	app.confirm = app.field("Confirm password", "examplepass").SetAttribute("PASSWORD", "YES")
	app.mode = iup.List().SetAttributes("DROPDOWN=YES, VISIBLECOLUMNS=13, ACCESSIBLETITLE=Installation mode")
	app.mode.SetAttributeId("", 1, "Standard")
	app.mode.SetAttributeId("", 2, "Custom")
	app.mode.SetAttribute("VALUE", 1)
	app.mode.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, selected int) int {
		if selected == 1 {
			app.log("Selected " + map[bool]string{true: "Custom", false: "Standard"}[item == 2] + " mode")
			app.reconcile()
		}
		return iup.DEFAULT
	}))
	app.encrypted = iup.Toggle("&Encrypt backup").SetAttribute("ACCESSIBLETITLE", "Encrypt backup")
	app.encrypted.SetCallback("ACTION", iup.ToggleActionFunc(func(_ iup.Ihandle, _ int) int {
		app.reconcile()
		return iup.DEFAULT
	}))
	app.consent = iup.Toggle("I have &reviewed the setup details").SetAttribute("ACCESSIBLETITLE", "Confirm setup details")
	app.progress = iup.ProgressBar().SetAttributes("MIN=0, MAX=100, EXPAND=HORIZONTAL, ACCESSIBLETITLE=Installation progress")
	app.progressText = iup.Label("Ready to install").SetAttributes("EXPAND=HORIZONTAL, WORDWRAP=YES")
	app.heading = iup.Label("").SetAttributes("FONTSTYLE=Bold, FONTSIZE=15, EXPAND=HORIZONTAL")
	app.eventLog = iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, WORDWRAP=YES, CANFOCUS=NO, EXPAND=HORIZONTAL, VISIBLELINES=3, ACCESSIBLETITLE=Wizard activity")

	app.welcome = app.page("Welcome", app.name, app.form("Create a local workspace for this example.", []string{"Full name", "Email"}, []iup.Ihandle{app.name, app.email}))
	app.account = app.page("Account", app.username, app.form("Choose a user name and password.", []string{"User name", "Password", "Confirm"}, []iup.Ihandle{app.username, app.password, app.confirm}))
	app.options = app.page("Options", app.mode, app.form("Optional pages appear as these settings change.", []string{"Mode", ""}, []iup.Ihandle{app.mode, app.encrypted}))
	app.review = app.page("Review", app.consent, iup.Vbox(
		iup.Label("Review your selections before installing.").SetAttribute("FONTSTYLE", "Bold"),
		iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, CANFOCUS=NO, EXPAND=YES, VISIBLELINES=6, VISIBLECOLUMNS=32, ACCESSIBLETITLE=Review summary").SetHandle("wizard_summary"),
		app.consent,
	).SetAttributes("NGAP=7, NMARGIN=8x8"))
	app.finish = app.page("Install", 0, iup.Vbox(
		iup.Label("Setting up your workspace").SetAttribute("FONTSTYLE", "Bold"),
		app.progress,
		app.progressText,
		iup.Fill(),
	).SetAttributes("NGAP=10, NMARGIN=8x8"))
	app.pages = iup.Zbox(app.welcome.panel, app.account.panel, app.options.panel, app.review.panel, app.finish.panel).
		SetAttributes("EXPAND=YES, CHILDSIZEALL=YES")
	app.back = button("&Back", "Go to previous step", app.previous)
	app.next = button("&Next", "Go to next step", app.advance)
	app.cancel = button("&Cancel", "Cancel setup", app.abort)
	if runtime.GOOS == "ios" {
		app.cancel.SetAttributes("VISIBLE=NO, FLOATING=YES")
	}
	navigation := iup.Hbox(app.back, iup.Fill(), app.cancel, app.next).SetAttributes("NGAP=7, ALIGNMENT=ACENTER")
	app.dialog = iup.Dialog(iup.Vbox(app.heading, app.pages, navigation, app.eventLog).SetAttributes("NGAP=8, NMARGIN=10x10"))
	app.dialog.SetAttribute("TITLE", "Setup Wizard")
	app.dialog.SetAttributeHandle("DEFAULTENTER", app.next)
	app.dialog.SetAttributeHandle("DEFAULTESC", app.cancel)
	app.dialog.SetAttributeHandle("STARTFOCUS", app.name)
	app.dialog.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int {
		app.closing = true
		app.hideError()
		if app.pending > 0 {
			if app.jobCancel != nil {
				app.jobCancel()
			}
			return iup.IGNORE
		}
		return iup.CLOSE
	}))
	app.dialog.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(func(_ iup.Ihandle, kind string, serial int, payload any) int {
		if kind == "complete" {
			app.pending--
			if app.closing {
				if app.pending == 0 {
					app.end()
				}
				return iup.DEFAULT
			}
		}
		if serial != app.jobSerial {
			return iup.DEFAULT
		}
		switch kind {
		case "progress":
			if !app.closing && app.working {
				app.progress.SetAttribute("VALUE", payload.(int))
				app.progressText.SetAttribute("TITLE", fmt.Sprintf("Installing... %d%%", payload.(int)))
			}
		case "complete":
			if app.jobCancel != nil {
				app.jobCancel()
			}
			app.jobCancel = nil
			if app.working {
				app.working = false
				app.finished = true
				app.progress.SetAttribute("VALUE", 100)
				app.progressText.SetAttribute("TITLE", "Setup complete. Your workspace is ready.")
				app.log("Setup completed")
				app.updateNavigation()
			}
		}
		return iup.DEFAULT
	}))
	for _, item := range []struct {
		field iup.Ihandle
		name  string
	}{
		{app.name, "Full name"}, {app.email, "Email"}, {app.username, "User name"},
		{app.password, "Password"}, {app.confirm, "Confirm password"},
		{app.encrypted, "Encryption"}, {app.consent, "Review confirmation"},
	} {
		app.track(item.field, item.name)
	}
}

func (app *wizard) field(name, value string) iup.Ihandle {
	return iup.Text().SetAttributes("EXPAND=HORIZONTAL, VISIBLECOLUMNS=21").
		SetAttribute("ACCESSIBLETITLE", name).SetAttribute("VALUE", value)
}

func (app *wizard) form(description string, labels []string, controls []iup.Ihandle) iup.Ihandle {
	var fields iup.Ihandle
	if mobile() {
		rows := make([]iup.Ihandle, 0, len(controls)*2)
		for i, field := range controls {
			if labels[i] != "" {
				rows = append(rows, iup.Label(labels[i]))
			}
			rows = append(rows, field)
		}
		fields = iup.Vbox(rows...).SetAttributes("NGAP=5")
	} else {
		rows := make([]iup.Ihandle, 0, len(controls)*2)
		for i, field := range controls {
			rows = append(rows, iup.Label(labels[i]), field)
		}
		fields = iup.GridBox(rows...).SetAttributes("NUMDIV=2, SIZELIN=-1, SIZECOL=-1, ALIGNMENTLIN=ACENTER, NGAPLIN=8, NGAPCOL=8")
	}
	return iup.Vbox(iup.Label(description), fields, iup.Fill()).
		SetAttributes("NGAP=10, NMARGIN=8x8, EXPAND=YES")
}

func (app *wizard) page(title string, first, child iup.Ihandle) *step {
	return &step{title: title, first: first, panel: iup.BackgroundBox(child).SetAttribute("EXPAND", "YES")}
}

func (app *wizard) steps() []*step {
	steps := []*step{app.welcome, app.account, app.options}
	if app.custom != nil {
		steps = append(steps, app.custom)
	}
	if app.security != nil {
		steps = append(steps, app.security)
	}
	return append(steps, app.review, app.finish)
}

func (app *wizard) reconcile() {
	app.hideError()
	custom := app.mode.GetInt("VALUE") == 2
	secure := app.encrypted.GetBool("VALUE")
	if !custom && app.custom != nil {
		app.custom.panel.Destroy()
		app.custom, app.file, app.date, app.choose = nil, 0, 0, 0
		app.log("Removed custom storage step")
	}
	if !secure && app.security != nil {
		app.security.panel.Destroy()
		app.security, app.pin = nil, 0
		app.log("Removed encryption step")
	}
	if custom && app.custom == nil {
		app.createCustom()
		ref := app.review.panel
		if app.security != nil {
			ref = app.security.panel
		}
		iup.Insert(app.pages, ref, app.custom.panel)
		if app.pages.GetAttribute("WID") != "" {
			iup.Map(app.custom.panel)
		}
		app.log("Inserted custom storage step")
	}
	if secure && app.security == nil {
		app.createSecurity()
		iup.Insert(app.pages, app.review.panel, app.security.panel)
		if app.pages.GetAttribute("WID") != "" {
			iup.Map(app.security.panel)
		}
		app.log("Inserted encryption step")
	}
	if app.dialog != 0 {
		if !mobile() {
			app.dialog.SetAttribute("SIZE", "")
		}
		iup.Refresh(app.dialog)
		app.updateNavigation()
	}
}

func (app *wizard) createCustom() {
	app.file = app.field("Configuration file", "")
	app.file.SetAttributes("READONLY=YES, CUEBANNER=Optional configuration file")
	app.choose = button("B&rowse", "Select configuration file", func() {
		dlg := iup.FileDlg().SetAttributes("DIALOGTYPE=OPEN, TITLE=Choose configuration file")
		dlg.SetAttributeHandle("PARENTDIALOG", app.dialog)
		iup.Popup(dlg, iup.CENTERPARENT, iup.CENTERPARENT)
		if dlg.GetInt("STATUS") != -1 {
			app.file.SetAttribute("VALUE", dlg.GetAttribute("VALUE"))
			app.log("Selected configuration file")
		}
		dlg.Destroy()
	})
	app.date = iup.DatePick().SetAttribute("ACCESSIBLETITLE", "Installation date")
	var fileRow iup.Ihandle
	if mobile() {
		fileRow = iup.Vbox(app.file, app.choose).SetAttributes("NGAP=5")
	} else {
		fileRow = iup.Hbox(app.file, app.choose).SetAttributes("NGAP=5, ALIGNMENT=ACENTER")
	}
	app.custom = app.page("Storage", app.choose, iup.Vbox(
		iup.Label("Custom storage").SetAttribute("FONTSTYLE", "Bold"),
		iup.Label("Choose a file and installation date (optional)."),
		fileRow,
		iup.Hbox(iup.Label("Install on"), app.date).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"),
		iup.Fill(),
	).SetAttributes("NGAP=9, NMARGIN=8x8, EXPAND=YES"))
	app.track(app.file, "Configuration file")
	app.track(app.choose, "Browse files")
	app.track(app.date, "Installation date")
}

func (app *wizard) createSecurity() {
	app.pin = app.field("Six-digit recovery PIN", "")
	app.pin.SetAttributes("MASK=/d/d/d/d/d/d, PASSWORD=YES, CUEBANNER=Six digits")
	app.pin.SetCallback("MASKFAIL_CB", iup.MaskFailFunc(func(_ iup.Ihandle, _ string) int {
		app.log("Recovery PIN accepts six digits only")
		return iup.DEFAULT
	}))
	app.security = app.page("Security", app.pin, app.form("Choose six digits to protect your encrypted backup.", []string{"PIN"}, []iup.Ihandle{app.pin}))
	app.track(app.pin, "Recovery PIN")
}

func (app *wizard) track(field iup.Ihandle, name string) {
	field.SetCallback("GETFOCUS_CB", iup.GetFocusFunc(func(iup.Ihandle) int {
		app.log("Focused " + name)
		return iup.DEFAULT
	}))
	field.SetCallback("KILLFOCUS_CB", iup.KillFocusFunc(func(ih iup.Ihandle) int {
		app.log("Left " + name)
		if !app.closing && app.current != app.finish.panel {
			app.validateField(ih, false)
		}
		return iup.DEFAULT
	}))
}

func (app *wizard) validateField(field iup.Ihandle, focus bool) bool {
	message := ""
	switch field {
	case app.name:
		if len(strings.TrimSpace(field.GetAttribute("VALUE"))) < 2 {
			message = "Enter at least two characters for the name"
		}
	case app.email:
		value := strings.TrimSpace(field.GetAttribute("VALUE"))
		address, err := mail.ParseAddress(value)
		if err != nil || address.Address != value {
			message = "Enter a valid email address"
		}
	case app.username:
		if len(strings.TrimSpace(field.GetAttribute("VALUE"))) < 3 {
			message = "User name needs at least three characters"
		}
	case app.password:
		if len(field.GetAttribute("VALUE")) < 8 {
			message = "Password needs at least eight characters"
		}
	case app.confirm:
		if field.GetAttribute("VALUE") != app.password.GetAttribute("VALUE") {
			message = "Passwords do not match"
		}
	case app.pin:
		if field != 0 && len(field.GetAttribute("VALUE")) != 6 {
			message = "Enter exactly six digits"
		}
	case app.consent:
		if !field.GetBool("VALUE") {
			message = "Confirm the review before installing"
		}
	}
	if message == "" {
		return true
	}
	if !focus {
		app.log(message)
		return false
	}
	iup.SetFocus(field)
	app.errorAt(field, message)
	return false
}

func (app *wizard) errorAt(field iup.Ihandle, message string) {
	app.hideError()
	app.log(message)
	if field == 0 || field.GetAttribute("WID") == "" {
		return
	}
	app.popover = iup.Popover(iup.Label(message).SetAttributes("PADDING=8x6"))
	app.popover.SetAttributes("POSITION=BOTTOMLEFT, AUTOHIDE=YES")
	app.popover.SetAttributeHandle("ANCHOR", field)
	app.popover.SetAttribute("VISIBLE", "YES")
}

func (app *wizard) hideError() {
	if app.popover != 0 {
		app.popover.SetAttribute("VISIBLE", "NO")
		app.popover.Destroy()
		app.popover = 0
	}
}

func (app *wizard) show(item *step, focus bool) {
	app.hideError()
	app.current = item.panel
	app.pages.SetAttribute("VALUEPOS", iup.GetChildPos(app.pages, item.panel))
	if item == app.review {
		summary := fmt.Sprintf("Name: %s\nEmail: %s\nUser: %s\nMode: %s\nEncrypted backup: %s",
			app.name.GetAttribute("VALUE"), app.email.GetAttribute("VALUE"), app.username.GetAttribute("VALUE"),
			map[bool]string{true: "Custom", false: "Standard"}[app.custom != nil], map[bool]string{true: "Yes", false: "No"}[app.security != nil])
		if app.custom != nil {
			summary += "\nInstallation date: " + app.date.GetAttribute("VALUE")
			if app.file.GetAttribute("VALUE") != "" {
				summary += "\nConfiguration file: " + app.file.GetAttribute("VALUE")
			}
		}
		iup.GetHandle("wizard_summary").SetAttribute("VALUE", summary)
	}
	app.updateNavigation()
	if focus && item.first != 0 && !mobile() {
		iup.SetFocus(item.first)
	}
}

func (app *wizard) updateNavigation() {
	steps := app.steps()
	for i, item := range steps {
		if item.panel != app.current {
			continue
		}
		app.heading.SetAttribute("TITLE", fmt.Sprintf("Step %d of %d: %s", i+1, len(steps), item.title))
		app.back.SetAttribute("ACTIVE", yesNo(i > 0 && !app.working && !app.finished))
		app.next.SetAttribute("ACTIVE", yesNo(!app.working))
		switch {
		case app.finished:
			app.next.SetAttribute("TITLE", "Cl&ose")
			app.next.SetAttribute("VISIBLE", yesNo(runtime.GOOS != "ios"))
		case item == app.review:
			app.next.SetAttribute("TITLE", "&Install")
		default:
			app.next.SetAttribute("TITLE", "&Next")
		}
		break
	}
}

func yesNo(value bool) string {
	if value {
		return "YES"
	}
	return "NO"
}

func (app *wizard) advance() {
	if app.finished {
		app.abort()
		return
	}
	if app.working {
		return
	}
	steps := app.steps()
	for i, item := range steps {
		if item.panel != app.current {
			continue
		}
		var fields []iup.Ihandle
		switch item {
		case app.welcome:
			fields = []iup.Ihandle{app.name, app.email}
		case app.account:
			fields = []iup.Ihandle{app.username, app.password, app.confirm}
		case app.security:
			fields = []iup.Ihandle{app.pin}
		case app.review:
			fields = []iup.Ihandle{app.consent}
		}
		for _, field := range fields {
			if !app.validateField(field, true) {
				return
			}
		}
		if item == app.review {
			app.show(app.finish, false)
			app.startJob()
			return
		}
		if i+1 < len(steps) {
			app.show(steps[i+1], true)
		}
		return
	}
}

func (app *wizard) previous() {
	if app.working {
		return
	}
	steps := app.steps()
	for i, item := range steps {
		if item.panel == app.current && i > 0 {
			app.show(steps[i-1], true)
			return
		}
	}
}

func (app *wizard) startJob() {
	app.working = true
	app.jobSerial++
	serial := app.jobSerial
	ctx, cancel := context.WithCancel(context.Background())
	app.jobCancel = cancel
	app.pending++
	app.progress.SetAttribute("VALUE", 0)
	app.progressText.SetAttribute("TITLE", "Preparing...")
	app.updateNavigation()
	app.log("Started asynchronous installation")
	go func() {
		defer iup.PostMessage(app.dialog, "complete", serial, nil)
		for value := 20; value <= 100; value += 20 {
			select {
			case <-ctx.Done():
				return
			case <-time.After(350 * time.Millisecond):
				iup.PostMessage(app.dialog, "progress", serial, value)
			}
		}
	}()
}

func (app *wizard) abort() {
	if app.working {
		app.jobCancel()
		app.working = false
		app.progress.SetAttribute("VALUE", 0)
		app.show(app.review, true)
		app.log("Installation canceled")
		return
	}
	if app.pending > 0 {
		app.closing = true
		if app.jobCancel != nil {
			app.jobCancel()
		}
		return
	}
	app.end()
}

func (app *wizard) end() {
	app.closing = true
	app.hideError()
	iup.Hide(app.dialog)
	iup.ExitLoop()
}

func (app *wizard) log(message string) {
	if app.eventLog != 0 {
		app.eventLog.SetAttribute("APPEND", fmt.Sprintf("[%s] %s", time.Now().Format("15:04:05"), message))
	}
}

func button(title, accessible string, action func()) iup.Ihandle {
	item := iup.Button(title).SetAttribute("ACCESSIBLETITLE", accessible).SetAttribute("PADDING", "5x3")
	item.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return item
}
