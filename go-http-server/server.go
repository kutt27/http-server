package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
)

const (
	Port    = ":8080"
	WebRoot = "./www"
)

func main() {
	if _, err := os.Stat(WebRoot); os.IsNotExist(err) {
		log.Printf("Creating %s directory...", WebRoot)
		err := os.Mkdir(WebRoot, 0755)
		if err != nil {
			log.Fatalf("Failed to create web root directory: %v", err)
		}
		err = os.WriteFile(filepath.Join(WebRoot, "index.html"), []byte("<h1>Hello from Go Server!</h1><p>This is index.html</p>"), 0644)
		if err != nil {
			log.Printf("Warning: Failed to create index.html: %v", err)
		}
		err = os.WriteFile(filepath.Join(WebRoot, "style.css"), []byte("body { background-color: #e0f7fa; font-family: sans-serif; }"), 0644)
		if err != nil {
			log.Printf("Warning: Failed to create style.css: %v", err)
		}
		err = os.WriteFile(filepath.Join(WebRoot, "script.js"), []byte("console.log('Hello from JavaScript!');"), 0644)
		if err != nil {
			log.Printf("Warning: Failed to create script.js: %v", err)
		}
	}

	fs := http.FileServer(http.Dir(WebRoot))
	http.Handle("/", customFileServer(fs))

	fmt.Printf("Starting server on port %s. Serving files from %s\n", Port, WebRoot)
	fmt.Printf("You can access it at http://localhost%s/\n", Port)

	log.Fatal(http.ListenAndServe(Port, nil))
}

func customFileServer(fs http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		log.Printf("Request received: Method=%s, Path=%s, From=%s", r.Method, r.URL.Path, r.RemoteAddr)

		if r.Method != http.MethodGet {
			http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
			return
		}

		if r.URL.Path == "/" {
			r.URL.Path = "/index.html"
		}

		if strings.Contains(r.URL.Path, "..") {
			http.Error(w, "Forbidden", http.StatusForbidden)
			return
		}

		fs.ServeHTTP(w, r)
	})
}
