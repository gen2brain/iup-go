package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

type Counter struct {
	frame      iup.Ihandle
	valueLabel iup.Ihandle
	value      int
}

func (c *Counter) GetValue() int {
	return c.value
}

func (c *Counter) SetValue(v int) {
	c.value = v
	c.valueLabel.SetAttribute("TITLE", fmt.Sprintf("%d", v))
}

func (c *Counter) Handle() iup.Ihandle {
	return c.frame
}

func createCounter(title string, initialValue int) *Counter {
	c := &Counter{value: initialValue}

	c.valueLabel = iup.Label(fmt.Sprintf("%d", initialValue)).SetAttribute("SIZE", "50x")

	decBtn := iup.Button("-").SetAttribute("SIZE", "30x")
	incBtn := iup.Button("+").SetAttribute("SIZE", "30x")

	decBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		c.SetValue(c.value - 1)
		return iup.DEFAULT
	}))

	incBtn.SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		c.SetValue(c.value + 1)
		return iup.DEFAULT
	}))

	c.frame = iup.Frame(
		iup.Hbox(
			decBtn,
			c.valueLabel,
			incBtn,
		).SetAttributes("GAP=10, ALIGNMENT=ACENTER"),
	).SetAttribute("TITLE", title)

	return c
}

func main() {
	iup.Open()
	defer iup.Close()

	counter1 := createCounter("Counter 1", 0)
	counter2 := createCounter("Counter 2", 100)
	counter3 := createCounter("Counter 3", -50)

	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("Custom controls using Go structs."),
			iup.Label("Each counter maintains its own state."),
			iup.Space().SetAttribute("SIZE", "x10"),
			counter1.Handle(),
			counter2.Handle(),
			counter3.Handle(),
			iup.Space().SetAttribute("SIZE", "x10"),
			iup.Button("Print All Values").SetCallback("ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
				fmt.Printf("Counter 1: %d\n", counter1.GetValue())
				fmt.Printf("Counter 2: %d\n", counter2.GetValue())
				fmt.Printf("Counter 3: %d\n", counter3.GetValue())
				return iup.DEFAULT
			})),
		).SetAttributes("MARGIN=20x20, GAP=10"),
	).SetAttribute("TITLE", "User Control Example")

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}
