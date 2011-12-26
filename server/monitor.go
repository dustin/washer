package main

import (
	"bufio"
	"flag"
	"github.com/dustin/rs232.go"
	"log"
	"strconv"
	"strings"
	"time"
)

type Reading struct {
	Port int
	Reading int
	On bool
	StateDuration time.Duration
}

func parseInt(s string, size int) int64 {
	v, err := strconv.ParseInt(s, 10, size)
	if err != nil {
		log.Fatalf("Error converting %s to an int: %s", s, err)
	}
	return v
}

func processReading(line string) {
	parts := strings.Split(line, " ")
	var r Reading
	r.Port = int(parseInt(parts[1], 32))
	r.Reading = int(parseInt(parts[2], 32))
	r.On = parts[3] == "ON"
	r.StateDuration = time.Duration(1e6 * parseInt(parts[4], 64))
	log.Printf("Read %d (%s) from %d.  In that state %s",
		r.Reading, parts[3], r.Port, r.StateDuration.String())

}

func processSwitchEvent(line string) {
	parts := strings.Split(line, " ")
	port := int(parseInt(parts[1], 32))
	// on := parts[2] == "ON"
	log.Printf("Changed %d to %s", port, parts[2])
}

func processInfo(line string) {
	log.Printf("INFO:  %s", line[2:])
}

func processException(line string) {
	d := time.Duration(1e6 * parseInt(line[2:], 64))
	log.Printf("WARN:  No message in %s", d.String())
}

func processLine(line string) {
	// log.Printf("<: %s", line)
	switch line[0] {
	default: log.Printf("UNHANDLED:  %s", line)
	case '<': processReading(line)
	case '#': processInfo(line)
	case '*': processException(line)
	case '+': processSwitchEvent(line)
	}
}

func main() {
	flag.Parse()
	portString := flag.Args()[0]
	log.Printf("Opening '%s'", portString)
	port, err := rs232.OpenPort(portString, 57600, rs232.S_8N1)
	if err != nil {
		log.Fatalf("Error opening port: %s", err)
	}
	defer port.Close()

	r := bufio.NewReader(&port)
	for {
		line, _, err := r.ReadLine()
		if err != nil {
			log.Fatalf("Error reading:  %s", err)
		}
		processLine(string(line))
	}
}
