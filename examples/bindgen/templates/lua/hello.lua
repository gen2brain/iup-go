package.path = "./?.lua;" .. package.path
local iup = require("iup")

iup.IupOpen(nil, nil)

local label = iup.Label("Hello from Lua", { padding = "10x10" })
local text = iup.Text({ expand = "HORIZONTAL", value = "Type here" })
local greet = iup.Button("Greet")
local quit = iup.Button("Quit")

greet.action = function(ih)
  label.title = "Hello, " .. text.value
  iup.IupRefresh(label)
end

quit.action = function(ih)
  return iup.IUP_CLOSE
end

local dialog = iup.Dialog(
  iup.Vbox(label, text, iup.Hbox(iup.Fill(), greet, quit, { gap = "5" }), { nmargin = "10x10", gap = "5" }),
  { title = "IUP Lua" }
)

iup.IupShowXY(dialog, iup.IUP_CENTER, iup.IUP_CENTER)
iup.IupMainLoop()
iup.IupClose()
