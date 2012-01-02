package main

import (
	"encoding/json"
	"log"
	"net/http"
	"time"
)

func WasherServer(w http.ResponseWriter, req *http.Request) {
	st := getState()
	st.StateDuration = st.StateDuration / time.Millisecond
	json.NewEncoder(w).Encode(st)
}

func startWeb(addr string) {
	http.HandleFunc("/washer", WasherServer)
	log.Printf("Listening to web requests on %s", addr)
	err := http.ListenAndServe(addr, nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}
