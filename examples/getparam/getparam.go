package main

import (
	"fmt"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	pBoolean := 1
	pInteger := 3456
	pReal := 3.543
	pIntegerSpin := 8
	pAngle := 90.0
	pString := "string text"
	pMulti := "second\nline\ntext\nmulti\nline\nedit\ncontrol"
	pList := 2
	pOptions := 1
	pFile := "test.txt"
	pColor := "128 0 255"
	pFont := "Courier, 24"
	pDate := ""

	format := "Boolean%b[No,Yes]\n" +
		"Integer%i\n" +
		"Real%R[0,1000]\n" +
		"Spin%i[0,50,1]\n" +
		"Angle%A[0,360]\n" +
		"String%s\n" +
		"Multiline%m\n" +
		"%t\n" +
		"List%l|item1|item2|item3|item4|item5|item6|\n" +
		"Options%o|option1|option2|option3|\n" +
		"File%f[OPEN|*.txt]\n" +
		"Color%c\n" +
		"Font%n\n" +
		"Date%d\n"

	ret := iup.GetParam("Title", func(dialog iup.Ihandle, paramIndex int) int {
		switch paramIndex {
		case iup.GETPARAM_MAP:
			fmt.Println("Map")
		case iup.GETPARAM_INIT:
			fmt.Println("Init")
		case iup.GETPARAM_OK:
			fmt.Println("OK")
		case iup.GETPARAM_CANCEL:
			fmt.Println("Cancel")
		case iup.GETPARAM_HELP:
			fmt.Println("Help")
		default:
			fmt.Printf("Param %d changed\n", paramIndex)
		}
		return 1
	}, format,
		&pBoolean, &pInteger, &pReal, &pIntegerSpin, &pAngle,
		&pString, &pMulti, &pList, &pOptions, &pFile, &pColor, &pFont, &pDate,
	)

	if ret == 1 {
		fmt.Println("OK")
		fmt.Printf("Boolean: %d\n", pBoolean)
		fmt.Printf("Integer: %d\n", pInteger)
		fmt.Printf("Real: %f\n", pReal)
		fmt.Printf("Spin: %d\n", pIntegerSpin)
		fmt.Printf("Angle: %f\n", pAngle)
		fmt.Printf("String: %s\n", pString)
		fmt.Printf("Multiline: %s\n", pMulti)
		fmt.Printf("List: %d\n", pList)
		fmt.Printf("Options: %d\n", pOptions)
		fmt.Printf("File: %s\n", pFile)
		fmt.Printf("Color: %s\n", pColor)
		fmt.Printf("Font: %s\n", pFont)
		fmt.Printf("Date: %s\n", pDate)
	} else {
		fmt.Println("Cancel")
	}
}
