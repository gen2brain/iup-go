// Tiny static file server for live testing the IUP WASM build in a browser.
//
//	go run . [-dir ../build] [-port 8000]
//
// Serves .wasm with the correct application/wasm MIME type so streaming
// instantiation works, and gzips responses when the client accepts it.
package main

import (
	"compress/gzip"
	"flag"
	"log"
	"mime"
	"net/http"
	"strings"
)

type gzipWriter struct {
	http.ResponseWriter
	gz *gzip.Writer
}

func (w gzipWriter) Write(b []byte) (int, error) { return w.gz.Write(b) }

// drop the file size; gzipped length is unknown, so respond chunked.
func (w gzipWriter) WriteHeader(code int) {
	w.Header().Del("Content-Length")
	w.ResponseWriter.WriteHeader(code)
}

func main() {
	dir := flag.String("dir", "../build", "directory to serve")
	port := flag.String("port", "8000", "port to listen on")
	flag.Parse()

	_ = mime.AddExtensionType(".wasm", "application/wasm")
	_ = mime.AddExtensionType(".js", "text/javascript")

	fs := http.FileServer(http.Dir(*dir))
	handler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// SharedArrayBuffer needs cross-origin isolation.
		w.Header().Set("Cross-Origin-Opener-Policy", "same-origin")
		w.Header().Set("Cross-Origin-Embedder-Policy", "require-corp")
		// dev server: a plain reload pick up the latest rebuild.
		w.Header().Set("Cache-Control", "no-store")

		if strings.Contains(r.Header.Get("Accept-Encoding"), "gzip") {
			w.Header().Set("Content-Encoding", "gzip")
			w.Header().Add("Vary", "Accept-Encoding")
			r.Header.Del("Range") // gzip and partial content don't mix
			gz := gzip.NewWriter(w)
			defer gz.Close()
			w = gzipWriter{ResponseWriter: w, gz: gz}
		}
		fs.ServeHTTP(w, r)
	})

	log.Printf("serving %s at http://localhost:%s/", *dir, *port)
	if err := http.ListenAndServe(":"+*port, handler); err != nil {
		log.Fatal(err)
	}
}
