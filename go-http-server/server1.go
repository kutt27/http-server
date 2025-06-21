package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
)

func main() {

	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		log.Println("Hello world!")
		d, err := io.ReadAll(r.Body)

		if err != nil {
			http.Error(w, "oops", http.StatusBadRequest)
			return
		}

		fmt.Fprintf(w, "hello %s", d)
	})

	http.HandleFunc("/goodbye", func(w http.ResponseWriter, r *http.Request) {
		log.Println("Goodbye world!")
	})

	http.ListenAndServe(":9090", nil)
}


// run this code as `go run server1.go`
// open a new shell and give a request with a simple message `curl -v -d "status" localhost:9090`
