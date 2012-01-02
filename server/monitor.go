package main

import (
	"bufio"
	"flag"
	"github.com/dustin/rs232.go"
	"io"
	"log"
	"os"
	"time"
)

var mockMode bool
var webBind string

func init() {
	flag.BoolVar(&mockMode, "mockMode", false, "True if we're mocking")
	flag.StringVar(&webBind, "web", ":8123", "Web binding address")
}

func initSourceSerial() io.ReadCloser {
	portString := flag.Args()[0]
	log.Printf("Opening '%s'", portString)
	port, err := rs232.OpenPort(portString, 57600, rs232.S_8N1)
	if err != nil {
		log.Fatalf("Error opening port: %s", err)
	}
	return &port
}

type mockReader struct {
	underlying *os.File
	buf        []byte
}

func (m *mockReader) Close() error {
	return m.underlying.Close()
}

func (m *mockReader) Read(p []byte) (n int, err error) {
	n, err = m.underlying.Read(m.buf)
	if err == nil {
		if p[0] == '\n' {
			time.Sleep(1 * time.Second)
		}
	} else if err == io.EOF {
		m.underlying.Seek(0, os.SEEK_SET)
		n, err = m.underlying.Read(m.buf)
		if err != nil {
			return
		}
	}
	p[0] = m.buf[0]
	return
}

func initSourceMock() io.ReadCloser {
	portString := flag.Args()[0]
	log.Printf("Opening mock '%s'", portString)
	port, err := os.Open(portString)
	if err != nil {
		log.Fatalf("Error opening mock port: %s", err)
	}
	return &mockReader{underlying: port, buf: make([]byte, 1)}
}

func initSource() io.ReadCloser {
	if mockMode {
		return initSourceMock()
	}
	return initSourceSerial()
}

func main() {
	flag.Parse()
	port := initSource()
	defer port.Close()

	if webBind != "" {
		go startWeb(webBind)
	} else {
		log.Printf("No web.")
	}

	r := bufio.NewReader(port)
	for {
		line, _, err := r.ReadLine()
		if err != nil {
			log.Fatalf("Error reading:  %s", err)
		}
		processLine(string(line))
	}
}
