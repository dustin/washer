package main

import (
	"encoding/json"
	"log"
	"strconv"
	"strings"
	"time"
)

type Reading struct {
	Port          int
	Reading       int
	High          int
	Seq           int
	On            bool
	StateDuration time.Duration
}

func (r Reading) MarshalJSON() ([]byte, error) {
	m := map[string]interface{}{
		"port":              r.Port,
		"reading":           r.Reading,
		"high":              r.High,
		"on":                r.On,
		"state_duration_ms": int64(r.StateDuration / time.Millisecond),
		"state_duration":    r.StateDuration.String(),
	}
	return json.Marshal(m)
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
	r := Reading{
		Port:          int(parseInt(parts[1], 32)),
		Reading:       int(parseInt(parts[2], 32)),
		High:          int(parseInt(parts[3], 32)),
		Seq:           int(parseInt(parts[4], 32)),
		On:            parts[5] == "ON",
		StateDuration: time.Duration(1e6 * parseInt(parts[6], 64))}

	saveState(r)
	log.Printf("Read %d/%d (%s), from %d (%d).  In that state %s",
		r.Reading, r.High, parts[5], r.Port, r.Seq,
		r.StateDuration.String())

}

func processSwitchEvent(line string) {
	parts := strings.Split(line, " ")
	port := int(parseInt(parts[1], 32))
	// on := parts[2] == "ON"
	d := time.Duration(1e6 * parseInt(parts[3], 64))
	log.Printf("Changed %d to %s after %s", port, parts[2], d.String())
	notifySwitch(port, parts[2], d)
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
	if len(line) < 1 {
		return
	}
	switch line[0] {
	default:
		log.Printf("UNHANDLED:  %s", line)
	case '<':
		processReading(line)
	case '#':
		processInfo(line)
	case '*':
		processException(line)
	case '+':
		processSwitchEvent(line)
	}
}
