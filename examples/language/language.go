package main

import (
	"github.com/gen2brain/iup-go/iup"
)

// String name constants used as translation keys.
const (
	strTitle    = "APP_TITLE"
	strGreeting = "APP_GREETING"
	strMessage  = "APP_MESSAGE"
	strEnglish  = "APP_ENGLISH"
	strSpanish  = "APP_SPANISH"
	strFrench   = "APP_FRENCH"
	strGerman   = "APP_GERMAN"
	strClose    = "APP_CLOSE"
)

func loadEnglish() {
	pack := iup.User()
	defer pack.Destroy()

	pack.SetAttribute(strTitle, "Language String Example")
	pack.SetAttribute(strGreeting, "Hello!")
	pack.SetAttribute(strMessage, "Select a language to see the interface update.\nThis demonstrates IupSetLanguagePack for internationalization.")
	pack.SetAttribute(strEnglish, "English")
	pack.SetAttribute(strSpanish, "Spanish")
	pack.SetAttribute(strFrench, "French")
	pack.SetAttribute(strGerman, "German")
	pack.SetAttribute(strClose, "Close")

	iup.SetLanguagePack(pack)
	iup.SetLanguage("ENGLISH")
}

func loadSpanish() {
	pack := iup.User()
	defer pack.Destroy()

	pack.SetAttribute(strTitle, "Ejemplo de cadena de idioma")
	pack.SetAttribute(strGreeting, "\u00a1Hola!")
	pack.SetAttribute(strMessage, "Seleccione un idioma para ver la interfaz actualizarse.\nEsto demuestra IupSetLanguagePack para la internacionalizaci\u00f3n.")
	pack.SetAttribute(strEnglish, "Ingl\u00e9s")
	pack.SetAttribute(strSpanish, "Espa\u00f1ol")
	pack.SetAttribute(strFrench, "Franc\u00e9s")
	pack.SetAttribute(strGerman, "Alem\u00e1n")
	pack.SetAttribute(strClose, "Cerrar")

	iup.SetLanguagePack(pack)
	iup.SetLanguage("SPANISH")
}

func loadFrench() {
	pack := iup.User()
	defer pack.Destroy()

	pack.SetAttribute(strTitle, "Exemple de cha\u00eene de langue")
	pack.SetAttribute(strGreeting, "Bonjour!")
	pack.SetAttribute(strMessage, "S\u00e9lectionnez une langue pour mettre \u00e0 jour l'interface.\nCeci d\u00e9montre IupSetLanguagePack pour l'internationalisation.")
	pack.SetAttribute(strEnglish, "Anglais")
	pack.SetAttribute(strSpanish, "Espagnol")
	pack.SetAttribute(strFrench, "Fran\u00e7ais")
	pack.SetAttribute(strGerman, "Allemand")
	pack.SetAttribute(strClose, "Fermer")

	iup.SetLanguagePack(pack)
	iup.SetLanguage("FRENCH")
}

func loadGerman() {
	pack := iup.User()
	defer pack.Destroy()

	pack.SetAttribute(strTitle, "Sprachzeichenketten-Beispiel")
	pack.SetAttribute(strGreeting, "Hallo!")
	pack.SetAttribute(strMessage, "W\u00e4hlen Sie eine Sprache, um die Oberfl\u00e4che zu aktualisieren.\nDies demonstriert IupSetLanguagePack f\u00fcr die Internationalisierung.")
	pack.SetAttribute(strEnglish, "Englisch")
	pack.SetAttribute(strSpanish, "Spanisch")
	pack.SetAttribute(strFrench, "Franz\u00f6sisch")
	pack.SetAttribute(strGerman, "Deutsch")
	pack.SetAttribute(strClose, "Schlie\u00dfen")

	iup.SetLanguagePack(pack)
	iup.SetLanguage("GERMAN")
}

var loaders = map[string]func(){
	"ENGLISH": loadEnglish,
	"SPANISH": loadSpanish,
	"FRENCH":  loadFrench,
	"GERMAN":  loadGerman,
}

func updateUI(dlg, lblGreeting, lblMessage, btnEn, btnEs, btnFr, btnDe, btnClose iup.Ihandle) {
	dlg.SetAttribute("TITLE", iup.GetLanguageString(strTitle))
	lblGreeting.SetAttribute("TITLE", iup.GetLanguageString(strGreeting))
	lblMessage.SetAttribute("TITLE", iup.GetLanguageString(strMessage))
	btnEn.SetAttribute("TITLE", iup.GetLanguageString(strEnglish))
	btnEs.SetAttribute("TITLE", iup.GetLanguageString(strSpanish))
	btnFr.SetAttribute("TITLE", iup.GetLanguageString(strFrench))
	btnDe.SetAttribute("TITLE", iup.GetLanguageString(strGerman))
	btnClose.SetAttribute("TITLE", iup.GetLanguageString(strClose))

	iup.Refresh(dlg)
}

func main() {
	iup.Open()
	defer iup.Close()

	loadEnglish()

	lblGreeting := iup.Label("_@" + strGreeting)
	lblGreeting.SetAttribute("FONTSTYLE", "Bold")
	lblGreeting.SetAttribute("FONTSIZE", "16")

	lblMessage := iup.Label("_@" + strMessage)

	btnEn := iup.Button("_@" + strEnglish)
	btnEs := iup.Button("_@" + strSpanish)
	btnFr := iup.Button("_@" + strFrench)
	btnDe := iup.Button("_@" + strGerman)

	btnClose := iup.Button("_@" + strClose)

	btnBox := iup.Hbox(btnEn, btnEs, btnFr, btnDe).SetAttribute("GAP", "5")

	dlg := iup.Dialog(
		iup.Vbox(
			lblGreeting,
			iup.Space().SetAttribute("SIZE", "x5"),
			lblMessage,
			iup.Space().SetAttribute("SIZE", "x10"),
			btnBox,
			iup.Space().SetAttribute("SIZE", "x5"),
			btnClose,
		).SetAttributes(`MARGIN=15x15, GAP=5, ALIGNMENT=ACENTER`),
	)

	dlg.SetAttribute("TITLE", iup.GetLanguageString(strTitle))
	dlg.SetAttribute("SIZE", "300x")

	switchLanguage := func(lang string) iup.ActionFunc {
		return func(ih iup.Ihandle) int {
			loaders[lang]()
			updateUI(dlg, lblGreeting, lblMessage, btnEn, btnEs, btnFr, btnDe, btnClose)
			return iup.DEFAULT
		}
	}

	iup.SetCallback(btnEn, "ACTION", switchLanguage("ENGLISH"))
	iup.SetCallback(btnEs, "ACTION", switchLanguage("SPANISH"))
	iup.SetCallback(btnFr, "ACTION", switchLanguage("FRENCH"))
	iup.SetCallback(btnDe, "ACTION", switchLanguage("GERMAN"))
	iup.SetCallback(btnClose, "ACTION", iup.ActionFunc(func(ih iup.Ihandle) int {
		return iup.CLOSE
	}))

	iup.Show(dlg)
	iup.MainLoop()
}
