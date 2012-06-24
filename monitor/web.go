package main

import (
	"encoding/json"
	"log"
	"net/http"
	"text/template"
)

const htmltmplsrc = `<html>
<head><title>Washer State</title></head>

<body>
<h1>Currently: {{if .On}}On{{else}}Off{{end}}</h1>
<p>
  Been in this state for {{.StateDuration}}.
  {{if .On}}Reading is currently {{.Reading}}/{{.High}}{{end}}
</p>
</body>
</html>`

var htmltmpl = template.Must(template.New("h").Parse(htmltmplsrc))

func washerJSON(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Content-type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	st := getState()
	json.NewEncoder(w).Encode(st)
}

func washerHTML(w http.ResponseWriter, req *http.Request) {
	st := getState()
	htmltmpl.Execute(w, st)
}

func startWeb(addr string) {
	http.HandleFunc("/washer.json", washerJSON)
	http.HandleFunc("/washer", washerHTML)
	log.Printf("Listening to web requests on %s", addr)
	err := http.ListenAndServe(addr, nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}
