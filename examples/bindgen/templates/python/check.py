import sys

import iup

failures = []


def check(name, ok):
    print("%s %s" % ("ok  " if ok else "FAIL", name))
    if not ok:
        failures.append(name)


def raises(func):
    try:
        func()
    except AttributeError:
        return True
    return False


iup.IupOpen(None, None)

label = iup.Label("start")
button = iup.Button("Button", padding="10x4")
tree = iup.Tree(rastersize="150x80")
matrix = iup.Matrix(numlin="3", numcol="2", numlin_visible="3", numcol_visible="2")
matrix.value_cb = lambda ih, lin, col: "r%dc%d" % (lin, col)
dialog = iup.Dialog(iup.Vbox(label, button, tree, matrix), title="check")

iup.IupShowXY(dialog, iup.IUP_CENTER, iup.IUP_CENTER)

label.title = "changed"
check("attribute set and get", label.title == "changed")
check("constructor attribute", button.title == "Button" and button.padding == "10x4")
check("boolean attribute", button.active is True)
tree.title[0] = "root"
tree.addleaf[0] = "leaf"
check("id attribute", tree.title[0] == "root" and tree.count == "2")
check("string callback in an optional library", iup.IupGetAttributeId2(matrix, "", 2, 1) == "r2c1")
check("same object from a handle", iup.IupGetParent(label) is iup.IupGetParent(button))
check("read-only attribute", raises(lambda: setattr(button, "charsize", "1x1")))
check("unknown attribute", raises(lambda: setattr(button, "titel", "x")))
check("constants", iup.K_cA == 0x20000041 and iup.IUP_CENTER == 0xFFFF and iup.IUP_CLOSE == -3)

ticks = []


def tick(ih):
    ticks.append(ih)
    return iup.IUP_CLOSE


timer = iup.Timer(time="300")
timer.action_cb = tick
timer.run = "YES"
iup.IupMainLoop()
check("timer callback ends the loop", len(ticks) == 1 and ticks[0] is timer)

iup.IupClose()
sys.exit(1 if failures else 0)
