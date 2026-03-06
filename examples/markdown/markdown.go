package main

import (
	"crypto/sha256"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"regexp"

	"github.com/gen2brain/iup-go/iup"
)

const markdownText = `# Heading 1
## Heading 2
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6

This is a regular paragraph with **bold text**, *italic text*, and ***bold italic text*** mixed together.

Alternate syntax: __double underscores for bold__ and _single underscores for italic_.

Here is some ` + "`inline code`" + ` in a sentence, and ` + "``code with `backticks` inside``" + ` too.

` + "```" + `
func main() {
    fmt.Println("Hello, World!")
}
` + "```" + `

[Click here for IUP docs](https://www.tecgraf.puc-rio.br/iup/)

![Gopher](gopher.png)

![Remote Gopher](https://go.dev/doc/gopher/frontpage.png)

- First unordered item
- Second unordered item
- Third unordered item

1. First ordered item
2. Second ordered item
3. Third ordered item

> This is a blockquote.
> It can span multiple lines.

---

Escaped characters: \*not italic\*, \**not bold\**, \` + "`not code`" + `.

Line with hard break\
continues on next line.

Final paragraph with **nested *bold and italic* text** to show combined formatting.`

var imagePattern = regexp.MustCompile(`!\[([^\]]*)\]\((https?://[^)]+)\)`)

func main() {
	iup.Open()
	defer iup.Close()

	md := markdownText
	if len(os.Args) > 1 {
		mdPath, err := filepath.Abs(os.Args[1])
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error resolving %s: %v\n", os.Args[1], err)
			os.Exit(1)
		}

		data, err := os.ReadFile(mdPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error reading %s: %v\n", mdPath, err)
			os.Exit(1)
		}
		md = string(data)

		os.Chdir(filepath.Dir(mdPath))
	}

	md = fetchRemoteImages(md)

	mltline := iup.MultiLine()
	iup.SetAttribute(mltline, "FORMATTING", "YES")
	iup.SetAttribute(mltline, "SIZE", "400x300")
	iup.SetAttribute(mltline, "EXPAND", "YES")
	iup.SetAttribute(mltline, "READONLY", "YES")

	iup.SetCallback(mltline, "LINK_CB", iup.TextLinkFunc(func(ih iup.Ihandle, url string) int {
		fmt.Printf("Link clicked: %s\n", url)
		return iup.DEFAULT
	}))

	dlg := iup.Dialog(iup.Vbox(mltline))
	iup.SetAttribute(dlg, "TITLE", "Markdown Example")
	iup.SetAttribute(dlg, "MARGIN", "10x10")

	iup.Map(dlg)

	iup.SetAttribute(mltline, "MARKDOWNVALUE", md)

	iup.ShowXY(dlg, iup.CENTER, iup.CENTER)
	iup.MainLoop()
}

func fetchRemoteImages(md string) string {
	tmpDir := os.TempDir()

	return imagePattern.ReplaceAllStringFunc(md, func(match string) string {
		parts := imagePattern.FindStringSubmatch(match)
		alt := parts[1]
		url := parts[2]

		ext := filepath.Ext(url)
		if ext == "" || len(ext) > 5 {
			ext = ".png"
		}

		hash := fmt.Sprintf("%x", sha256.Sum256([]byte(url)))
		localPath := filepath.Join(tmpDir, "iup_md_"+hash[:16]+ext)

		if _, err := os.Stat(localPath); err == nil {
			return fmt.Sprintf("![%s](%s)", alt, localPath)
		}

		resp, err := http.Get(url)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to fetch %s: %v\n", url, err)
			return match
		}
		defer resp.Body.Close()

		if resp.StatusCode != http.StatusOK {
			fmt.Fprintf(os.Stderr, "Failed to fetch %s: %s\n", url, resp.Status)
			return match
		}

		f, err := os.Create(localPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to create %s: %v\n", localPath, err)
			return match
		}

		_, err = io.Copy(f, resp.Body)
		f.Close()
		if err != nil {
			os.Remove(localPath)
			fmt.Fprintf(os.Stderr, "Failed to write %s: %v\n", localPath, err)
			return match
		}

		return fmt.Sprintf("![%s](%s)", alt, localPath)
	})
}
