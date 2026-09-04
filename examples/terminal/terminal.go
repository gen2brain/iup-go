package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func init() { iup.EntryPoint(main) }

func status(term iup.Ihandle) {
	state := "display mode"
	if pid := iup.GetAttribute(term, "PTYPID"); pid != "-1" {
		state = "running pid " + pid
	}
	if title := iup.GetAttribute(term, "APPTITLE"); title != "" {
		state += ", " + title
	}
	iup.GetHandle("statusbar").SetAttribute("TITLE", fmt.Sprintf("%s x %s cells, %s",
		iup.GetAttribute(term, "COLUMNS"), iup.GetAttribute(term, "LINES"), state))
}

func write(s string) {
	iup.GetHandle("terminal").SetAttribute("WRITE", s)
}

func colorsCb(iup.Ihandle) int {
	write("\r\n\033[1m16 colors\033[0m\r\n")
	for _, base := range []int{40, 100} {
		for i := 0; i < 8; i++ {
			write(fmt.Sprintf("\033[%dm    ", base+i))
		}
		write("\033[0m\r\n")
	}
	for _, base := range []int{30, 90} {
		for i := 0; i < 8; i++ {
			write(fmt.Sprintf("\033[%dm %02d ", base+i, base+i))
		}
		write("\033[0m\r\n")
	}

	write("\033[1m256 colors\033[0m\r\n")
	for i := 16; i < 232; i++ {
		write(fmt.Sprintf("\033[48;5;%dm ", i))
		if (i-15)%36 == 0 {
			write("\033[0m\r\n")
		}
	}

	write("\033[0m\033[1mbold\033[0m \033[3mitalic\033[0m \033[4munderline\033[0m " +
		"\033[9mstrike\033[0m \033[7minverse\033[0m \033[38;2;255;140;0mtruecolor\033[0m\r\n")
	return iup.DEFAULT
}

func boxesCb(iup.Ihandle) int {
	term := iup.GetHandle("terminal")
	write("\r\n┌──────────┬──────────┐\r\n")
	write("│ IupTerm  │ \033[32mrunning\033[0m  │\r\n")
	write("├──────────┼──────────┤\r\n")
	write("│ cells    │ " + fmt.Sprintf("%-8s", iup.GetAttribute(term, "CHARSIZE")) + " │\r\n")
	write("└──────────┴──────────┘\r\n")
	return iup.DEFAULT
}

func fontCb(delta int) func(iup.Ihandle) int {
	return func(iup.Ihandle) int {
		term := iup.GetHandle("terminal")
		if size := iup.GetInt(term, "FONTSIZE") + delta; size >= 6 && size <= 32 {
			term.SetAttribute("FONTSIZE", size)
		}
		status(term)
		return iup.DEFAULT
	}
}

func execCb(iup.Ihandle) int {
	term := iup.GetHandle("terminal")
	term.SetAttribute("EXEC", nil)
	if err := iup.GetAttribute(term, "PTYERROR"); err != "" {
		write("\r\n\033[31mcannot start a shell: " + err + "\033[0m\r\n")
	}
	status(term)
	return iup.DEFAULT
}

func main() {
	iup.Open()
	defer iup.Close()

	term := iup.Terminal().SetAttribute("EXPAND", "YES").SetHandle("terminal")
	term.SetAttributes("SCROLLBACKLINES=500, SCROLLONOUTPUT=YES, SCROLLONKEY=YES, TERMNAME=xterm-256color, AUTOCOPY=YES, ALLOWOSC52=YES, OPTIONASMETA=YES")
	statusbar := iup.Label("").SetAttributes("EXPAND=HORIZONTAL, PADDING=10x5").SetHandle("statusbar")

	btn := func(t, tip string, cb iup.ActionFunc) iup.Ihandle {
		return iup.Button(t).SetAttributes("PADDING=6x3, CANFOCUS=NO").
			SetAttribute("TIP", tip).SetCallback("ACTION", cb)
	}
	sep := func() iup.Ihandle {
		return iup.Label("").SetAttribute("SEPARATOR", "VERTICAL")
	}

	run := btn("Run Shell", "Start a shell on a pseudo terminal", execCb)
	if iup.GetAttribute(term, "PTYSUPPORT") != "YES" {
		run.SetAttributes("ACTIVE=NO").SetAttribute("TIP", "No pseudo terminal on this platform")
	}

	cursor := iup.List().SetAttributes("DROPDOWN=YES, VISIBLEITEMS=3, 1=BLOCK, 2=UNDERLINE, 3=BAR, VALUE=1")
	cursor.SetCallback("ACTION", iup.ListActionFunc(func(ih iup.Ihandle, text string, item, state int) int {
		if state == 1 {
			iup.GetHandle("terminal").SetAttribute("CURSORSTYLE", text)
		}
		return iup.DEFAULT
	}))

	blink := iup.Toggle("Blink").SetAttribute("CANFOCUS", "NO")
	blink.SetCallback("ACTION", iup.ToggleActionFunc(func(ih iup.Ihandle, state int) int {
		iup.GetHandle("terminal").SetAttribute("CURSORBLINK", state == 1)
		return iup.DEFAULT
	}))

	toolbar := iup.Hbox(
		run,
		btn("Kill", "Terminate the running command", func(iup.Ihandle) int {
			iup.GetHandle("terminal").SetAttribute("KILL", "YES")
			return iup.DEFAULT
		}),
		sep(),
		btn("Colors", "Write an ANSI color chart", colorsCb),
		btn("Boxes", "Write box drawing characters", boxesCb),
		sep(),
		btn("Copy", "Copy the selection", func(iup.Ihandle) int {
			iup.GetHandle("terminal").SetAttribute("COPYSELECTION", "YES")
			return iup.DEFAULT
		}),
		btn("Paste", "Paste the clipboard", func(iup.Ihandle) int {
			iup.GetHandle("terminal").SetAttribute("PASTE", "YES")
			return iup.DEFAULT
		}),
		btn("Clear", "Clear the screen", func(iup.Ihandle) int {
			iup.GetHandle("terminal").SetAttribute("CLEARSCREEN", "YES")
			return iup.DEFAULT
		}),
		sep(),
		btn("A-", "Smaller font", fontCb(-1)),
		btn("A+", "Larger font", fontCb(1)),
		iup.Label("Cursor:"), cursor, blink,
	).SetAttributes("ALIGNMENT=ACENTER, GAP=3")

	term.SetCallback("TERMSIZE_CB", iup.TerminalSizeFunc(func(ih iup.Ihandle, cols, lines int) int {
		status(ih)
		return iup.DEFAULT
	}))
	term.SetCallback("TITLE_CB", iup.TerminalTitleFunc(func(ih iup.Ihandle, title string) int {
		ih.SetAttribute("APPTITLE", title)
		status(ih)
		return iup.DEFAULT
	}))
	term.SetCallback("EXIT_CB", iup.TerminalExitFunc(func(ih iup.Ihandle, code int) int {
		write(fmt.Sprintf("\r\n\033[33m[command exited with %d]\033[0m\r\n", code))
		status(ih)
		return iup.DEFAULT
	}))
	term.SetCallback("BELL_CB", iup.TerminalBellFunc(func(ih iup.Ihandle) int {
		iup.GetHandle("statusbar").SetAttribute("TITLE", "bell")
		return iup.DEFAULT
	}))

	// without a command the terminal has nobody to echo what is typed
	term.SetCallback("INPUT_CB", iup.TerminalInputFunc(func(ih iup.Ihandle, b []byte) int {
		echo := make([]byte, 0, len(b)+8)
		for _, c := range b {
			switch c {
			case '\r':
				echo = append(echo, '\r', '\n')
			case 0x7F:
				echo = append(echo, '\b', ' ', '\b')
			default:
				echo = append(echo, c)
			}
		}
		write(string(echo))
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(toolbar, term, statusbar).SetAttributes("NMARGIN=5x5, NGAP=5"))
	dlg.SetAttribute("TITLE", "IupTerminal")

	iup.Show(dlg)

	write("\033[1;36mIupTerminal\033[0m: a VT100 screen drawn with the IUP draw API\r\n")
	write("Use the buttons above, or type here.\r\n")
	colorsCb(term)
	iup.SetFocus(term)
	status(term)

	iup.MainLoop()
}
