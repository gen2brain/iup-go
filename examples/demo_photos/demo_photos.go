package main

import (
	"fmt"
	"image"
	"image/color"
	"image/png"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type photo struct {
	id                       int
	name, path               string
	captured                 time.Time
	width, height            int
	size                     int64
	rating, score, sharpness int
	exposure, thumb          string
	pick, rejected, analyzed bool
}

type analysisResult struct {
	id, generation   int
	width, height    int
	size             int64
	score, sharpness int
	exposure         string
	pixels           []byte
	err              error
}

type previewRequest struct {
	id, serial int
	path       string
}

type previewResult struct {
	id, serial, width, height int
	pixels                    []byte
	err                       error
}

var sampleNames = []string{
	"IMG_2401_Dawn_Ridge.png",
	"IMG_2402_Lakeside.png",
	"IMG_2403_Pine_Trail.png",
	"IMG_2404_Golden_Field.png",
	"IMG_2405_Blue_Hour.png",
	"IMG_2406_Cliff_Path.png",
	"IMG_2407_Quiet_Cove.png",
	"IMG_2408_Cloudbreak.png",
	"IMG_2409_Forest_Light.png",
	"IMG_2410_River_Stone.png",
	"IMG_2411_Last_Light.png",
	"IMG_2412_Night_Camp.png",
}

var (
	photos []*photo
	shown  []*photo

	selectedID    int
	sortColumn    = 2
	sortAscending = true
	filterMode    = 1
	syncingTable  bool
	editingRating bool

	rootDir            string
	analysisGeneration int
	analysisComplete   int
	previewSerial      int
	previewName        string
	previewImage       iup.Ihandle
	previewWidth       int
	previewHeight      int
	previewScale       float64

	analysisRequests = make(chan int, 1)
	previewRequests  = make(chan previewRequest, 1)
	cancelWorkers    = make(chan struct{})
	cancelOnce       sync.Once
	workersDone      atomic.Int32
	closing          bool
)

func main() {
	iup.Open()
	iup.SetGlobal("UTF8MODE", "YES")
	iup.SetGlobal("APPID", "com.example.PhotoCull")
	iup.SetGlobal("APPNAME", "Photo Culling Studio")

	var err error
	rootDir, err = os.MkdirTemp(iup.GetGlobal("TMPDIR"), "iup-photo-cull-")
	if err == nil {
		if err = createSamples(); err != nil {
			os.RemoveAll(rootDir)
		}
	}
	if err != nil {
		iup.Message("Photo Culling Studio", "The sample photos need a writable folder: "+err.Error())
		iup.Close()
		return
	}

	table := photoTable()
	canvas := previewCanvas()
	canvas.SetCallback("POSTMESSAGE_CB", iup.PostMessageFunc(workerMessage))

	library := iup.Vbox(filterBar(), table).SetAttributes("NGAP=5, NMARGIN=7x7").SetAttribute("TABTITLE", "Library")
	preview := iup.Vbox(canvas, metadataPanel()).SetAttributes("NGAP=0").SetAttribute("TABTITLE", "Preview")
	var center iup.Ihandle
	if phone() {
		center = iup.Tabs(library, preview).SetAttribute("EXPAND", "YES")
	} else {
		center = iup.Split(library, preview).SetAttributes("ORIENTATION=VERTICAL, VALUE=560, MINMAX=360:760, SHOWGRIP=YES")
	}

	progress := iup.ProgressBar().SetAttribute("EXPAND", "HORIZONTAL").SetHandle("cull_progress")
	status := iup.Hbox(
		iup.Label("Preparing library...").SetAttribute("EXPAND", "HORIZONTAL").SetHandle("cull_status"),
		progress,
		iup.Label("0 / 12 analyzed").SetHandle("cull_count"),
	).SetAttributes("NGAP=8, NMARGIN=7x4, ALIGNMENT=ACENTER")

	dlg := iup.Dialog(iup.Vbox(toolbar(), center, status).SetAttributes("NGAP=0")).SetHandle("cull_dlg")
	dlg.SetAttributes(map[string]string{"TITLE": "Photo Culling Studio", "PLACEMENT": "MAXIMIZED", "SHRINK": "YES"})
	dlg.SetCallback("CLOSE_CB", iup.CloseFunc(func(iup.Ihandle) int {
		beginClose()
		return iup.IGNORE
	}))
	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int {
		refreshTable()
		iup.Update(canvas)
		return iup.DEFAULT
	}))
	dlg.SetCallback("K_ANY", iup.KAnyFunc(shortcut))

	refreshTable()
	selectedID = photos[0].id
	updateMetadata()
	startWorkers(canvas)
	iup.Show(dlg)
	moveSelection(0)
	requestAnalysis()
	iup.SetFocus(table)
	iup.MainLoop()

	if phone() {
		return
	}
	releasePreview()
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

func photoTable() iup.Ihandle {
	table := iup.Table().SetAttributes("NUMCOL=7, SHOWIMAGE=YES, FITIMAGE=NO, VISIBLECOLUMNS=5, VISIBLELINES=8, EXPAND=YES, SORTABLE=YES, USERRESIZE=YES, ALLOWREORDER=YES, STRETCHLAST=YES, ALTERNATECOLOR=YES, SELECTIONMODE=MULTIPLE, FOCUSRECT=NO, EDITABLE4=YES").SetHandle("cull_table")
	for col, title := range []string{"Photo", "Captured", "Dimensions", "Rating", "Flag", "Quality", "Exposure"} {
		iup.SetAttributeId(table, "TITLE", col+1, title)
	}
	table.SetAttributes("ALIGNMENT3=ARIGHT, ALIGNMENT4=ACENTER, ALIGNMENT5=ACENTER, ALIGNMENT6=ARIGHT, ALIGNMENT7=ACENTER")
	table.SetCallback("ENTERITEM_CB", iup.EnterItemFunc(func(_ iup.Ihandle, lin, _ int) int {
		if !syncingTable && lin >= 1 && lin <= len(shown) && shown[lin-1].id != selectedID {
			selectPhoto(shown[lin-1].id)
		}
		return iup.DEFAULT
	}))
	table.SetCallback("MULTISELECTION_CB", iup.MultiSelectionFunc(func(_ iup.Ihandle, _ []int, n int) int {
		setStatus(fmt.Sprintf("%d photo(s) selected", n))
		return iup.DEFAULT
	}))
	table.SetCallback("SORT_CB", iup.TableSortFunc(sortPhotos))
	table.SetCallback("EDITBEGIN_CB", iup.EditBeginFunc(func(_ iup.Ihandle, _, col int) int {
		if col != 4 {
			return iup.IGNORE
		}
		editingRating = true
		return iup.DEFAULT
	}))
	table.SetCallback("EDITEND_CB", iup.EditEndFunc(editRating))
	return table
}

func previewCanvas() iup.Ihandle {
	canvas := iup.Canvas().SetAttributes("BORDER=NO, EXPAND=YES, CANFOCUS=YES, TOUCH=YES").SetHandle("cull_canvas")
	canvas.SetCallback("ACTION", iup.ActionFunc(drawPreview))
	canvas.SetCallback("RESIZE_CB", iup.ResizeFunc(func(ih iup.Ihandle, _, _ int) int {
		iup.Update(ih)
		return iup.DEFAULT
	}))
	canvas.SetCallback("WHEEL_CB", iup.WheelFunc(func(ih iup.Ihandle, delta float64, _, _ int, _ string) int {
		if previewImage == 0 {
			return iup.DEFAULT
		}
		if previewScale == 0 {
			previewScale = fitScale(ih)
		}
		previewScale = max(0.05, min(previewScale*math.Pow(1.2, delta), 4))
		iup.Update(ih)
		return iup.DEFAULT
	}))
	canvas.SetCallback("BUTTON_CB", iup.ButtonFunc(func(_ iup.Ihandle, button, pressed, _, _ int, _ string) int {
		if button == 1 && pressed == 1 {
			iup.SetFocus(iup.GetHandle("cull_canvas"))
		}
		return iup.DEFAULT
	}))
	return canvas
}

func toolbar() iup.Ihandle {
	first := []iup.Ihandle{
		button("Previous", func() { moveSelection(-1) }),
		button("Next", func() { moveSelection(1) }),
		button("Pick", func() { applyFlag("pick") }).SetHandle("cull_pick"),
		button("Reject", func() { applyFlag("reject") }).SetHandle("cull_reject"),
		button("Clear flag", func() { applyFlag("clear") }),
	}
	ratings := []iup.Ihandle{iup.Label("Rating")}
	for rating := 0; rating <= 5; rating++ {
		r := rating
		ratings = append(ratings, button(strconv.Itoa(rating), func() { applyRating(r) }))
	}
	second := append(ratings,
		button("Fit", func() { previewScale = 0; iup.Update(iup.GetHandle("cull_canvas")) }),
		button("100%", func() { previewScale = 1; iup.Update(iup.GetHandle("cull_canvas")) }),
		button("Analyze again", requestAnalysis),
		button("Theme", toggleTheme),
	)
	if phone() {
		return iup.Vbox(
			iup.Hbox(first...).SetAttributes("NGAP=4, NMARGIN=6x4, ALIGNMENT=ACENTER"),
			iup.Hbox(second...).SetAttributes("NGAP=4, NMARGIN=6x4, ALIGNMENT=ACENTER"),
		)
	}
	return iup.Hbox(append(first, append([]iup.Ihandle{iup.Label("").SetAttribute("SEPARATOR", "VERTICAL")}, second...)...)...).
		SetAttributes("NGAP=4, NMARGIN=7x5, ALIGNMENT=ACENTER")
}

func button(title string, action func()) iup.Ihandle {
	b := iup.Button(title).SetAttributes("PADDING=5x3, CANFOCUS=NO")
	b.SetCallback("ACTION", iup.ActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
	return b
}

func filterBar() iup.Ihandle {
	filter := iup.List().SetAttributes("DROPDOWN=YES, VISIBLECOLUMNS=12").SetHandle("cull_filter")
	for i, value := range []string{"All photos", "Unrated", "Picks", "Rejected"} {
		filter.SetAttribute(strconv.Itoa(i+1), value)
	}
	filter.SetAttribute("VALUE", 1)
	filter.SetCallback("ACTION", iup.ListActionFunc(func(_ iup.Ihandle, _ string, item, state int) int {
		if state == 1 {
			filterMode = item
			refreshTable()
		}
		return iup.DEFAULT
	}))
	search := iup.Text().SetAttributes("VISIBLECOLUMNS=16, CUEBANNER=Search, EXPAND=HORIZONTAL").SetHandle("cull_search")
	search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(iup.Ihandle) int {
		refreshTable()
		return iup.DEFAULT
	}))
	return iup.Hbox(iup.Label("Show"), filter, search).SetAttributes("NGAP=6, ALIGNMENT=ACENTER")
}

func metadataPanel() iup.Ihandle {
	return iup.Vbox(
		iup.Hbox(
			iup.Label("No photo selected").SetAttributes("FONTSTYLE=Bold, EXPAND=HORIZONTAL").SetHandle("cull_name"),
			iup.Label("").SetHandle("cull_badge"),
		).SetAttributes("ALIGNMENT=ACENTER"),
		iup.Label("").SetHandle("cull_details"),
		iup.Label("").SetHandle("cull_metrics"),
	).SetAttributes("NGAP=3, NMARGIN=9x7")
}

func requestAnalysis() {
	analysisGeneration++
	analysisComplete = 0
	for _, p := range photos {
		p.analyzed = false
		p.exposure = "Waiting"
	}
	refreshTable()
	setStatus("Analyzing thumbnails and image quality...")
	iup.GetHandle("cull_progress").SetAttribute("VALUE", 0)
	select {
	case analysisRequests <- analysisGeneration:
	default:
		select {
		case <-analysisRequests:
		default:
		}
		analysisRequests <- analysisGeneration
	}
}

func startWorkers(target iup.Ihandle) {
	go analysisWorker(target)
	go previewWorker(target)
}

func analysisWorker(target iup.Ihandle) {
	defer iup.PostMessage(target, "workerDone", 0, nil)
	for {
		select {
		case <-cancelWorkers:
			return
		case generation := <-analysisRequests:
			for i := 0; i < len(photos); i++ {
				select {
				case <-cancelWorkers:
					return
				case generation = <-analysisRequests:
					i = -1
					continue
				default:
				}
				p := photos[i]
				result := analyzePhoto(p, generation)
				iup.PostMessage(target, "analysis", p.id, result)
				time.Sleep(55 * time.Millisecond)
			}
		}
	}
}

func previewWorker(target iup.Ihandle) {
	defer iup.PostMessage(target, "workerDone", 0, nil)
	for {
		select {
		case <-cancelWorkers:
			return
		case request := <-previewRequests:
			for len(previewRequests) > 0 {
				request = <-previewRequests
			}
			result := loadPreview(request)
			iup.PostMessage(target, "preview", request.id, result)
		}
	}
}

func analyzePhoto(p *photo, generation int) analysisResult {
	result := analysisResult{id: p.id, generation: generation}
	file, err := os.Open(p.path)
	if err != nil {
		result.err = err
		return result
	}
	img, _, err := image.Decode(file)
	file.Close()
	if err != nil {
		result.err = err
		return result
	}
	if info, err := os.Stat(p.path); err == nil {
		result.size = info.Size()
	}
	result.width, result.height = img.Bounds().Dx(), img.Bounds().Dy()
	mean, contrast, sharpness := imageMetrics(img)
	result.sharpness = int(sharpness)
	result.score = max(1, min(99, int(52+contrast*0.55+sharpness*1.2-math.Abs(mean-128)*0.25)))
	switch {
	case mean < 82:
		result.exposure = "Under"
	case mean > 182:
		result.exposure = "Over"
	default:
		result.exposure = "Balanced"
	}
	result.pixels = resizeRGBA(img, 72, 48)
	return result
}

func loadPreview(request previewRequest) previewResult {
	result := previewResult{id: request.id, serial: request.serial}
	file, err := os.Open(request.path)
	if err != nil {
		result.err = err
		return result
	}
	img, _, err := image.Decode(file)
	file.Close()
	if err != nil {
		result.err = err
		return result
	}
	result.width, result.height = img.Bounds().Dx(), img.Bounds().Dy()
	result.pixels = rgbaPixels(img)
	return result
}

func workerMessage(_ iup.Ihandle, kind string, _ int, payload any) int {
	switch kind {
	case "analysis":
		result := payload.(analysisResult)
		if result.generation != analysisGeneration || closing {
			return iup.DEFAULT
		}
		p := photoByID(result.id)
		if p == nil {
			return iup.DEFAULT
		}
		p.analyzed = true
		p.width, p.height, p.size = result.width, result.height, result.size
		p.score, p.sharpness, p.exposure = result.score, result.sharpness, result.exposure
		oldThumb := p.thumb
		if result.err != nil {
			p.exposure = "Error"
			p.thumb = ""
		} else {
			p.thumb = fmt.Sprintf("cull_thumb_%d_%d", result.id, result.generation)
			iup.ImageRGBA(72, 48, result.pixels).SetHandle(p.thumb)
		}
		analysisComplete++
		refreshTable()
		if oldThumb != "" && oldThumb != p.thumb {
			old := iup.GetHandle(oldThumb)
			iup.SetHandle(oldThumb, 0)
			old.Destroy()
		}
		updateAnalysisStatus()
		if p.id == selectedID {
			updateMetadata()
		}
	case "preview":
		result := payload.(previewResult)
		if result.serial != previewSerial || result.id != selectedID || closing {
			return iup.DEFAULT
		}
		releasePreview()
		if result.err != nil {
			setStatus("Cannot load preview: " + result.err.Error())
			iup.Update(iup.GetHandle("cull_canvas"))
			return iup.DEFAULT
		}
		setStatus("")
		previewName = fmt.Sprintf("cull_preview_%d_%d", result.id, result.serial)
		previewImage = iup.ImageRGBA(result.width, result.height, result.pixels).SetHandle(previewName)
		previewWidth, previewHeight = result.width, result.height
		previewScale = 0
		iup.Update(iup.GetHandle("cull_canvas"))
	case "workerDone":
		if workersDone.Add(1) == 2 && closing {
			finishClose()
		}
	}
	return iup.DEFAULT
}

func updateAnalysisStatus() {
	progress := float64(analysisComplete) / float64(len(photos))
	iup.GetHandle("cull_progress").SetAttribute("VALUE", progress)
	iup.GetHandle("cull_count").SetAttribute("TITLE", fmt.Sprintf("%d / %d analyzed", analysisComplete, len(photos)))
	if analysisComplete == len(photos) {
		setStatus(fmt.Sprintf("Analysis complete. %d picks, %d rejected.", countFlag("pick"), countFlag("reject")))
	}
}

func refreshTable() {
	table := iup.GetHandle("cull_table")
	if table == 0 {
		return
	}
	selected := selectedIDs()
	query := strings.ToLower(strings.TrimSpace(iup.GetHandle("cull_search").GetAttribute("VALUE")))
	shown = shown[:0]
	for _, p := range photos {
		if query != "" && !strings.Contains(strings.ToLower(p.name), query) {
			continue
		}
		if filterMode == 2 && p.rating != 0 || filterMode == 3 && !p.pick || filterMode == 4 && !p.rejected {
			continue
		}
		shown = append(shown, p)
	}
	sortShown()
	syncingTable = true
	table.SetAttribute("NUMLIN", len(shown))
	for i, p := range shown {
		lin := i + 1
		dimensions := "Analyzing"
		quality := "-"
		if p.analyzed {
			dimensions = fmt.Sprintf("%d x %d", p.width, p.height)
			quality = strconv.Itoa(p.score)
		}
		values := []string{displayName(p.name), p.captured.Format("Jan 02  15:04"), dimensions, strconv.Itoa(p.rating), flagName(p), quality, p.exposure}
		for col, value := range values {
			iup.SetAttributeId2(table, "", lin, col+1, value)
		}
		if p.thumb != "" {
			iup.SetAttributeId2(table, "IMAGE", lin, 1, p.thumb)
		} else {
			iup.SetAttributeId2(table, "IMAGE", lin, 1, nil)
		}
		iup.SetAttributeId2(table, "BGCOLOR", lin, 5, rowColor(p))
		iup.SetAttributeId2(table, "BGCOLOR", lin, 6, qualityColor(p))
	}
	set := map[int]bool{}
	for _, id := range selected {
		set[id] = true
	}
	marks := make([]byte, len(shown))
	focus := 0
	for i, p := range shown {
		marks[i] = '-'
		if set[p.id] {
			marks[i] = '+'
		}
		if p.id == selectedID {
			focus = i + 1
		}
	}
	if len(marks) > 0 {
		table.SetAttribute("SELECTEDLINES", string(marks))
	}
	if focus > 0 {
		table.SetAttribute("FOCUSCELL", fmt.Sprintf("%d:1", focus))
	}
	sign := "UP"
	if !sortAscending {
		sign = "DOWN"
	}
	iup.SetAttributeId(table, "SORTSIGN", sortColumn, sign)
	syncingTable = false
}

func sortPhotos(_ iup.Ihandle, col int) int {
	if sortColumn == col {
		sortAscending = !sortAscending
	} else {
		sortColumn, sortAscending = col, true
	}
	refreshTable()
	return iup.IGNORE
}

func sortShown() {
	sort.SliceStable(shown, func(i, j int) bool {
		a, b := shown[i], shown[j]
		cmp := 0
		switch sortColumn {
		case 1:
			cmp = strings.Compare(strings.ToLower(a.name), strings.ToLower(b.name))
		case 2:
			cmp = a.captured.Compare(b.captured)
		case 3:
			cmp = compare(a.width*a.height, b.width*b.height)
		case 4:
			cmp = compare(a.rating, b.rating)
		case 5:
			cmp = strings.Compare(flagName(a), flagName(b))
		case 6:
			cmp = compare(a.score, b.score)
		case 7:
			cmp = strings.Compare(a.exposure, b.exposure)
		}
		if sortAscending {
			return cmp < 0
		}
		return cmp > 0
	})
}

func compare(a, b int) int {
	if a < b {
		return -1
	}
	if a > b {
		return 1
	}
	return 0
}

func editRating(_ iup.Ihandle, lin, col int, value string, apply int) int {
	editingRating = false
	if apply == 0 || col != 4 || lin < 1 || lin > len(shown) {
		return iup.DEFAULT
	}
	rating, err := strconv.Atoi(strings.TrimSpace(value))
	if err != nil || rating < 0 || rating > 5 {
		setStatus("Rating must be a number from 0 to 5.")
		return iup.IGNORE
	}
	shown[lin-1].rating = rating
	refreshTable()
	updateMetadata()
	return iup.IGNORE
}

func selectedIDs() []int {
	table := iup.GetHandle("cull_table")
	if table == 0 {
		return nil
	}
	marks := table.GetAttribute("SELECTEDLINES")
	var ids []int
	for i, mark := range marks {
		if mark == '+' && i < len(shown) {
			ids = append(ids, shown[i].id)
		}
	}
	return ids
}

func actionPhotos() []*photo {
	ids := selectedIDs()
	if len(ids) == 0 && selectedID != 0 {
		ids = []int{selectedID}
	}
	set := map[int]bool{}
	for _, id := range ids {
		set[id] = true
	}
	var out []*photo
	for _, p := range photos {
		if set[p.id] {
			out = append(out, p)
		}
	}
	return out
}

func applyRating(rating int) {
	items := actionPhotos()
	for _, p := range items {
		p.rating = rating
	}
	refreshTable()
	updateMetadata()
	setStatus(fmt.Sprintf("Set rating %d on %d photo(s).", rating, len(items)))
}

func applyFlag(flag string) {
	items := actionPhotos()
	for _, p := range items {
		switch flag {
		case "pick":
			p.pick, p.rejected = true, false
		case "reject":
			p.pick, p.rejected = false, true
		default:
			p.pick, p.rejected = false, false
		}
	}
	refreshTable()
	updateMetadata()
	iup.Update(iup.GetHandle("cull_canvas"))
	setStatus(fmt.Sprintf("Updated %d photo(s).", len(items)))
}

func selectPhoto(id int) {
	p := photoByID(id)
	if p == nil {
		return
	}
	selectedID = id
	previewSerial++
	updateMetadata()
	setStatus("Loading preview for " + displayName(p.name) + "...")
	request := previewRequest{id: id, serial: previewSerial, path: p.path}
	select {
	case previewRequests <- request:
	default:
		select {
		case <-previewRequests:
		default:
		}
		previewRequests <- request
	}
}

func moveSelection(delta int) {
	if len(shown) == 0 {
		return
	}
	index := -1
	for i, p := range shown {
		if p.id == selectedID {
			index = i
			break
		}
	}
	if index < 0 {
		index = 0
	} else {
		index = max(0, min(index+delta, len(shown)-1))
	}
	selectPhoto(shown[index].id)
	table := iup.GetHandle("cull_table")
	syncingTable = true
	table.SetAttribute("FOCUSCELL", fmt.Sprintf("%d:1", index+1))
	table.SetAttribute("SELECTEDLINES", strings.Repeat("-", index)+"+"+strings.Repeat("-", len(shown)-index-1))
	table.SetAttribute("SHOW", fmt.Sprintf("%d:1", index+1))
	syncingTable = false
}

func updateMetadata() {
	p := photoByID(selectedID)
	if p == nil || iup.GetHandle("cull_name") == 0 {
		return
	}
	iup.GetHandle("cull_name").SetAttribute("TITLE", displayName(p.name))
	iup.GetHandle("cull_badge").SetAttribute("TITLE", fmt.Sprintf("%s   Rating %d / 5", flagName(p), p.rating))
	details := p.captured.Format("Monday, Jan 2 2006 at 15:04")
	metrics := "Analysis pending"
	if p.analyzed {
		details = fmt.Sprintf("%s   |   %d x %d   |   %s", details, p.width, p.height, fileSize(p.size))
		metrics = fmt.Sprintf("Quality %d   |   Sharpness %d   |   %s exposure", p.score, p.sharpness, p.exposure)
	}
	iup.GetHandle("cull_details").SetAttribute("TITLE", details)
	iup.GetHandle("cull_metrics").SetAttribute("TITLE", metrics)
}

func photoByID(id int) *photo {
	for _, p := range photos {
		if p.id == id {
			return p
		}
	}
	return nil
}

func flagName(p *photo) string {
	if p.rejected {
		return "Rejected"
	}
	if p.pick {
		return "Pick"
	}
	return "Unflagged"
}

func rowColor(p *photo) any {
	dark := iup.GetGlobalBool("DARKMODE")
	switch {
	case p.pick && dark:
		return "#294031"
	case p.pick:
		return "#DDF3E4"
	case p.rejected && dark:
		return "#492E31"
	case p.rejected:
		return "#F8DDDF"
	}
	return nil
}

func qualityColor(p *photo) any {
	if !p.analyzed {
		return nil
	}
	dark := iup.GetGlobalBool("DARKMODE")
	switch {
	case p.score >= 75 && dark:
		return "#294031"
	case p.score >= 75:
		return "#DDF3E4"
	case p.score < 55 && dark:
		return "#493B28"
	case p.score < 55:
		return "#F9E9C8"
	}
	return nil
}

func countFlag(flag string) int {
	count := 0
	for _, p := range photos {
		if flag == "pick" && p.pick || flag == "reject" && p.rejected {
			count++
		}
	}
	return count
}

func drawPreview(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	w, h := iup.DrawGetSize(ih)
	ih.SetAttribute("DRAWCOLOR", global("DLGBGCOLOR", "35 38 42"))
	ih.SetAttribute("DRAWSTYLE", "FILL")
	iup.DrawRectangle(ih, 0, 0, w-1, h-1)
	if previewImage == 0 {
		ih.SetAttributes(map[string]string{"DRAWCOLOR": "128 128 128", "DRAWFONT": global("DEFAULTFONTFACE", "Sans") + ", Bold 16", "DRAWTEXTALIGNMENT": "ACENTER"})
		_, th := iup.DrawGetTextSize(ih, "Loading preview...")
		iup.DrawText(ih, "Loading preview...", 0, (h-th)/2, w, th)
		iup.DrawEnd(ih)
		return iup.DEFAULT
	}
	scale := previewScale
	if scale == 0 {
		scale = fitScale(ih)
	}
	dw, dh := int(float64(previewWidth)*scale), int(float64(previewHeight)*scale)
	x, y := (w-dw)/2, (h-dh)/2
	ih.SetAttribute("DRAWIMAGEQUALITY", "LINEAR")
	iup.DrawImage(ih, previewName, x, y, dw, dh)
	p := photoByID(selectedID)
	if p != nil && (p.pick || p.rejected) {
		label, badge := "PICK", "38 154 83"
		if p.rejected {
			label, badge = "REJECTED", "197 67 72"
		}
		ih.SetAttribute("DRAWFONT", global("DEFAULTFONTFACE", "Sans")+", Bold 11")
		tw, th := iup.DrawGetTextSize(ih, label)
		bw, bh := tw+24, th+12
		ih.SetAttributes(map[string]string{"DRAWCOLOR": badge, "DRAWSTYLE": "FILL"})
		iup.DrawRectangle(ih, x+12, y+12, x+12+bw, y+12+bh)
		ih.SetAttributes(map[string]string{"DRAWCOLOR": "255 255 255", "DRAWTEXTALIGNMENT": "ACENTER"})
		iup.DrawText(ih, label, x+12, y+18, bw, th)
	}
	iup.DrawEnd(ih)
	return iup.DEFAULT
}

func fitScale(ih iup.Ihandle) float64 {
	w, h := iup.DrawGetSize(ih)
	if previewWidth == 0 || previewHeight == 0 || w <= 36 || h <= 36 {
		return 1
	}
	return min(float64(w-36)/float64(previewWidth), float64(h-36)/float64(previewHeight))
}

func releasePreview() {
	if previewImage != 0 {
		iup.SetHandle(previewName, 0)
		previewImage.Destroy()
	}
	previewImage, previewName = 0, ""
	previewWidth, previewHeight = 0, 0
}

func shortcut(_ iup.Ihandle, key int) int {
	if editingRating || iup.GetClassName(iup.GetFocus()) == "text" {
		return iup.CONTINUE
	}
	switch key {
	case iup.K_LEFT:
		moveSelection(-1)
	case iup.K_RIGHT:
		moveSelection(1)
	case iup.K_DEL:
		applyFlag("reject")
	case '0', '1', '2', '3', '4', '5':
		applyRating(key - '0')
	default:
		return iup.CONTINUE
	}
	return iup.IGNORE
}

func toggleTheme() {
	if iup.GetGlobalBool("DARKMODE") {
		iup.SetGlobal("APPEARANCE", "LIGHT")
	} else {
		iup.SetGlobal("APPEARANCE", "DARK")
	}
}

func beginClose() {
	if closing {
		return
	}
	closing = true
	setStatus("Finishing background work...")
	cancelOnce.Do(func() { close(cancelWorkers) })
	if workersDone.Load() == 2 {
		finishClose()
	}
}

func finishClose() {
	releasePreview()
	os.RemoveAll(rootDir)
	iup.ExitLoop()
}

func setStatus(value string) {
	if status := iup.GetHandle("cull_status"); status != 0 {
		status.SetAttribute("TITLE", value)
	}
}

func global(name, fallback string) string {
	if value := iup.GetGlobal(name); value != "" {
		return value
	}
	return fallback
}

func displayName(name string) string {
	return strings.TrimSuffix(strings.ReplaceAll(name, "_", " "), filepath.Ext(name))
}

func fileSize(size int64) string {
	if size >= 1<<20 {
		return fmt.Sprintf("%.1f MB", float64(size)/(1<<20))
	}
	return fmt.Sprintf("%.0f KB", float64(size)/(1<<10))
}

func createSamples() error {
	base := time.Date(2026, time.September, 18, 6, 35, 0, 0, time.Local)
	for i, name := range sampleNames {
		path := filepath.Join(rootDir, name)
		file, err := os.Create(path)
		if err != nil {
			return err
		}
		img := sampleImage(i, 720, 480)
		err = png.Encode(file, img)
		closeErr := file.Close()
		if err != nil {
			return err
		}
		if closeErr != nil {
			return closeErr
		}
		photos = append(photos, &photo{
			id: i + 1, name: name, path: path,
			captured: base.Add(time.Duration(i*17) * time.Minute),
			exposure: "Waiting",
		})
	}
	return nil
}

func sampleImage(index, width, height int) *image.RGBA {
	palettes := [][4]color.RGBA{
		{{58, 106, 160, 255}, {246, 169, 102, 255}, {55, 84, 78, 255}, {20, 40, 45, 255}},
		{{84, 155, 196, 255}, {210, 228, 220, 255}, {52, 112, 114, 255}, {21, 59, 76, 255}},
		{{95, 139, 151, 255}, {218, 202, 154, 255}, {45, 91, 70, 255}, {24, 48, 37, 255}},
		{{92, 138, 190, 255}, {251, 190, 91, 255}, {151, 119, 53, 255}, {74, 62, 37, 255}},
		{{31, 48, 91, 255}, {139, 103, 155, 255}, {35, 55, 83, 255}, {16, 25, 48, 255}},
		{{107, 153, 181, 255}, {226, 213, 181, 255}, {100, 101, 84, 255}, {43, 55, 50, 255}},
		{{68, 137, 171, 255}, {232, 198, 153, 255}, {44, 116, 125, 255}, {26, 67, 78, 255}},
		{{68, 94, 130, 255}, {212, 173, 128, 255}, {70, 91, 92, 255}, {28, 47, 56, 255}},
		{{83, 132, 126, 255}, {224, 207, 146, 255}, {43, 95, 58, 255}, {19, 50, 31, 255}},
		{{88, 142, 174, 255}, {224, 213, 184, 255}, {83, 119, 109, 255}, {34, 65, 67, 255}},
		{{74, 91, 135, 255}, {240, 135, 83, 255}, {79, 75, 69, 255}, {31, 39, 47, 255}},
		{{14, 25, 48, 255}, {48, 63, 92, 255}, {26, 44, 47, 255}, {8, 17, 25, 255}},
	}
	p := palettes[index%len(palettes)]
	img := image.NewRGBA(image.Rect(0, 0, width, height))
	horizon := height * (50 + index%18) / 100
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			var c color.RGBA
			if y < horizon {
				c = blend(p[0], p[1], float64(y)/float64(horizon))
			} else {
				c = blend(p[2], p[3], float64(y-horizon)/float64(height-horizon))
			}
			mountain := horizon - 35 - int(105*math.Abs(float64((x+index*71)%width-width/2))/float64(width/2))
			if y > mountain && y < horizon && (index%3 != 1) {
				c = blend(p[2], p[3], 0.35)
			}
			noise := ((x*17+y*31+index*47)%17 - 8) / 2
			c.R = clampByte(int(c.R) + noise)
			c.G = clampByte(int(c.G) + noise)
			c.B = clampByte(int(c.B) + noise)
			img.SetRGBA(x, y, c)
		}
	}
	sx := width * (20 + (index*11)%60) / 100
	sy := height * (18 + index%16) / 100
	radius := 24 + index%4*7
	sun := color.RGBA{255, 224, 154, 255}
	for y := max(0, sy-radius); y < min(height, sy+radius); y++ {
		for x := max(0, sx-radius); x < min(width, sx+radius); x++ {
			if (x-sx)*(x-sx)+(y-sy)*(y-sy) <= radius*radius {
				img.SetRGBA(x, y, sun)
			}
		}
	}
	for x := 0; x < width; x++ {
		y := horizon + 20 + int(8*math.Sin(float64(x+index*30)/37))
		if y >= 0 && y < height {
			for n := 0; n < 3 && y+n < height; n++ {
				img.SetRGBA(x, y+n, blend(p[1], p[2], 0.35))
			}
		}
	}
	return img
}

func blend(a, b color.RGBA, t float64) color.RGBA {
	t = max(0, min(t, 1))
	return color.RGBA{
		R: byte(float64(a.R) + (float64(b.R)-float64(a.R))*t),
		G: byte(float64(a.G) + (float64(b.G)-float64(a.G))*t),
		B: byte(float64(a.B) + (float64(b.B)-float64(a.B))*t),
		A: 255,
	}
}

func clampByte(value int) byte {
	return byte(max(0, min(value, 255)))
}

func rgbaPixels(img image.Image) []byte {
	bounds := img.Bounds()
	pixels := make([]byte, bounds.Dx()*bounds.Dy()*4)
	for y := 0; y < bounds.Dy(); y++ {
		for x := 0; x < bounds.Dx(); x++ {
			r, g, b, a := img.At(bounds.Min.X+x, bounds.Min.Y+y).RGBA()
			i := (y*bounds.Dx() + x) * 4
			pixels[i], pixels[i+1], pixels[i+2], pixels[i+3] = byte(r>>8), byte(g>>8), byte(b>>8), byte(a>>8)
		}
	}
	return pixels
}

func resizeRGBA(img image.Image, width, height int) []byte {
	bounds := img.Bounds()
	pixels := make([]byte, width*height*4)
	for y := 0; y < height; y++ {
		sy := bounds.Min.Y + y*bounds.Dy()/height
		for x := 0; x < width; x++ {
			sx := bounds.Min.X + x*bounds.Dx()/width
			r, g, b, a := img.At(sx, sy).RGBA()
			i := (y*width + x) * 4
			pixels[i], pixels[i+1], pixels[i+2], pixels[i+3] = byte(r>>8), byte(g>>8), byte(b>>8), byte(a>>8)
		}
	}
	return pixels
}

func imageMetrics(img image.Image) (float64, float64, float64) {
	bounds := img.Bounds()
	step := max(1, min(bounds.Dx(), bounds.Dy())/120)
	var sum, sum2, edges float64
	count := 0
	for y := bounds.Min.Y; y < bounds.Max.Y; y += step {
		previous := -1.0
		for x := bounds.Min.X; x < bounds.Max.X; x += step {
			r, g, b, _ := img.At(x, y).RGBA()
			lum := 0.2126*float64(r>>8) + 0.7152*float64(g>>8) + 0.0722*float64(b>>8)
			sum += lum
			sum2 += lum * lum
			if previous >= 0 {
				edges += math.Abs(lum - previous)
			}
			previous = lum
			count++
		}
	}
	if count == 0 {
		return 0, 0, 0
	}
	mean := sum / float64(count)
	variance := max(0, sum2/float64(count)-mean*mean)
	return mean, math.Sqrt(variance), edges / float64(count)
}
