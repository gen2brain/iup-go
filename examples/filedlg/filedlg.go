// The PORTAL attribute controls whether to use the legacy GtkFileChooserDialog.
//
// Default behavior (PORTAL not set or PORTAL=YES):
//   - GTK4: Uses GtkFileDialog (modern async API, introduced in GTK 4.10).
//           Internally uses GtkFileChooserNative, which uses XDG Desktop Portal
//           when running inside a sandbox (Flatpak, Snap) or falls back to native dialog.
//   - GTK3: Uses GtkFileChooserNative with the same portal detection behavior.
//   - Qt: Uses native/portal dialog (default Qt behavior on Linux).
//
// PORTAL=NO:
//   - Uses legacy GtkFileChooserDialog widget.
//   - Required when using SHOWPREVIEW with FILE_CB callback (custom preview drawing).
//   - Never uses XDG Desktop Portal.
//
// SANDBOX=YES (read-only):
//   - Indicates app is running in a sandbox, portal will be used automatically.
//
// To test portal mode on GTK3 outside a sandbox, run with:
//   GTK_USE_PORTAL=1 ./filedlg_portal

package main

import (
	"fmt"
	"image"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"os"
	"strings"

	"github.com/gen2brain/iup-go/iup"
)

func main() {
	iup.Open()
	defer iup.Close()

	// Button to show file dialog with PORTAL=YES
	btnPortal := iup.Button("Open File (Portal Mode)")
	iup.SetCallback(btnPortal, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "OPEN")
		filedlg.SetAttribute("TITLE", "Open File (Portal)")
		filedlg.SetAttribute("EXTFILTER", "Image Files|*.png;*.jpg;*.jpeg;*.gif|All Files|*.*")
		filedlg.SetAttribute("PORTAL", "YES")

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		status := filedlg.GetInt("STATUS")
		if status >= 0 {
			fmt.Printf("Portal mode selected: %s\n", filedlg.GetAttribute("VALUE"))
		} else {
			fmt.Println("Portal mode: Canceled")
		}

		filedlg.Destroy()
		return iup.DEFAULT
	}))

	// Button to show file dialog with PORTAL=NO (native widget dialog without portal)
	btnNative := iup.Button("Open File (Native Mode)")
	iup.SetCallback(btnNative, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "OPEN")
		filedlg.SetAttribute("TITLE", "Open File (Native)")
		filedlg.SetAttribute("EXTFILTER", "Image Files|*.png;*.jpg;*.jpeg;*.gif|All Files|*.*")
		filedlg.SetAttribute("PORTAL", "NO")

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		status := filedlg.GetInt("STATUS")
		if status >= 0 {
			fmt.Printf("Native mode selected: %s\n", filedlg.GetAttribute("VALUE"))
		} else {
			fmt.Println("Native mode: Canceled")
		}

		filedlg.Destroy()
		return iup.DEFAULT
	}))

	// Button to show file dialog with SHOWPREVIEW
	btnPreview := iup.Button("Open File (Preview Mode)")
	iup.SetCallback(btnPreview, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "OPEN")
		filedlg.SetAttribute("TITLE", "Open File (Preview)")
		filedlg.SetAttribute("EXTFILTER", "Image Files|*.png;*.jpg;*.jpeg;*.gif|All Files|*.*")
		filedlg.SetAttribute("SHOWPREVIEW", "YES")
		filedlg.SetAttribute("PREVIEWWIDTH", "250")
		filedlg.SetAttribute("PREVIEWHEIGHT", "250")

		var previewImage iup.Ihandle
		var lastFilename string

		iup.SetCallback(filedlg, "FILE_CB", iup.FileFunc(func(ih iup.Ihandle, filename, status string) int {
			switch status {
			case "INIT":
				fmt.Println("Preview: Dialog initialized")
			case "SELECT":
				fmt.Printf("Preview: File selected: %s\n", filename)
			case "PAINT":
				if filename != lastFilename {
					if previewImage != 0 {
						previewImage.Destroy()
						previewImage = 0
					}
					lastFilename = filename
					if filename != "" && isImageFile(filename) {
						previewImage = loadPreviewImage(filename)
						if previewImage != 0 {
							iup.SetHandle("_PREVIEW_IMAGE_", previewImage)
						}
					}
				}

				iup.DrawBegin(ih)

				canvasW, canvasH := iup.DrawGetSize(ih)

				iup.DrawParentBackground(ih)

				if previewImage != 0 {
					imgW, imgH, _ := iup.DrawGetImageInfo("_PREVIEW_IMAGE_")
					x := (canvasW - imgW) / 2
					y := (canvasH - imgH) / 2
					iup.DrawImage(ih, "_PREVIEW_IMAGE_", x, y, imgW, imgH)
				} else {
					ih.SetAttribute("DRAWCOLOR", "128 128 128")
					textW, textH := iup.DrawGetTextSize(ih, "No preview")
					x := (canvasW - textW) / 2
					y := (canvasH - textH) / 2
					iup.DrawText(ih, "No preview", x, y, 0, 0)
				}
				iup.DrawEnd(ih)
			case "FINISH":
				fmt.Println("Preview: Dialog finished")
				if previewImage != 0 {
					previewImage.Destroy()
					previewImage = 0
				}
			}
			return iup.DEFAULT
		}))

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		status := filedlg.GetInt("STATUS")
		if status >= 0 {
			fmt.Printf("Preview mode selected: %s\n", filedlg.GetAttribute("VALUE"))
		} else {
			fmt.Println("Preview mode: Canceled")
		}

		filedlg.Destroy()
		return iup.DEFAULT
	}))

	// Button to show file dialog without a PORTAL attribute (default behavior)
	btnDefault := iup.Button("Open File (Default)")
	iup.SetCallback(btnDefault, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "OPEN")
		filedlg.SetAttribute("TITLE", "Open File (Default)")
		filedlg.SetAttribute("EXTFILTER", "Image Files|*.png;*.jpg;*.jpeg;*.gif|All Files|*.*")

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		status := filedlg.GetInt("STATUS")
		if status >= 0 {
			fmt.Printf("Default mode selected: %s\n", filedlg.GetAttribute("VALUE"))
		} else {
			fmt.Println("Default mode: Canceled")
		}

		filedlg.Destroy()
		return iup.DEFAULT
	}))

	// Button to save a file with PORTAL=YES
	btnSavePortal := iup.Button("Save File (Portal Mode)")
	iup.SetCallback(btnSavePortal, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "SAVE")
		filedlg.SetAttribute("TITLE", "Save File (Portal)")
		filedlg.SetAttribute("FILTER", "*.txt")
		filedlg.SetAttribute("FILTERINFO", "Text Files")
		filedlg.SetAttribute("PORTAL", "YES")

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		status := filedlg.GetInt("STATUS")
		if status >= 0 {
			fmt.Printf("Portal save: %s\n", filedlg.GetAttribute("VALUE"))
		} else {
			fmt.Println("Portal save: Canceled")
		}

		filedlg.Destroy()
		return iup.DEFAULT
	}))

	// Button to select directory with PORTAL=YES
	btnDirPortal := iup.Button("Select Directory (Portal Mode)")
	iup.SetCallback(btnDirPortal, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		filedlg := iup.FileDlg()
		filedlg.SetAttribute("DIALOGTYPE", "DIR")
		filedlg.SetAttribute("TITLE", "Select Directory (Portal)")
		filedlg.SetAttribute("PORTAL", "YES")

		iup.Popup(filedlg, iup.CENTER, iup.CENTER)

		status := filedlg.GetInt("STATUS")
		if status >= 0 {
			fmt.Printf("Portal dir: %s\n", filedlg.GetAttribute("VALUE"))
		} else {
			fmt.Println("Portal dir: Canceled")
		}

		filedlg.Destroy()
		return iup.DEFAULT
	}))

	// Create dialog
	dlg := iup.Dialog(
		iup.Vbox(
			iup.Label("IupFileDlg PORTAL Attribute").SetAttributes(`FONT="Sans, Bold 12"`),
			iup.Hbox(btnPortal, btnNative, btnDefault).SetAttributes("GAP=5"),
			iup.Hbox(btnPreview, btnSavePortal, btnDirPortal).SetAttributes("GAP=5"),
		).SetAttributes("MARGIN=10x10, GAP=10"),
	)

	dlg.SetAttributes(`TITLE="FileDlg Portal", SIZE=500x200`)

	iup.Show(dlg)
	iup.MainLoop()
}

func isImageFile(filename string) bool {
	lower := strings.ToLower(filename)
	return strings.HasSuffix(lower, ".png") ||
		strings.HasSuffix(lower, ".jpg") ||
		strings.HasSuffix(lower, ".jpeg") ||
		strings.HasSuffix(lower, ".gif")
}

func loadPreviewImage(filename string) iup.Ihandle {
	file, err := os.Open(filename)
	if err != nil {
		return 0
	}
	defer file.Close()

	img, _, err := image.Decode(file)
	if err != nil {
		return 0
	}

	const targetWidth = 180
	bounds := img.Bounds()
	w, h := bounds.Dx(), bounds.Dy()

	if w != targetWidth {
		scale := float64(targetWidth) / float64(w)
		newH := int(float64(h) * scale)
		img = resizeImage(img, targetWidth, newH)
	}

	return iup.ImageFromImage(img)
}

func resizeImage(src image.Image, newWidth, newHeight int) image.Image {
	bounds := src.Bounds()
	srcW, srcH := bounds.Dx(), bounds.Dy()

	dst := image.NewRGBA(image.Rect(0, 0, newWidth, newHeight))

	for y := 0; y < newHeight; y++ {
		for x := 0; x < newWidth; x++ {
			srcX := x * srcW / newWidth
			srcY := y * srcH / newHeight
			dst.Set(x, y, src.At(bounds.Min.X+srcX, bounds.Min.Y+srcY))
		}
	}

	return dst
}
