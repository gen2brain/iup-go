// Package iup implements Go bindings for IUP (https://www.tecgraf.puc-rio.br/iup/).
//
// IUP is a multi-platform toolkit for building graphical user interfaces.
//
// Currently available interface elements can be categorized as follows:
//
//  - Primitives (effective user interaction): dialog, label, button, text, multi-line, list, toggle, canvas, frame, image.
//  - Composition (ways to show the elements): hbox, vbox, zbox, fill.
//  - Grouping (definition of a common functionality for a group of elements): radio.
//  - Menu (related both to menu bars and to pop-up menus): menu, submenu, item, separator.
//  - Dialogs (useful predefined dialogs): file selection, message, alarm, data input, list selection.
//
// IUP has 3 concepts that any user has to understand: Elements, Attributes and Callbacks.
//
// Elements are every kind of interface element present in the application. IUP contains several user interface elements. The library's main characteristic is the use of native elements. This means that the drawing and management of a button or text box is done by the native interface system, not by IUP. This makes the application's appearance more similar to other applications in that system. On the other hand, the application's appearance can vary from one system to another. Besides, some additional controls are drawn by IUP, and are independent from the native system. Dialogs are special elements that represent every window created by IUP. Any application that uses IUP will be composed by one or more dialogs. Every dialog can contains one or more controls inside.
// IUP controls are never positioned in a specific (x,y) coordinate inside the dialog. The positioning is always calculated dynamically from the abstract layout hierarchy. So get used to the Fill, Hbox and Vbox controls that allows you to position the controls in the dialog.
//
// Attributes are used to change or consult properties of elements. Each element has a set of attributes that affects its behavior or its appearance. Each attribute may work differently for each elements, but usually attributes with the same name work the same. Attribute names are always upper case. But attribute values like "YES", "NO", "TOP", are case insensitive, so "Yes", "no", "top", and other variations will work.
// IUP has only a few functions because it uses string attributes to access the properties of each control. So get used to SetAttribute and GetAttribute, because you are going to use them a lot.
// Attribute names are always upper case, lower case names will not work. But attribute values like "YES", "NO", "TOP", are case insensitive, so "Yes", "no", "top", and other variations will work.
// If not defined their value can be inherited from the parent container.
// Boolean attributes accept the values "1", "YES" or "ON" for true, and "0", "NO" or "OFF" for false. But they will return the value described in the documentation. You can also use SetInt with 1 for true and 0 for false. GetInt will return 1 for any of the true values, and 0 for any of the false values.
//
// Callbacks are functions which notify the application that some user interface event occurred. Usually callbacks will be called only when the user interacts with the application elements. If the application register the callback function, then the function will be called every time the event occurs.
//
// IUP has several global tables along with some system tools that must be initialized before any dialog is created.
// The default system language used by predefined dialogs and messages is English. But it can be changed to Portuguese or Spanish.
//
// Before running any of IUP’s functions, function Open must be run to initialize the toolkit.
// After running the last IUP function, function Close must be run so that the toolkit can free internal memory and close the interface system.
// Executing these functions in this order is crucial for the correct functioning of the toolkit.
// Between calls to the Open and Close functions, the application can create dialogs and display them.
package iup
