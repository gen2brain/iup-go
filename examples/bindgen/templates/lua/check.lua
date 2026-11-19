package.path = "./?.lua;" .. package.path
local iup = require("iup")

local failures = 0

local function check(name, ok)
  print((ok and "ok   " or "FAIL ") .. name)
  if not ok then
    failures = failures + 1
  end
end

local function raises(func)
  return not pcall(func)
end

iup.IupOpen(nil, nil)

local label = iup.Label("start")
local button = iup.Button("Button", { padding = "10x4" })
local tree = iup.Tree({ rastersize = "150x80" })
local matrix = iup.Matrix({ numlin = "3", numcol = "2", numlin_visible = "3", numcol_visible = "2" })
matrix.value_cb = function(ih, lin, col)
  return string.format("r%dc%d", lin, col)
end
local dialog = iup.Dialog(iup.Vbox(label, button, tree, matrix), { title = "check" })

iup.IupShowXY(dialog, iup.IUP_CENTER, iup.IUP_CENTER)

label.title = "changed"
check("attribute set and get", label.title == "changed")
check("constructor attribute", button.title == "Button" and button.padding == "10x4")
check("boolean attribute", button.active == true)
tree.title[0] = "root"
tree.addleaf[0] = "leaf"
check("id attribute", tree.title[0] == "root" and tree.count == "2")
check("string callback in an optional library", iup.IupGetAttributeId2(matrix, "", 2, 1) == "r2c1")
check("same object from a handle", iup.IupGetParent(label) == iup.IupGetParent(button))
check("read-only attribute", raises(function() button.charsize = "1x1" end))
check("unknown attribute", raises(function() button.titel = "x" end))
check("constants", iup.K_cA == 0x20000041 and iup.IUP_CENTER == 0xFFFF and iup.IUP_CLOSE == -3)

local ticks = {}
local timer = iup.Timer({ time = "300" })
timer.action_cb = function(ih)
  ticks[#ticks + 1] = ih
  return iup.IUP_CLOSE
end
timer.run = "YES"
iup.IupMainLoop()
check("timer callback ends the loop", #ticks == 1 and ticks[1] == timer)

iup.IupClose()
os.exit(failures == 0 and 0 or 1)
