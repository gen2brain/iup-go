//go:build ctrl

package main

import (
	"fmt"
	"math"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

type ingredient struct {
	amount float64
	unit   string
	name   string
}

type recipe struct {
	title, category, description, time, accent string
	servings                                   int
	ingredients                                []ingredient
	steps                                      []string
}

type palette struct {
	name, appearance                   string
	dlgBg, dlgFg, txtBg, txtFg, accent string
	heroBg, plate, inner, garnish      string
}

var palettes = []palette{
	{"Ocean", "LIGHT", "224 238 243", "30 65 80", "245 251 251", "27 63 77", "33 126 157", "206 231 236", "250 253 250", "222 240 234", "61 153 148"},
	{"Meadow", "LIGHT", "232 239 217", "49 72 48", "250 252 240", "41 67 43", "89 135 73", "222 233 199", "253 251 237", "232 220 174", "86 145 88"},
	{"Midnight", "DARK", "30 41 60", "224 232 243", "39 53 76", "230 238 248", "120 184 209", "25 34 54", "85 107 132", "50 67 91", "134 201 174"},
}

type scheme struct {
	dlgBg, dlgFg, txtBg, txtFg, cardBg, cardFg string
	buttonBg, hl, ps, textPs, border, accent   string
	focus                                      string
}

type cookbook struct {
	recipes                                 []recipe
	dialog, tree, search, hero, title, meta iup.Ihandle
	ingredients, method, shopping           iup.Ihandle
	status, servings, addButton, newItem    iup.Ihandle
	themeButton, actions, info              iup.Ihandle
	popups                                  []iup.Ihandle
	selected, portions, themeIndex          int
	nodes                                   map[int]int
	updating, applyingTheme, empty          bool
}

var categories = []string{"Breakfast", "Lunch", "Dinner", "Something sweet"}

func main() {
	iup.Open()
	defer iup.Close()
	iup.ControlsOpen()
	iup.SetGlobal("UTF8MODE", "YES")

	app := &cookbook{selected: 0, portions: 2, recipes: []recipe{
		{"Citrus ricotta pancakes", "Breakfast", "Fluffy, bright and made for a slow morning.", "25 min", "238 167 79", 2,
			[]ingredient{{1, "cup", "flour"}, {0.75, "cup", "ricotta"}, {2, "", "eggs"}, {1, "", "orange"}, {1, "tbsp", "honey"}},
			[]string{"Whisk flour, ricotta and eggs into a soft batter.", "Grate in the orange zest and fold gently.", "Cook small pancakes until golden on both sides.", "Serve with orange slices and honey."}},
		{"Garden toast", "Breakfast", "Crunchy sourdough with herbs and fresh greens.", "15 min", "102 153 103", 2,
			[]ingredient{{2, "slices", "sourdough"}, {1, "", "avocado"}, {0.5, "cup", "peas"}, {1, "tbsp", "lemon juice"}, {1, "handful", "fresh herbs"}},
			[]string{"Toast the sourdough until crisp.", "Mash avocado and peas with lemon juice.", "Pile onto toast and finish with herbs."}},
		{"Rainbow grain bowl", "Lunch", "A colorful bowl with a creamy tahini finish.", "30 min", "219 139 105", 2,
			[]ingredient{{1, "cup", "cooked quinoa"}, {1, "", "carrot"}, {0.5, "", "cucumber"}, {0.5, "cup", "chickpeas"}, {2, "tbsp", "tahini"}},
			[]string{"Cook and cool the quinoa.", "Slice the vegetables and rinse the chickpeas.", "Arrange in bowls and drizzle with loosened tahini."}},
		{"Tomato & basil soup", "Lunch", "A little comfort in a bowl.", "35 min", "207 104 84", 4,
			[]ingredient{{6, "", "tomatoes"}, {1, "", "onion"}, {2, "cloves", "garlic"}, {2, "cups", "vegetable stock"}, {1, "handful", "basil"}},
			[]string{"Soften onion and garlic in a pot.", "Add tomatoes and stock; simmer until tender.", "Blend smooth and finish with torn basil."}},
		{"Lemon herb pasta", "Dinner", "Silky, lemony pasta for any night of the week.", "20 min", "190 171 89", 2,
			[]ingredient{{200, "g", "pasta"}, {1, "", "lemon"}, {2, "tbsp", "olive oil"}, {1, "handful", "parsley"}, {30, "g", "parmesan"}},
			[]string{"Cook pasta, reserving a cup of cooking water.", "Toss with lemon zest, oil and a splash of water.", "Finish with parsley and grated parmesan."}},
		{"Roasted pepper traybake", "Dinner", "A hands-off supper with caramelized edges.", "45 min", "205 119 88", 4,
			[]ingredient{{3, "", "bell peppers"}, {2, "", "red onions"}, {1, "cup", "chickpeas"}, {2, "tbsp", "olive oil"}, {1, "tsp", "smoked paprika"}},
			[]string{"Heat the oven to 210 C.", "Toss everything with oil and paprika.", "Roast until the vegetables are soft and browned."}},
		{"Berry yogurt cups", "Something sweet", "Layers of berries, yogurt and crunch.", "10 min", "172 105 152", 2,
			[]ingredient{{1, "cup", "yogurt"}, {1, "cup", "mixed berries"}, {0.5, "cup", "granola"}, {1, "tbsp", "honey"}},
			[]string{"Spoon yogurt into two glasses.", "Layer with berries and granola.", "Drizzle with honey just before serving."}},
		{"Chocolate orange pots", "Something sweet", "Rich chocolate with a hint of orange.", "20 min", "139 94 80", 4,
			[]ingredient{{150, "g", "dark chocolate"}, {1, "cup", "cream"}, {1, "", "orange"}, {1, "pinch", "sea salt"}},
			[]string{"Warm the cream and pour over chopped chocolate.", "Stir until smooth; add orange zest and salt.", "Spoon into cups and chill until set."}},
	}}
	app.build()
	app.applyTheme()
	iup.Show(app.dialog)
	for _, name := range []string{"1 cup cooked quinoa", "2 lemons", "1 handful fresh herbs"} {
		app.shopping.SetAttribute("APPENDITEM", name)
	}
	app.shopping.SetAttribute("IMAGEVALUE2", "YES")
	app.shopping.SetAttribute("REDRAW", "L2")
	app.updateStatus()
	iup.MainLoop()
}

func (app *cookbook) build() {
	app.tree = iup.FlatTree().SetAttributes("EXPAND=YES, VISIBLECOLUMNS=16, SPACING=5, SHOWRENAME=YES, SHOWDRAGDROP=YES, ICONSPACING=6")
	app.tree.SetCallback("SELECTION_CB", iup.SelectionFunc(func(_ iup.Ihandle, id, state int) int {
		if state == 1 && !app.updating {
			if index, ok := app.nodes[id]; ok {
				app.selected = index
				app.portions = app.recipes[index].servings
				app.showRecipe()
			}
		}
		return iup.DEFAULT
	}))
	app.tree.SetCallback("SHOWRENAME_CB", iup.ShowRenameFunc(func(_ iup.Ihandle, id int) int {
		if _, ok := app.nodes[id]; !ok {
			return iup.IGNORE
		}
		return iup.DEFAULT
	}))
	app.tree.SetCallback("RENAME_CB", iup.RenameFunc(func(_ iup.Ihandle, id int, name string) int {
		index, ok := app.nodes[id]
		name = strings.TrimSpace(name)
		if !ok || name == "" {
			return iup.IGNORE
		}
		for i, r := range app.recipes {
			if i != index && r.category == app.recipes[index].category && strings.EqualFold(r.title, name) {
				return iup.IGNORE
			}
		}
		app.recipes[index].title = name
		app.showRecipe()
		return iup.DEFAULT
	}))
	app.tree.SetCallback("DRAGDROP_CB", iup.DragDropFunc(func(_ iup.Ihandle, from, to, _, _ int) int {
		index, ok := app.nodes[from]
		if !ok || to < 0 {
			return iup.DEFAULT
		}
		category := ""
		if other, found := app.nodes[to]; found {
			category = app.recipes[other].category
		} else {
			category = app.tree.GetAttribute("TITLE", to)
		}
		if category != "" && category != app.recipes[index].category {
			app.recipes[index].category = category
			app.selected = index
			app.rebuildTree()
		}
		return iup.DEFAULT
	}))

	app.search = iup.Text().SetAttribute("EXPAND", "HORIZONTAL").SetAttribute("CUEBANNER", "Search recipes")
	app.search.SetCallback("VALUECHANGED_CB", iup.ValueChangedFunc(func(iup.Ihandle) int {
		app.rebuildTree()
		return iup.DEFAULT
	}))

	app.hero = iup.Canvas().SetAttributes("EXPAND=YES, BORDER=NO")
	app.hero.SetCallback("ACTION", iup.ActionFunc(app.drawHero))
	app.title = iup.FlatLabel("").SetAttributes("FONTSTYLE=Bold, FONTSIZE=18, EXPAND=HORIZONTAL, TEXTELLIPSIS=YES")
	app.meta = iup.FlatLabel("").SetAttributes("EXPAND=HORIZONTAL, TEXTELLIPSIS=YES")
	app.ingredients = iup.FlatLabel("").SetAttributes("EXPAND=YES, ALIGNMENT=ALEFT:ATOP, PADDING=10x8")
	app.method = iup.FlatLabel("").SetAttributes("EXPAND=YES, ALIGNMENT=ALEFT:ATOP, PADDING=10x8")

	choices := iup.Vbox().SetAttributes("NMARGIN=4x4, NGAP=0")
	for _, n := range []int{1, 2, 4, 6} {
		servings := n
		iup.Append(choices, menuItem(servingsText(n), func() {
			app.servings.SetAttribute("SHOWDROPDOWN", "NO")
			app.portions = servings
			app.showRecipe()
		}))
	}
	app.servings = iup.DropButton(choices).SetAttributes("DROPONARROW=NO, SHOWBORDER=YES, PADDING=8x4")
	app.addButton = button("Add ingredients", app.addIngredients)

	app.shopping = iup.MatrixList().SetAttributes("COLUMNORDER=IMAGE:LABEL, EXPAND=YES, VISIBLELINES=10")
	app.shopping.SetCallback("RESIZEMATRIX_CB", iup.ResizeMatrixFunc(func(ih iup.Ihandle, _, _ int) int {
		ih.SetAttributeId("RASTERWIDTH", 2, nil)
		ih.SetAttribute("FITTOSIZE", "COLUMNS")
		return iup.DEFAULT
	}))
	app.newItem = iup.Text().SetAttributes("VISIBLECOLUMNS=18, EXPAND=HORIZONTAL").SetAttribute("CUEBANNER", "Add an item")
	addItem := func() {
		name := strings.TrimSpace(app.newItem.GetAttribute("VALUE"))
		if name != "" {
			app.shopping.SetAttribute("APPENDITEM", name)
			app.newItem.SetAttribute("VALUE", "")
			app.updateStatus()
		}
	}
	app.newItem.SetCallback("K_ANY", iup.KAnyFunc(func(_ iup.Ihandle, key int) int {
		if key == iup.K_CR {
			addItem()
			return iup.IGNORE
		}
		return iup.CONTINUE
	}))
	app.status = iup.FlatLabel("").SetAttributes("EXPAND=HORIZONTAL, TEXTELLIPSIS=YES")

	left := iup.Vbox(iup.FlatLabel("THE RECIPE BOOK").SetAttributes("FONTSTYLE=Bold, PADDING=0x5"), app.search, app.tree).
		SetAttributes("NGAP=8, NMARGIN=12x12, EXPAND=VERTICAL")
	app.info = iup.Hbox(
		iup.Vbox(iup.FlatLabel("INGREDIENTS").SetAttribute("FONTSTYLE", "Bold"), iup.BackgroundBox(app.ingredients)).SetAttributes("NGAP=5, EXPAND=YES"),
		iup.Vbox(iup.FlatLabel("METHOD").SetAttribute("FONTSTYLE", "Bold"), iup.BackgroundBox(app.method)).SetAttributes("NGAP=5, EXPAND=YES"),
	).SetAttributes("NGAP=14, EXPAND=HORIZONTAL")
	app.actions = iup.Hbox(app.servings, app.addButton).SetAttributes("NGAP=7")
	center := iup.Vbox(app.hero, app.title, app.meta, app.actions, app.info).
		SetAttributes("NGAP=10, NMARGIN=12x12, EXPAND=YES")
	right := iup.Vbox(
		iup.FlatLabel("SHOPPING LIST").SetAttributes("FONTSTYLE=Bold, PADDING=0x5"),
		iup.FlatLabel("Tick off items as you shop."),
		app.shopping,
		iup.Hbox(app.newItem, button("Add", addItem)).SetAttributes("NGAP=5, ALIGNMENT=ACENTER"),
		button("Remove selected", func() {
			id := app.shopping.GetInt("FOCUSITEM")
			if id > 0 && id <= app.shopping.GetInt("COUNT") {
				app.shopping.SetAttribute("DELLIN", id)
				app.updateStatus()
			}
		}),
	).SetAttributes("NGAP=8, NMARGIN=12x12, EXPAND=VERTICAL")

	var content iup.Ihandle
	if phone() {
		left.SetAttribute("TABTITLE", "Browse")
		center.SetAttribute("TABTITLE", "Recipe")
		right.SetAttribute("TABTITLE", "Shopping")
		content = iup.FlatTabs(left, center, right).SetAttribute("EXPAND", "YES")
	} else {
		content = iup.Hbox(left, iup.Separator(), center, iup.Separator(), right).SetAttribute("EXPAND", "YES")
	}
	themeChoices := iup.Vbox().SetAttributes("NMARGIN=4x4, NGAP=0")
	for index, name := range []string{"System", "Light", "Dark", "Ocean", "Meadow", "Midnight"} {
		choice := index
		iup.Append(themeChoices, menuItem(name, func() {
			app.themeButton.SetAttribute("SHOWDROPDOWN", "NO")
			app.selectTheme(choice)
		}))
	}
	app.themeButton = iup.DropButton(themeChoices).SetAttributes("DROPONARROW=NO, SHOWBORDER=YES, PADDING=8x4, DROPPOSITION=TOPRIGHT")
	app.themeButton.SetAttribute("TITLE", "Theme: System")
	app.popups = []iup.Ihandle{choices, themeChoices}
	footer := iup.Hbox(app.status, app.themeButton).SetAttributes("NMARGIN=12x6, ALIGNMENT=ACENTER")
	dlg := iup.Dialog(iup.Vbox(content, footer).SetAttributes("NGAP=0")).SetAttributes(map[string]string{"TITLE": "Recipe Book", "PLACEMENT": "MAXIMIZED"})
	app.dialog = dlg
	dlg.SetCallback("THEMECHANGED_CB", iup.ThemeChangedFunc(func(iup.Ihandle, int) int {
		if !app.applyingTheme && app.themeIndex < 3 {
			app.applyTheme()
		}
		return iup.DEFAULT
	}))
	extraWidth, _ := iup.DrawGetTextSize(app.tree, fmt.Sprintf("%d", len(app.recipes)))
	for _, r := range app.recipes {
		width, _ := iup.DrawGetTextSize(app.tree, r.time)
		extraWidth = max(extraWidth, width)
	}
	app.tree.SetAttribute("EXTRATEXTWIDTH", extraWidth+20)
	app.rebuildTree()
}

func (app *cookbook) rebuildTree() {
	app.updating = true
	defer func() { app.updating = false }()
	app.tree.SetAttribute("DELNODE", "ALL")
	query := strings.ToLower(strings.TrimSpace(app.search.GetAttribute("VALUE")))
	for c := len(categories) - 1; c >= 0; c-- {
		matches := []int{}
		for i, r := range app.recipes {
			if r.category != categories[c] {
				continue
			}
			text := r.title + " " + r.description + " " + r.category
			for _, ing := range r.ingredients {
				text += " " + ing.name
			}
			if strings.Contains(strings.ToLower(text), query) {
				matches = append(matches, i)
			}
		}
		if len(matches) == 0 {
			continue
		}
		app.tree.SetAttributeId("ADDBRANCH", -1, categories[c])
		for i := len(matches) - 1; i >= 0; i-- {
			app.tree.SetAttributeId("ADDLEAF", 0, app.recipes[matches[i]].title)
		}
	}
	app.nodes = make(map[int]int)
	category := ""
	chosen := -1
	first := -1
	for id := 0; id < app.tree.GetInt("COUNT"); id++ {
		extra := ""
		if app.tree.GetAttribute("KIND", id) == "BRANCH" {
			category = app.tree.GetAttribute("TITLE", id)
			extra = fmt.Sprintf("%d", app.tree.GetInt("CHILDCOUNT", id))
		} else {
			for i, r := range app.recipes {
				if r.category == category && r.title == app.tree.GetAttribute("TITLE", id) {
					app.nodes[id] = i
					extra = r.time
					if first < 0 {
						first = id
					}
					if i == app.selected {
						chosen = id
					}
					break
				}
			}
		}
		app.tree.SetAttributeId("EXTRATEXT", id, extra)
	}
	if chosen < 0 {
		chosen = first
	}
	app.empty = chosen < 0
	if !app.empty {
		if app.selected != app.nodes[chosen] {
			app.selected = app.nodes[chosen]
			app.portions = app.recipes[app.selected].servings
		}
		app.tree.SetAttribute("VALUE", chosen)
	}
	app.styleNodes()
	app.showRecipe()
}

func (app *cookbook) scheme() scheme {
	if app.themeIndex >= 3 {
		p := palettes[app.themeIndex-3]
		return scheme{p.dlgBg, p.dlgFg, p.txtBg, p.txtFg, p.txtBg, p.txtFg, p.heroBg, p.txtBg, p.accent, p.dlgBg, p.accent, p.accent, p.inner}
	}
	s := scheme{cardBg: iup.GetGlobal("TXTBGCOLOR"), cardFg: iup.GetGlobal("TXTFGCOLOR"),
		hl: "200 225 245", ps: "150 200 235", border: "160 160 160"}
	if iup.GetGlobalBool("DARKMODE") {
		s.hl, s.ps, s.border, s.focus = "65 75 82", "52 61 68", "95 100 105", "65 75 82"
	}
	return s
}

func setColor(ih iup.Ihandle, name, color string) {
	if color == "" {
		ih.SetAttribute(name, nil)
	} else {
		ih.SetAttribute(name, color)
	}
}

func (app *cookbook) visitTheme(ih iup.Ihandle, s scheme, card bool) {
	if ih == 0 {
		return
	}
	bg, fg := s.dlgBg, s.dlgFg
	switch iup.GetClassName(ih) {
	case "text", "matrixlist":
		bg, fg = s.txtBg, s.txtFg
	case "backgroundbox", "flattree":
		card = true
	case "flatbutton", "dropbutton":
		if ih.GetAttribute("SHOWBORDER") == "YES" {
			bg = s.buttonBg
		}
		setColor(ih, "HLCOLOR", s.hl)
		setColor(ih, "PSCOLOR", s.ps)
		setColor(ih, "BORDERCOLOR", s.border)
		setColor(ih, "TEXTHLCOLOR", fg)
		setColor(ih, "TEXTPSCOLOR", s.textPs)
	}
	if card {
		bg, fg = s.cardBg, s.cardFg
	}
	setColor(ih, "BGCOLOR", bg)
	setColor(ih, "FGCOLOR", fg)
	switch ih {
	case app.tree:
		setColor(ih, "HLCOLOR", s.accent)
	case app.shopping:
		setColor(ih, "FOCUSCOLOR", s.focus)
	}
	for i := 0; i < iup.GetChildCount(ih); i++ {
		app.visitTheme(iup.GetChild(ih, i), s, card)
	}
}

func (app *cookbook) styleNodes() {
	color := app.scheme().cardFg
	for id := 0; id < app.tree.GetInt("COUNT"); id++ {
		app.tree.SetAttributeId("COLOR", id, color)
	}
	iup.Update(app.tree)
}

func (app *cookbook) applyTheme() {
	s := app.scheme()
	app.visitTheme(app.dialog, s, false)
	for _, popup := range app.popups {
		app.visitTheme(popup, s, false)
	}
	app.styleNodes()
	iup.Redraw(app.dialog, 1)
}

func (app *cookbook) selectTheme(index int) {
	if index == app.themeIndex {
		return
	}
	app.applyingTheme = true
	defer func() { app.applyingTheme = false }()
	app.themeIndex = index
	appearance := []string{"SYSTEM", "LIGHT", "DARK"}[min(index, 2)]
	name := []string{"System", "Light", "Dark"}[min(index, 2)]
	if index >= 3 {
		appearance = palettes[index-3].appearance
		name = palettes[index-3].name
	}
	iup.SetGlobal("APPEARANCE", appearance)
	app.applyTheme()
	app.themeButton.SetAttribute("TITLE", "Theme: "+name)
	iup.Refresh(app.dialog)
}

func (app *cookbook) showRecipe() {
	visible, floating := "YES", "NO"
	if app.empty {
		visible, floating = "NO", "IGNORE"
	}
	for _, box := range []iup.Ihandle{app.actions, app.info} {
		box.SetAttribute("FLOATING", floating)
		box.SetAttribute("VISIBLE", visible)
	}
	if app.empty {
		app.title.SetAttribute("TITLE", "No recipes match your search")
		app.meta.SetAttribute("TITLE", "Try another ingredient or clear the search.")
	} else {
		r := app.recipes[app.selected]
		app.title.SetAttribute("TITLE", r.title)
		app.meta.SetAttribute("TITLE", fmt.Sprintf("%s    /    %s    /    %s", strings.ToUpper(r.category), r.time, r.description))
		app.servings.SetAttribute("TITLE", servingsText(app.portions))
		lines := make([]string, len(r.ingredients))
		for i, ing := range r.ingredients {
			lines[i] = formatIngredient(ing, float64(app.portions)/float64(r.servings))
		}
		app.ingredients.SetAttribute("TITLE", strings.Join(lines, "\n"))
		steps := make([]string, len(r.steps))
		for i, step := range r.steps {
			steps[i] = fmt.Sprintf("%d.  %s", i+1, step)
		}
		app.method.SetAttribute("TITLE", strings.Join(steps, "\n\n"))
	}
	app.updateStatus()
	iup.Update(app.hero)
	if app.dialog.GetAttribute("WID") != "" {
		iup.Refresh(app.dialog)
	}
}

func servingsText(n int) string {
	if n == 1 {
		return "1 serving"
	}
	return fmt.Sprintf("%d servings", n)
}

func formatIngredient(ing ingredient, scale float64) string {
	amount := ing.amount * scale
	quantity := fmt.Sprintf("%g", math.Round(amount*100)/100)
	if ing.unit != "" {
		quantity += " " + ing.unit
	}
	return quantity + "  " + ing.name
}

func (app *cookbook) addIngredients() {
	if app.empty {
		return
	}
	r := app.recipes[app.selected]
	for _, ing := range r.ingredients {
		app.shopping.SetAttribute("APPENDITEM", formatIngredient(ing, float64(app.portions)/float64(r.servings)))
	}
	app.updateStatus()
}

func (app *cookbook) updateStatus() {
	app.status.SetAttribute("TITLE", fmt.Sprintf("%d recipes   /   %d shopping items   /   Drag a recipe to another category, or press F2 to rename it", len(app.recipes), app.shopping.GetInt("COUNT")))
}

func button(title string, action func()) iup.Ihandle {
	return iup.FlatButton(title).SetAttributes("PADDING=8x4, SHOWBORDER=YES").SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
}

func menuItem(title string, action func()) iup.Ihandle {
	return iup.FlatButton(title).SetAttributes("PADDING=10x4, EXPAND=HORIZONTAL, ALIGNMENT=ALEFT, BORDERWIDTH=0").SetCallback("FLAT_ACTION", iup.FlatActionFunc(func(iup.Ihandle) int {
		action()
		return iup.DEFAULT
	}))
}

func phone() bool {
	system := iup.GetGlobal("SYSTEM")
	return system == "Android" || system == "iOS"
}

func (app *cookbook) drawHero(ih iup.Ihandle) int {
	iup.DrawBegin(ih)
	defer iup.DrawEnd(ih)
	iup.DrawParentBackground(ih)
	w, h := iup.DrawGetSize(ih)
	if w < 2 || h < 2 {
		return iup.DEFAULT
	}
	background, plate, inner, garnish := "244 239 230", "255 254 250", "232 218 190", "75 128 87"
	if iup.GetGlobalBool("DARKMODE") {
		background, plate, inner, garnish = "45 48 48", "100 104 101", "73 77 73", "129 180 137"
	}
	if app.themeIndex >= 3 {
		p := palettes[app.themeIndex-3]
		background, plate, inner, garnish = p.heroBg, p.plate, p.inner, p.garnish
	}
	ih.SetAttributes(map[string]string{"DRAWSTYLE": "FILL", "DRAWCOLOR": background})
	iup.DrawRoundedRectangle(ih, 0, 0, w-1, h-1, 14)
	if app.empty {
		return iup.DEFAULT
	}
	r := app.recipes[app.selected]
	minSide := min(w, h)
	radius := minSide * 37 / 100
	cx, cy := w/2, h/2
	ih.SetAttribute("DRAWCOLOR", "140 135 125 65")
	iup.DrawEllipse(ih, cx-radius+7, cy-radius+9, cx+radius+7, cy+radius+9)
	ih.SetAttribute("DRAWCOLOR", plate)
	iup.DrawEllipse(ih, cx-radius, cy-radius, cx+radius, cy+radius)
	ih.SetAttribute("DRAWCOLOR", inner)
	iup.DrawEllipse(ih, cx-radius*82/100, cy-radius*82/100, cx+radius*82/100, cy+radius*82/100)
	ih.SetAttribute("DRAWCOLOR", r.accent)
	for i := 0; i < 7; i++ {
		angle := float64(i)*2*math.Pi/7 + 0.3
		x := cx + int(float64(radius)*0.42*math.Cos(angle))
		y := cy + int(float64(radius)*0.42*math.Sin(angle))
		size := max(5, radius/5+i%3*2)
		iup.DrawEllipse(ih, x-size, y-size, x+size, y+size)
	}
	ih.SetAttribute("DRAWCOLOR", garnish)
	for i := 0; i < 9; i++ {
		angle := float64(i)*2*math.Pi/9 + 0.7
		x := cx + int(float64(radius)*0.51*math.Cos(angle))
		y := cy + int(float64(radius)*0.51*math.Sin(angle))
		size := max(2, radius/24)
		iup.DrawEllipse(ih, x-size, y-size, x+size, y+size)
	}
	return iup.DEFAULT
}
