package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type source struct {
	rel  string
	size int64
}

type treeEntry struct {
	rel  string
	dir  bool
	size int64
}

type job struct {
	id, progress      int
	rel, source, dest string
	size              int64
	state, priority   string
	failedOnce        bool
}

type update struct {
	id int
}

var sampleFiles = []source{
	{"Documents/Project Atlas/brief.pdf", 2800000},
	{"Documents/Project Atlas/roadmap.xlsx", 860000},
	{"Documents/Project Atlas/meeting-notes.md", 48000},
	{"Documents/Invoices/2026-08.pdf", 420000},
	{"Pictures/Coast/daybreak.jpg", 6400000},
	{"Pictures/Coast/harbour.jpg", 5100000},
	{"Pictures/Coast/contact-sheet.png", 2300000},
	{"Archive/locked-report.pdf", 1700000},
	{"Archive/records-2025.zip", 9800000},
}

var (
	rootDir, sourceDir, destDir string

	jobsMu sync.Mutex
	jobs   []job
	nextID = 1

	workerRunning atomic.Bool
	paused        atomic.Bool
	generation    atomic.Uint64
	exiting       atomic.Bool

	treeEntries = map[int]treeEntry{}
	treeIDs     = map[string]int{}
	treeSyncing bool

	sortColumn = 1
	sortUp     = true

	config     iup.Ihandle
	tray       iup.Ihandle
	notice     iup.Ihandle
	quitTimer  iup.Ihandle
	canUseTray bool
	quitting   bool
)

func main() {
	iup.Open()
	iup.SetGlobal("UTF8MODE", "YES")
	iup.SetGlobal("APPID", "com.example.TransferCenter")
	iup.SetGlobal("APPNAME", "Transfer Center")

	var err error
	rootDir, err = os.MkdirTemp(iup.GetGlobal("TMPDIR"), "iup-transfer-center-")
	if err == nil {
		sourceDir, destDir = filepath.Join(rootDir, "source"), filepath.Join(rootDir, "vault")
		err = createSamples()
	}
	if err != nil {
		iup.Message("Transfer Center", "The sample files need a writable folder: "+err.Error())
		iup.Close()
		return
	}

	config = iup.Config().SetAttribute("APP_NAME", "IupTransferCenterExample")
	iup.ConfigLoad(config)

	createImages()
	createInitialQueue()

	tree := sourceTree()
	table := queueTable()
	progress := iup.ProgressBar().SetAttributes("EXPAND=HORIZONTAL").SetHandle("transfer_progress")
	progress.SetAttribute("VALUE", "0")
	log := iup.Text().SetAttributes("MULTILINE=YES, READONLY=YES, WORDWRAP=YES, EXPAND=HORIZONTAL, VISIBLELINES=4").SetHandle("transfer_log")

	sourcePanel := iup.Vbox(
		iup.Hbox(
			iup.Label("Sources").SetAttributes("FONTSTYLE=Bold, EXPAND=HORIZONTAL"),
			button("All", func() { setAllSources("ON") }),
			button("None", func() { setAllSources("OFF") }),
		).SetAttributes("NGAP=4, ALIGNMENT=ACENTER"),
		tree,
		iup.Label("Three-state folders track partial selections").SetAttribute("FGCOLOR", "128 128 128"),
	).SetAttributes("NMARGIN=8x8, NGAP=6").SetAttribute("TABTITLE", "Sources")

	queuePanel := iup.Vbox(
		iup.Hbox(
			iup.Label("Transfer queue").SetAttributes("FONTSTYLE=Bold, EXPAND=HORIZONTAL"),
			iup.Label("").SetHandle("transfer_selection"),
		).SetAttributes("ALIGNMENT=ACENTER"),
		table,
	).SetAttributes("NMARGIN=8x8, NGAP=6").SetAttribute("TABTITLE", "Queue")

	var center iup.Ihandle
	if phone() {
		center = iup.Tabs(sourcePanel, queuePanel).SetAttribute("EXPAND", "YES")
	} else {
		splitValue := iup.ConfigGetVariableIntDef(config, "View", "Split", 280)
		center = iup.Split(sourcePanel, queuePanel).SetAttributes(map[string]string{
			"ORIENTATION": "VERTICAL", "VALUE": strconv.Itoa(splitValue), "MINMAX": "180:500",
		}).SetHandle("transfer_split")
		center.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(ih iup.Ihandle) int {
			iup.ConfigSetVariableInt(config, "View", "Split", ih.GetInt("VALUE"))
			return iup.DEFAULT
		}))
	}

	header := iup.Hbox(
		iup.Label("").SetAttributes("IMAGE=transfer_icon, PADDING=2x2"),
		iup.Vbox(
			iup.Label("Nightly backup").SetAttributes("FONTSTYLE=Bold, FONTSIZE=14"),
			iup.Label("Local workspace  ->  Backup vault").SetAttribute("FGCOLOR", "128 128 128"),
		).SetAttributes("NGAP=2, EXPAND=HORIZONTAL"),
		iup.Vbox(
			iup.Label("Ready").SetAttributes("FONTSTYLE=Bold, ALIGNMENT=ARIGHT").SetHandle("transfer_state"),
			iup.Label("").SetAttribute("ALIGNMENT", "ARIGHT").SetHandle("transfer_summary"),
		).SetAttributes("NGAP=2"),
	).SetAttributes("NMARGIN=10x8, NGAP=10, ALIGNMENT=ACENTER")

	footer := iup.Vbox(
		iup.Hbox(
			iup.Label("Overall").SetAttribute("FONTSTYLE", "Bold"),
			progress,
			iup.Label("0%").SetHandle("transfer_percent"),
		).SetAttributes("NGAP=8, ALIGNMENT=ACENTER"),
		iup.Hbox(
			iup.Label("").SetAttributes("EXPAND=HORIZONTAL").SetHandle("transfer_status"),
			iup.Label("Destination: Backup vault / Today"),
		).SetAttributes("NGAP=8"),
		iup.Label("Activity").SetAttribute("FONTSTYLE", "Bold"),
		log,
	).SetAttributes("NMARGIN=10x8, NGAP=5")

	dlg := iup.Dialog(iup.Vbox(header, toolbar(), center, footer).SetAttributes("NGAP=0")).SetHandle("transfer_dlg")
	dlg.SetAttribute("TITLE", "Backup and Transfer Center")
	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(closeDialog))
	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int {
		refreshTable()
		return iup.DEFAULT
	}))

	progress.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(handleWorkerUpdate))
	setupTray(dlg)
	setupQuitTimer()

	refreshTable()
	iup.ConfigDialogShow(config, dlg, "MainWindow")
	buildSourceTree()
	refreshSummary()
	appendLog("Sample workspace scanned. Seven files are queued; two archive files remain available.")
	if runtime.GOOS == "darwin" || runtime.GOOS == "ios" {
		notice = iup.Notify()
		notice.SetAttribute("REQUESTPERMISSION", "YES")
	}

	iup.MainLoop()
	if phone() {
		return
	}

	exiting.Store(true)
	generation.Add(1)
	for workerRunning.Load() {
		time.Sleep(10 * time.Millisecond)
	}
	if notice != 0 {
		notice.Destroy()
	}
	if tray != 0 {
		tray.SetAttribute("VISIBLE", "NO")
		tray.Destroy()
	}
	iup.ConfigSave(config)
	config.Destroy()
	quitTimer.Destroy()
	os.RemoveAll(rootDir)
	iup.Close()
}

func phone() bool {
	switch iup.GetGlobal("SYSTEM") {
	case "Android", "iOS":
		return true
	}
	return false
}

func toolbar() iup.Ihandle {
	first := []iup.Ihandle{
		button("Start", startTransfers).SetHandle("transfer_start"),
		button("Pause", togglePause).SetHandle("transfer_pause"),
		button("Cancel selected", cancelSelected),
		button("Retry", retrySelected),
	}
	second := []iup.Ihandle{
		button("Queue selected", queueSelectedSources),
		button("Clear finished", clearFinished),
		button("Theme", toggleTheme),
		button("Hide", hideDialog).SetHandle("transfer_hide"),
	}
	if !phone() {
		second = append(second, button("Quit", beginQuit))
	}
	if phone() {
		return iup.Vbox(
			iup.Hbox(first...).SetAttributes("NMARGIN=6x4, NGAP=4"),
			iup.Hbox(second...).SetAttributes("NMARGIN=6x4, NGAP=4"),
		)
	}
	return iup.Hbox(append(first, append([]iup.Ihandle{iup.Fill()}, second...)...)...).
		SetAttributes("NMARGIN=8x5, NGAP=5, ALIGNMENT=ACENTER")
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=6x3")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func sourceTree() iup.Ihandle {
	tree := iup.Tree().SetAttributes("ADDROOT=NO, SHOWTOGGLE=3STATE, MARKWHENTOGGLE=NO, VISIBLECOLUMNS=24, VISIBLELINES=13, EXPAND=YES").SetHandle("transfer_tree")
	tree.SetCallback("TOGGLEVALUE_CB", iup.ToggleValueFunc(func(_ iup.Ihandle, id, state int) int {
		if treeSyncing {
			return iup.DEFAULT
		}
		entry, ok := treeEntries[id]
		if !ok {
			return iup.DEFAULT
		}
		value := "OFF"
		if state == 1 {
			value = "ON"
		}
		if entry.dir {
			setSourceBranch(entry.rel, value)
		}
		updateSourceParents(entry.rel)
		refreshSourceStatus()
		return iup.DEFAULT
	}))
	return tree
}

func queueTable() iup.Ihandle {
	table := iup.Table().SetAttributes("NUMCOL=6, VISIBLELINES=13, EXPAND=YES, SORTABLE=YES, USERRESIZE=YES, ALLOWREORDER=YES, STRETCHLAST=YES, ALTERNATECOLOR=YES, SELECTIONMODE=MULTIPLE, EDITABLE=YES, FOCUSRECT=NO").SetHandle("transfer_table")
	for col, title := range []string{"Name", "Folder", "Size", "Status", "Progress", "Priority"} {
		iup.SetAttributeId(table, "TITLE", col+1, title)
	}
	table.SetAttributes("ALIGNMENT3=ARIGHT, ALIGNMENT5=ARIGHT, ALIGNMENT6=ACENTER")
	table.SetCallback("MULTISELECTION_CB", iup.MultiSelectionFunc(func(_ iup.Ihandle, ids []int, n int) int {
		iup.GetHandle("transfer_selection").SetAttribute("TITLE", fmt.Sprintf("%d selected", n))
		return iup.DEFAULT
	}))
	table.SetCallback("SORT_CB", iup.TableSortFunc(sortQueue))
	table.SetCallback("EDITBEGIN_CB", iup.EditBeginFunc(func(_ iup.Ihandle, lin, col int) int {
		if col != 6 || !editableLine(lin) {
			return iup.IGNORE
		}
		return iup.DEFAULT
	}))
	table.SetCallback("EDITEND_CB", iup.EditEndFunc(editPriority))
	return table
}

func createSamples() error {
	for i, file := range sampleFiles {
		path := filepath.Join(sourceDir, filepath.FromSlash(file.rel))
		if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
			return err
		}
		content := strings.Repeat(fmt.Sprintf("Transfer Center sample %d: %s\n", i+1, file.rel), 80+i*12)
		if err := os.WriteFile(path, []byte(content), 0o644); err != nil {
			return err
		}
	}
	return os.MkdirAll(destDir, 0o755)
}

func createInitialQueue() {
	for _, file := range sampleFiles[:7] {
		appendJob(file)
	}
}

func appendJob(file source) {
	jobs = append(jobs, job{
		id: nextID, rel: file.rel,
		source: filepath.Join(sourceDir, filepath.FromSlash(file.rel)),
		dest:   filepath.Join(destDir, filepath.FromSlash(file.rel)),
		size:   file.size, state: "Queued", priority: "Normal",
	})
	nextID++
}

func buildSourceTree() {
	tree := iup.GetHandle("transfer_tree")
	treeSyncing = true
	if tree.GetInt("COUNT") > 0 {
		tree.SetAttribute("DELNODE", "ALL")
	}
	treeEntries = map[int]treeEntry{}
	treeIDs = map[string]int{}
	items, _ := os.ReadDir(sourceDir)
	prev := -1
	for _, item := range items {
		prev = appendTreeItem(tree, -1, prev, filepath.Join(sourceDir, item.Name()), item.Name())
	}
	treeSyncing = false
	refreshSourceStatus()
}

func appendTreeItem(tree iup.Ihandle, parent, prev int, path, rel string) int {
	info, err := os.Stat(path)
	if err != nil {
		return prev
	}
	kind := "LEAF"
	if info.IsDir() {
		kind = "BRANCH"
	}
	op, ref := "ADD"+kind, parent
	if prev >= 0 {
		op, ref = "INSERT"+kind, prev
	}
	tree.SetAttribute(fmt.Sprintf("%s%d", op, ref), filepath.Base(path))
	id := tree.GetInt("LASTADDNODE")
	entry := treeEntry{rel: filepath.ToSlash(rel), dir: info.IsDir()}
	if !entry.dir {
		entry.size = logicalSize(entry.rel)
	}
	treeEntries[id] = entry
	treeIDs[entry.rel] = id
	if entry.dir {
		children, _ := os.ReadDir(path)
		childPrev := -1
		for _, child := range children {
			childRel := filepath.ToSlash(filepath.Join(rel, child.Name()))
			childPrev = appendTreeItem(tree, id, childPrev, filepath.Join(path, child.Name()), childRel)
		}
		tree.SetAttributeId("STATE", id, "EXPANDED")
	}
	tree.SetAttributeId("TOGGLEVALUE", id, "ON")
	return id
}

func logicalSize(rel string) int64 {
	for _, file := range sampleFiles {
		if file.rel == rel {
			return file.size
		}
	}
	return 0
}

func setAllSources(value string) {
	treeSyncing = true
	tree := iup.GetHandle("transfer_tree")
	for id := range treeEntries {
		tree.SetAttributeId("TOGGLEVALUE", id, value)
	}
	treeSyncing = false
	refreshSourceStatus()
}

func setSourceBranch(rel, value string) {
	treeSyncing = true
	tree := iup.GetHandle("transfer_tree")
	prefix := rel + "/"
	for id, entry := range treeEntries {
		if entry.rel == rel || strings.HasPrefix(entry.rel, prefix) {
			tree.SetAttributeId("TOGGLEVALUE", id, value)
		}
	}
	treeSyncing = false
}

func updateSourceParents(rel string) {
	treeSyncing = true
	defer func() { treeSyncing = false }()
	tree := iup.GetHandle("transfer_tree")
	for parent := filepath.ToSlash(filepath.Dir(rel)); parent != "." && parent != "/"; parent = filepath.ToSlash(filepath.Dir(parent)) {
		on, off := 0, 0
		prefix := parent + "/"
		for id, entry := range treeEntries {
			if entry.dir || !strings.HasPrefix(entry.rel, prefix) {
				continue
			}
			if iup.GetAttributeId(tree, "TOGGLEVALUE", id) == "ON" {
				on++
			} else {
				off++
			}
		}
		value := "NOTDEF"
		if off == 0 {
			value = "ON"
		} else if on == 0 {
			value = "OFF"
		}
		if id, ok := treeIDs[parent]; ok {
			tree.SetAttributeId("TOGGLEVALUE", id, value)
		}
	}
}

func selectedSources() []source {
	tree := iup.GetHandle("transfer_tree")
	var selected []source
	for id, entry := range treeEntries {
		if !entry.dir && iup.GetAttributeId(tree, "TOGGLEVALUE", id) == "ON" {
			selected = append(selected, source{entry.rel, entry.size})
		}
	}
	sort.Slice(selected, func(i, j int) bool { return selected[i].rel < selected[j].rel })
	return selected
}

func queueSelectedSources() {
	selected := selectedSources()
	jobsMu.Lock()
	existing := map[string]bool{}
	for _, item := range jobs {
		if item.state != "Canceled" {
			existing[item.rel] = true
		}
	}
	added := 0
	for _, file := range selected {
		if !existing[file.rel] {
			appendJob(file)
			existing[file.rel] = true
			added++
		}
	}
	jobsMu.Unlock()
	refreshTable()
	appendLog(fmt.Sprintf("Queued %d new file(s) from %d selected source(s).", added, len(selected)))
}

func refreshSourceStatus() {
	if iup.GetHandle("transfer_tree") == 0 {
		return
	}
	selected := selectedSources()
	var size int64
	for _, file := range selected {
		size += file.size
	}
	setStatus(fmt.Sprintf("%d source files selected, %s", len(selected), bytes(size)))
}

func startTransfers() {
	if paused.Load() {
		togglePause()
		return
	}
	if workerRunning.Load() {
		return
	}
	if !hasQueuedJobs() {
		setStatus("Nothing is queued. Select sources or retry a failed transfer.")
		return
	}
	workerRunning.Store(true)
	paused.Store(false)
	iup.GetHandle("transfer_pause").SetAttribute("TITLE", "Pause")
	dlg := iup.GetHandle("transfer_dlg")
	dlg.SetAttributes("TASKBARPROGRESS=YES, TASKBARPROGRESSSTATE=NORMAL")
	setState("Transferring")
	appendLog("Transfer started.")
	gen := generation.Load()
	go transferWorker(iup.GetHandle("transfer_progress"), gen)
}

func transferWorker(target iup.Ihandle, gen uint64) {
	defer workerRunning.Store(false)
	for {
		if exiting.Load() || generation.Load() != gen {
			return
		}
		id := nextQueuedJob()
		if id < 0 {
			post(target, "done", 0, nil)
			return
		}
		post(target, "job", id, update{id})
		failed := false
		for progress := 5; progress <= 100; progress += 5 {
			for paused.Load() && generation.Load() == gen && !exiting.Load() {
				time.Sleep(40 * time.Millisecond)
			}
			if exiting.Load() || generation.Load() != gen {
				return
			}
			time.Sleep(55 * time.Millisecond)
			jobsMu.Lock()
			item := jobByIDLocked(id)
			if item == nil || item.state != "Copying" {
				jobsMu.Unlock()
				break
			}
			if strings.HasSuffix(item.rel, "locked-report.pdf") && progress >= 65 && !item.failedOnce {
				item.state = "Error"
				item.failedOnce = true
				failed = true
			}
			if !failed {
				item.progress = progress
			}
			jobsMu.Unlock()
			post(target, "job", id, update{id})
			if failed {
				break
			}
		}
		if failed {
			post(target, "error", id, nil)
			continue
		}
		jobsMu.Lock()
		item := jobByIDLocked(id)
		if item == nil || item.state != "Copying" || item.progress < 100 {
			jobsMu.Unlock()
			continue
		}
		item.state = "Verifying"
		sourcePath, destPath := item.source, item.dest
		jobsMu.Unlock()
		post(target, "job", id, update{id})
		time.Sleep(180 * time.Millisecond)
		data, err := os.ReadFile(sourcePath)
		if err == nil {
			err = os.MkdirAll(filepath.Dir(destPath), 0o755)
		}
		if err == nil {
			err = os.WriteFile(destPath, data, 0o644)
		}
		jobsMu.Lock()
		item = jobByIDLocked(id)
		if item != nil && item.state == "Verifying" {
			if err != nil {
				item.state = "Error"
			} else {
				item.state = "Complete"
				item.progress = 100
			}
		}
		jobsMu.Unlock()
		post(target, "job", id, update{id})
	}
}

func post(target iup.Ihandle, kind string, value int, payload any) {
	if !exiting.Load() {
		iup.PostMessage(target, kind, value, payload)
	}
}

func nextQueuedJob() int {
	jobsMu.Lock()
	defer jobsMu.Unlock()
	for _, priority := range []string{"High", "Normal", "Low"} {
		for i := range jobs {
			if jobs[i].state == "Queued" && jobs[i].priority == priority {
				jobs[i].state = "Copying"
				return jobs[i].id
			}
		}
	}
	return -1
}

func handleWorkerUpdate(_ iup.Ihandle, kind string, id int, payload any) int {
	switch kind {
	case "job":
		refreshTable()
	case "error":
		setState("Needs attention")
		iup.GetHandle("transfer_dlg").SetAttribute("TASKBARPROGRESSSTATE", "ERROR")
		if item := jobByID(id); item != nil {
			appendLog(item.rel + " could not be read. Select it and press Retry.")
		}
	case "done":
		finishRun()
	}
	return iup.DEFAULT
}

func finishRun() {
	paused.Store(false)
	iup.GetHandle("transfer_pause").SetAttribute("TITLE", "Pause")
	refreshTable()
	complete, errors, canceled := stateCounts()
	if errors > 0 {
		setState("Finished with issues")
		iup.GetHandle("transfer_dlg").SetAttribute("TASKBARPROGRESSSTATE", "ERROR")
		appendLog(fmt.Sprintf("Run finished: %d complete, %d need attention.", complete, errors))
		notify("Backup needs attention", fmt.Sprintf("%d file(s) need to be retried.", errors))
	} else if canceled > 0 {
		setState("Finished")
		iup.GetHandle("transfer_dlg").SetAttributes("TASKBARPROGRESSSTATE=NOPROGRESS, TASKBARPROGRESS=NO")
		appendLog(fmt.Sprintf("Run finished: %d complete, %d canceled.", complete, canceled))
		notify("Backup finished", fmt.Sprintf("%d files were transferred; %d were canceled.", complete, canceled))
	} else {
		setState("Up to date")
		iup.GetHandle("transfer_dlg").SetAttributes("TASKBARPROGRESSSTATE=NOPROGRESS, TASKBARPROGRESS=NO")
		appendLog(fmt.Sprintf("Backup complete: %d files are up to date.", complete))
		notify("Backup complete", fmt.Sprintf("%d files were transferred to the backup vault.", complete))
	}
}

func togglePause() {
	if !workerRunning.Load() {
		setStatus("Start the queue before pausing it.")
		return
	}
	value := !paused.Load()
	paused.Store(value)
	if value {
		iup.GetHandle("transfer_pause").SetAttribute("TITLE", "Resume")
		iup.GetHandle("transfer_dlg").SetAttribute("TASKBARPROGRESSSTATE", "PAUSED")
		setState("Paused")
		appendLog("Transfer paused.")
	} else {
		iup.GetHandle("transfer_pause").SetAttribute("TITLE", "Pause")
		iup.GetHandle("transfer_dlg").SetAttribute("TASKBARPROGRESSSTATE", "NORMAL")
		setState("Transferring")
		appendLog("Transfer resumed.")
	}
}

func cancelSelected() {
	ids := selectedJobIDs()
	if len(ids) == 0 {
		setStatus("Select one or more queue rows to cancel.")
		return
	}
	jobsMu.Lock()
	for _, id := range ids {
		if item := jobByIDLocked(id); item != nil && item.state != "Complete" {
			item.state = "Canceled"
		}
	}
	jobsMu.Unlock()
	refreshTable()
	appendLog(fmt.Sprintf("Canceled %d selected transfer(s).", len(ids)))
}

func retrySelected() {
	ids := selectedJobIDs()
	jobsMu.Lock()
	retried := 0
	for _, id := range ids {
		if item := jobByIDLocked(id); item != nil && (item.state == "Error" || item.state == "Canceled") {
			item.state = "Queued"
			item.progress = 0
			retried++
		}
	}
	jobsMu.Unlock()
	refreshTable()
	appendLog(fmt.Sprintf("Returned %d transfer(s) to the queue.", retried))
}

func clearFinished() {
	jobsMu.Lock()
	kept := jobs[:0]
	removed := 0
	for _, item := range jobs {
		if item.state == "Complete" || item.state == "Canceled" {
			removed++
			continue
		}
		kept = append(kept, item)
	}
	jobs = kept
	jobsMu.Unlock()
	refreshTable()
	appendLog(fmt.Sprintf("Cleared %d finished or canceled transfer(s).", removed))
}

func hasQueuedJobs() bool {
	jobsMu.Lock()
	defer jobsMu.Unlock()
	for _, item := range jobs {
		if item.state == "Queued" {
			return true
		}
	}
	return false
}

func selectedJobIDs() []int {
	table := iup.GetHandle("transfer_table")
	selected := table.GetAttribute("SELECTEDLINES")
	jobsMu.Lock()
	defer jobsMu.Unlock()
	var ids []int
	for i, mark := range selected {
		if mark == '+' && i < len(jobs) {
			ids = append(ids, jobs[i].id)
		}
	}
	return ids
}

func refreshTable() {
	if iup.GetHandle("transfer_table") == 0 {
		return
	}
	table := iup.GetHandle("transfer_table")
	selection := table.GetAttribute("SELECTEDLINES")
	jobsMu.Lock()
	snapshot := append([]job(nil), jobs...)
	jobsMu.Unlock()
	if table.GetInt("NUMLIN") != len(snapshot) {
		table.SetAttribute("NUMLIN", len(snapshot))
	}
	for i, item := range snapshot {
		lin := i + 1
		folder := filepath.ToSlash(filepath.Dir(item.rel))
		values := []string{filepath.Base(item.rel), folder, bytes(item.size), item.state, fmt.Sprintf("%d%%", item.progress), item.priority}
		for col, value := range values {
			iup.SetAttributeId2(table, "", lin, col+1, value)
		}
		color := statusColor(item.state)
		iup.SetAttributeId2(table, "BGCOLOR", lin, 4, color)
		iup.SetAttributeId2(table, "BGCOLOR", lin, 5, color)
	}
	selectedCount := 0
	if len(selection) == len(snapshot) {
		table.SetAttribute("SELECTEDLINES", selection)
		selectedCount = strings.Count(selection, "+")
	}
	iup.GetHandle("transfer_selection").SetAttribute("TITLE", fmt.Sprintf("%d selected", selectedCount))
	iup.SetAttributeId(table, "SORTSIGN", sortColumn, map[bool]string{true: "UP", false: "DOWN"}[sortUp])
	refreshSummary()
}

func refreshSummary() {
	if iup.GetHandle("transfer_summary") == 0 {
		return
	}
	jobsMu.Lock()
	snapshot := append([]job(nil), jobs...)
	jobsMu.Unlock()
	var total, done int64
	complete, active, errors := 0, 0, 0
	for _, item := range snapshot {
		if item.state != "Canceled" {
			total += item.size
			done += item.size * int64(item.progress) / 100
		}
		switch item.state {
		case "Complete":
			complete++
		case "Copying", "Verifying":
			active++
		case "Error":
			errors++
		}
	}
	percent := 0
	if total > 0 {
		percent = int(100 * done / total)
	}
	iup.GetHandle("transfer_progress").SetAttribute("VALUE", float64(percent)/100)
	iup.GetHandle("transfer_percent").SetAttribute("TITLE", fmt.Sprintf("%d%%", percent))
	iup.GetHandle("transfer_summary").SetAttribute("TITLE", fmt.Sprintf("%d files  |  %s", len(snapshot), bytes(total)))
	iup.GetHandle("transfer_dlg").SetAttribute("TASKBARPROGRESSVALUE", percent)
	if errors > 0 {
		setStatus(fmt.Sprintf("%d complete, %d active, %d need attention", complete, active, errors))
	} else {
		setStatus(fmt.Sprintf("%d complete, %d active, %d queued", complete, active, queuedCount(snapshot)))
	}
	if tray != 0 {
		tray.SetAttribute("TIP", fmt.Sprintf("Transfer Center - %d%%", percent))
	}
}

func queuedCount(items []job) int {
	n := 0
	for _, item := range items {
		if item.state == "Queued" {
			n++
		}
	}
	return n
}

func stateCounts() (complete, errors, canceled int) {
	jobsMu.Lock()
	defer jobsMu.Unlock()
	for _, item := range jobs {
		if item.state == "Complete" {
			complete++
		}
		if item.state == "Error" {
			errors++
		}
		if item.state == "Canceled" {
			canceled++
		}
	}
	return
}

func statusColor(state string) string {
	bg := global("TXTBGCOLOR", "255 255 255")
	var accent string
	switch state {
	case "Complete":
		accent = "68 166 98"
	case "Copying", "Verifying":
		accent = global("ACCENTCOLOR", "65 120 220")
	case "Error":
		accent = "210 72 72"
	case "Canceled":
		accent = "130 130 130"
	default:
		accent = "215 154 52"
	}
	return mix(bg, accent, 0.20)
}

func sortQueue(_ iup.Ihandle, col int) int {
	selected := selectedJobIDs()
	if sortColumn == col {
		sortUp = !sortUp
	} else {
		sortColumn, sortUp = col, true
	}
	jobsMu.Lock()
	sort.SliceStable(jobs, func(i, j int) bool {
		a, b := jobs[i], jobs[j]
		var cmp int
		switch col {
		case 1:
			cmp = strings.Compare(strings.ToLower(filepath.Base(a.rel)), strings.ToLower(filepath.Base(b.rel)))
		case 2:
			cmp = strings.Compare(a.rel, b.rel)
		case 3:
			cmp = compare(a.size, b.size)
		case 4:
			cmp = strings.Compare(a.state, b.state)
		case 5:
			cmp = compare(a.progress, b.progress)
		case 6:
			cmp = compare(priorityRank(a.priority), priorityRank(b.priority))
		}
		if sortUp {
			return cmp < 0
		}
		return cmp > 0
	})
	jobsMu.Unlock()
	refreshTable()
	selectJobIDs(selected)
	return iup.IGNORE
}

func selectJobIDs(ids []int) {
	set := map[int]bool{}
	for _, id := range ids {
		set[id] = true
	}
	jobsMu.Lock()
	marks := make([]byte, len(jobs))
	for i, item := range jobs {
		marks[i] = '-'
		if set[item.id] {
			marks[i] = '+'
		}
	}
	jobsMu.Unlock()
	iup.GetHandle("transfer_table").SetAttribute("SELECTEDLINES", string(marks))
}

func priorityRank(value string) int {
	switch value {
	case "High":
		return 0
	case "Low":
		return 2
	}
	return 1
}

func compare[T ~int | ~int64](a, b T) int {
	if a < b {
		return -1
	}
	if a > b {
		return 1
	}
	return 0
}

func editableLine(lin int) bool {
	jobsMu.Lock()
	defer jobsMu.Unlock()
	if lin < 1 || lin > len(jobs) {
		return false
	}
	return jobs[lin-1].state == "Queued" || jobs[lin-1].state == "Error" || jobs[lin-1].state == "Canceled"
}

func editPriority(_ iup.Ihandle, lin, col int, value string, apply int) int {
	if apply == 0 || col != 6 {
		return iup.DEFAULT
	}
	switch strings.ToLower(strings.TrimSpace(value)) {
	case "high":
		value = "High"
	case "normal":
		value = "Normal"
	case "low":
		value = "Low"
	default:
		setStatus("Priority must be High, Normal, or Low.")
		return iup.IGNORE
	}
	jobsMu.Lock()
	if lin >= 1 && lin <= len(jobs) {
		jobs[lin-1].priority = value
	}
	jobsMu.Unlock()
	refreshTable()
	appendLog(fmt.Sprintf("Priority changed to %s.", value))
	return iup.IGNORE
}

func jobByID(id int) *job {
	jobsMu.Lock()
	defer jobsMu.Unlock()
	if item := jobByIDLocked(id); item != nil {
		copy := *item
		return &copy
	}
	return nil
}

func jobByIDLocked(id int) *job {
	for i := range jobs {
		if jobs[i].id == id {
			return &jobs[i]
		}
	}
	return nil
}

func setupTray(dlg iup.Ihandle) {
	canUseTray = !phone() && runtime.GOOS != "js"
	if !canUseTray {
		iup.GetHandle("transfer_hide").SetAttribute("ACTIVE", "NO")
		return
	}
	showItem := iup.MenuItem("Show Transfer Center").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		dlg.SetAttribute("HIDETASKBAR", "NO")
		return iup.DEFAULT
	}))
	pauseItem := iup.MenuItem("Pause or Resume").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		togglePause()
		return iup.DEFAULT
	}))
	exitItem := iup.MenuItem("Quit").SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		beginQuit()
		return iup.DEFAULT
	}))
	menu := iup.Menu(showItem, pauseItem, iup.Separator(), exitItem).SetHandle("transfer_tray_menu")
	iup.Map(menu)
	tray = iup.Tray().SetHandle("transfer_tray")
	tray.SetAttributes("IMAGE=transfer_icon, TIP=Transfer Center, MENU=transfer_tray_menu")
	tray.SetCallback("TRAYCLICK_CB", iup.TrayClickFunc(func(_ iup.Ihandle, button, pressed, double int) int {
		if button == 1 && pressed == 1 {
			dlg.SetAttribute("HIDETASKBAR", "NO")
		}
		return iup.DEFAULT
	}))
	tray.SetAttribute("VISIBLE", "YES")
}

func hideDialog() {
	if !canUseTray {
		setStatus("The system tray is not available on this platform.")
		return
	}
	iup.GetHandle("transfer_dlg").SetAttribute("HIDETASKBAR", "YES")
}

func closeDialog(iup.Ihandle) int {
	if quitting {
		return iup.IGNORE
	}
	if canUseTray {
		hideDialog()
		appendLog("Window hidden; transfers continue in the system tray.")
		return iup.IGNORE
	}
	beginQuit()
	return iup.IGNORE
}

func setupQuitTimer() {
	quitTimer = iup.Timer().SetAttribute("TIME", "50")
	quitTimer.SetCallback("ACTION_CB", iup.TimerActionFunc(func(ih iup.Ihandle) int {
		if !workerRunning.Load() {
			ih.SetAttribute("RUN", "NO")
			iup.ExitLoop()
		}
		return iup.DEFAULT
	}))
}

func beginQuit() {
	if quitting {
		return
	}
	quitting = true
	exiting.Store(true)
	generation.Add(1)
	paused.Store(false)
	iup.ConfigDialogClosed(config, iup.GetHandle("transfer_dlg"), "MainWindow")
	if tray != 0 {
		tray.SetAttribute("VISIBLE", "NO")
	}
	quitTimer.SetAttribute("RUN", "YES")
}

func notify(title, body string) {
	if exiting.Load() {
		return
	}
	if notice != 0 {
		notice.SetAttribute("CLOSE", "YES")
		notice.Destroy()
	}
	notice = iup.Notify()
	notice.SetAttributes(map[string]string{
		"TITLE": title, "BODY": body, "ICON": "transfer_icon", "ACTION1": "Show", "SILENT": "YES",
	})
	notice.SetCallback("NOTIFY_CB", iup.NotifyFunc(func(iup.Ihandle, int) int {
		iup.GetHandle("transfer_dlg").SetAttribute("HIDETASKBAR", "NO")
		return iup.DEFAULT
	}))
	notice.SetCallback("ERROR_CB", iup.ErrorFunc(func(_ iup.Ihandle, message string) int {
		appendLog("Notification unavailable: " + message)
		return iup.DEFAULT
	}))
	notice.SetAttribute("SHOW", "YES")
}

func createImages() {
	const size = 32
	pixels := make([]byte, size*size*4)
	for y := 0; y < size; y++ {
		for x := 0; x < size; x++ {
			dx, dy := x-16, y-16
			if dx*dx+dy*dy > 14*14 {
				continue
			}
			i := (y*size + x) * 4
			pixels[i], pixels[i+1], pixels[i+2], pixels[i+3] = 64, 122, 224, 255
			if (x >= 14 && x <= 18 && y >= 7 && y <= 21) || (y >= 17 && y <= 21 && x >= 10 && x <= 22) || (y-x >= 7 && y-x <= 10 && x >= 9 && x <= 16) || (x+y >= 37 && x+y <= 40 && x >= 16 && x <= 23) {
				pixels[i], pixels[i+1], pixels[i+2] = 255, 255, 255
			}
		}
	}
	iup.ImageRGBA(size, size, pixels).SetHandle("transfer_icon")
}

func toggleTheme() {
	if iup.GetGlobalBool("DARKMODE") {
		iup.SetGlobal("APPEARANCE", "LIGHT")
	} else {
		iup.SetGlobal("APPEARANCE", "DARK")
	}
}

func appendLog(message string) {
	if log := iup.GetHandle("transfer_log"); log != 0 {
		log.SetAttribute("APPEND", fmt.Sprintf("[%s] %s", time.Now().Format("15:04:05"), message))
	}
}

func setState(value string) {
	iup.GetHandle("transfer_state").SetAttribute("TITLE", value)
}

func setStatus(value string) {
	if status := iup.GetHandle("transfer_status"); status != 0 {
		status.SetAttribute("TITLE", value)
	}
}

func bytes(size int64) string {
	switch {
	case size >= 1<<30:
		return fmt.Sprintf("%.1f GB", float64(size)/(1<<30))
	case size >= 1<<20:
		return fmt.Sprintf("%.1f MB", float64(size)/(1<<20))
	case size >= 1<<10:
		return fmt.Sprintf("%.1f KB", float64(size)/(1<<10))
	}
	return fmt.Sprintf("%d B", size)
}

func global(name, fallback string) string {
	if value := iup.GetGlobal(name); value != "" {
		return value
	}
	return fallback
}

func mix(a, b string, amount float64) string {
	parse := func(value string) [3]float64 {
		fields := strings.Fields(value)
		var rgb [3]float64
		for i := range 3 {
			if i < len(fields) {
				rgb[i], _ = strconv.ParseFloat(fields[i], 64)
			}
		}
		return rgb
	}
	x, y := parse(a), parse(b)
	return fmt.Sprintf("#%02X%02X%02X", int(x[0]+(y[0]-x[0])*amount), int(x[1]+(y[1]-x[1])*amount), int(x[2]+(y[2]-x[2])*amount))
}
