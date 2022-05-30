package iup

import (
	"image"
	"image/draw"
	"unsafe"

	"github.com/google/uuid"
)

/*
#include <stdlib.h>
#include "iup.h"

static void Log(const char* type, const char* str) {
	IupLog(type, "%s", str);
}
*/
import "C"

// Image creates an image to be shown on a label, button, toggle, or as a cursor.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupimage.html
func Image(width, height int, pixMap []byte) Ihandle {
	h := mkih(C.IupImage(C.int(width), C.int(height), (*C.uchar)(unsafe.Pointer(&pixMap[0]))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ImageRGB creates an image to be shown on a label, button, toggle, or as a cursor.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupimage.html
func ImageRGB(width, height int, pixMap []byte) Ihandle {
	h := mkih(C.IupImageRGB(C.int(width), C.int(height), (*C.uchar)(unsafe.Pointer(&pixMap[0]))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ImageRGBA creates an image to be shown on a label, button, toggle, or as a cursor.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupimage.html
func ImageRGBA(width, height int, pixMap []byte) Ihandle {
	h := mkih(C.IupImageRGBA(C.int(width), C.int(height), (*C.uchar)(unsafe.Pointer(&pixMap[0]))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// ImageFromImage creates an image from image.Image.
func ImageFromImage(i image.Image) Ihandle {
	if img, ok := i.(*image.RGBA); ok {
		h := mkih(C.IupImageRGBA(C.int(img.Bounds().Dx()), C.int(img.Bounds().Dy()), (*C.uchar)(unsafe.Pointer(&img.Pix[0]))))
		h.SetAttribute("UUID", uuid.NewString())
		return h
	}

	b := i.Bounds()
	dst := image.NewRGBA(image.Rect(0, 0, b.Dx(), b.Dy()))
	draw.Draw(dst, dst.Bounds(), i, b.Min, draw.Src)

	h := mkih(C.IupImageRGBA(C.int(dst.Bounds().Dx()), C.int(dst.Bounds().Dy()), (*C.uchar)(unsafe.Pointer(&dst.Pix[0]))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// NextField shifts the focus to the next element that can have the focus.
// It is relative to the given element and does not depend on the element currently with the focus.
//
// It will search for the next element first in the children, then in the brothers,
// then in the uncles and their children, and so on.
//
// This sequence is not the same sequence used by the Tab key, which is dependent on the native system.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupnextfield.html
func NextField(ih Ihandle) Ihandle {
	return mkih(C.IupNextField(ih.ptr()))
}

// PreviousField shifts the focus to the previous element that can have the focus.
// It is relative to the given element and does not depend on the element currently with the focus.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuppreviousfield.html
func PreviousField(ih Ihandle) Ihandle {
	return mkih(C.IupPreviousField(ih.ptr()))
}

// GetFocus returns the identifier of the interface element that has the keyboard focus, i.e. the element that will receive keyboard events.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetfocus.html
func GetFocus() Ihandle {
	return mkih(C.IupGetFocus())
}

// SetFocus sets the interface element that will receive the keyboard focus, i.e., the element that will receive keyboard events.
// But this will be processed only after the control actually receive the focus.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetfocus.html
func SetFocus(ih Ihandle) Ihandle {
	return mkih(C.IupSetFocus(ih.ptr()))
}

// Item creates an item of the menu interface element. When selected, it generates an action.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupitem.html
func Item(title string) Ihandle {
	cTitle := C.CString(title)
	defer C.free(unsafe.Pointer(cTitle))

	h := mkih(C.IupItem(cTitle, nil))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Menu Creates a menu element, which groups 3 types of interface elements: item, submenu and separator.
// Any other interface element defined inside a menu will be an error.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupmenu.html
func Menu(children ...Ihandle) Ihandle {
	children = append(children, Ihandle(0))

	h := mkih(C.IupMenuv((**C.Ihandle)(unsafe.Pointer(&(children[0])))))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Separator creates the separator interface element. It shows a line between two menu items.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupseparator.html
func Separator() Ihandle {
	h := mkih(C.IupSeparator())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Submenu creates a menu item that, when selected, opens another menu.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupsubmenu.html
func Submenu(title string, child Ihandle) Ihandle {
	cTitle := C.CString(title)
	defer C.free(unsafe.Pointer(cTitle))

	h := mkih(C.IupSubmenu(cTitle, child.ptr()))
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// SetHandle associates a name with an interface element.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsethandle.html
func SetHandle(name string, ih Ihandle) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return mkih(C.IupSetHandle(cName, ih.ptr()))
}

// GetHandle returns the identifier of an interface element that has an associated name using SetHandle.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgethandle.html
func GetHandle(name string) Ihandle {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return mkih(C.IupGetHandle(cName))
}

// GetName Returns a name of an interface element, if the element has an associated name using SetHandle.
// Notice that a handle can have many names. GetName will return the last name set.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetname.html
func GetName(ih Ihandle) string {
	return C.GoString(C.IupGetName(ih.ptr()))
}

// GetAllNames returns the names of all interface elements that have an associated name using SetHandle.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetallnames.html
func GetAllNames() (names []string) {
	n := int(C.IupGetAllNames(nil, 0))
	if n > 0 {
		names = make([]string, n)
		pNames := make([]*C.char, n)
		C.IupGetAllNames((**C.char)(unsafe.Pointer(&pNames[0])), C.int(n))
		for i := 0; i < n; i++ {
			names[i] = C.GoString(pNames[i])
		}
	}
	return
}

// GetAllDialogs returns the names of all dialogs that have an associated name using SetHandle.
// Other dialogs will not be returned.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetalldialogs.html
func GetAllDialogs() (names []string) {
	n := int(C.IupGetAllDialogs(nil, 0))
	if n > 0 {
		names = make([]string, n)
		pNames := make([]*C.char, n)
		C.IupGetAllDialogs((**C.char)(unsafe.Pointer(&pNames[0])), C.int(n))
		for i := 0; i < n; i++ {
			names[i] = C.GoString(pNames[i])
		}
	}
	return
}

// SetLanguage sets the language name used by some pre-defined dialogs.
// Can also be changed using the global attribute LANGUAGE.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetlanguage.html
func SetLanguage(lng string) {
	cLng := C.CString(lng)
	defer C.free(unsafe.Pointer(cLng))

	C.IupSetLanguage(cLng)
}

// GetLanguage returns the language used by some pre-defined dialogs.
// Returns the same value as the LANGUAGE global attribute.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetlanguage.html
func GetLanguage() string {
	return C.GoString(C.IupGetLanguage())
}

// SetLanguageString associates a name with a string as an auxiliary method for Internationalization of applications.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetlanguagestring.html
func SetLanguageString(name, str string) {
	cName, cStr := C.CString(name), C.CString(str)
	defer C.free(unsafe.Pointer(cName))
	defer C.free(unsafe.Pointer(cStr))

	C.IupStoreLanguageString(cName, cStr) //NOTE string always duplicated
}

// GetLanguageString returns a language dependent string.
// The string must have been associated with the name using the SetLanguageString or SetLanguagePack functions.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupgetlanguagestring.html
func GetLanguageString(name string) string {
	cName := C.CString(name)
	defer C.free(unsafe.Pointer(cName))

	return C.GoString(C.IupGetLanguageString(cName))
}

// SetLanguagePack sets a pack of associations between names and string values.
// It is simply a User element with several attributes set.
// Internally will call SetLanguageString for each name in the pack.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupsetlanguagepack.html
func SetLanguagePack(ih Ihandle) {
	C.IupSetLanguagePack(ih.ptr())
}

// Clipboard creates an element that allows access to the clipboard.
// Each clipboard should be destroyed using Destroy,
// but you can use only one for the entire application because it does not store any data inside.
// Or you can simply create and destroy every time you need to copy or paste.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupclipboard.html
func Clipboard() Ihandle {
	h := mkih(C.IupClipboard())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Timer creates a timer which periodically invokes a callback when the time is up.
// Each timer should be destroyed using Destroy.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iuptimer.html
func Timer() Ihandle {
	h := mkih(C.IupTimer())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Thread creates a thread element in IUP, which is not associated to any interface element.
// It is a very simple support to create and manage threads in a multithreading environment.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupthread.html
func Thread() Ihandle {
	h := mkih(C.IupThread())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// User creates a user element in IUP, which is not associated to any interface element.
// It is used to map an external element to a IUP element.
// Its use is usually for additional elements, but you can use it to create an Ihandle to store private attributes.
//
// It is also a void container. Its children can be dynamically added using Append or Insert.
//
// https://www.tecgraf.puc-rio.br/iup/en/elem/iupuser.html
func User() Ihandle {
	h := mkih(C.IupUser())
	h.SetAttribute("UUID", uuid.NewString())
	return h
}

// Execute runs the executable with the given parameters.
// It is a non synchronous operation, i.e. the function will return just after execute the command and it will not wait for its result.
// In Windows, there is no need to add the ".exe" file extension.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupexecute.html
func Execute(fileName, parameters string) int {
	cFileName := C.CString(fileName)
	defer C.free(unsafe.Pointer(cFileName))

	return int(C.IupExecute(cFileName, C.CString(parameters)))
}

// ExecuteWait runs the executable with the given parameters.
// It is a synchronous operation, i.e. the function will wait the command to terminate before it returns.
// In Windows, there is no need to add the ".exe" file extension.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iupexecutewait.html
func ExecuteWait(fileName, parameters string) int {
	cFileName := C.CString(fileName)
	defer C.free(unsafe.Pointer(cFileName))

	return int(C.IupExecuteWait(cFileName, C.CString(parameters)))
}

// Help opens the given URL. In UNIX executes Netscape, Safari (MacOS) or Firefox (in Linux) passing the desired URL as a parameter.
// In Windows executes the shell "open" operation on the given URL.
// In UNIX you can change the used browser setting the environment variable IUP_HELPAPP or using the global attribute "HELPAPP".
// It is a non synchronous operation, i.e. the function will return just after execute the command and it will not wait for its result.
// Since IUP 3.17, it will use the Execute function.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuphelp.html
func Help(url string) int {
	cUrl := C.CString(url)
	defer C.free(unsafe.Pointer(cUrl))

	return int(C.IupHelp(cUrl))
}

// Log writes a message to the system log.
// In Windows, writes to the Application event log. In Linux, writes to the Syslog.
//
// https://www.tecgraf.puc-rio.br/iup/en/func/iuplog.html
func Log(_type, str string) {
	cType, cStr := C.CString(_type), C.CString(str)
	defer C.free(unsafe.Pointer(cType))
	defer C.free(unsafe.Pointer(cStr))

	C.Log(cType, cStr)
}
