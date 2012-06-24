package main

import (
	"bytes"
	"testing"
	"time"
)

func TestTemplateOff(t *testing.T) {
	st := Reading{StateDuration: time.Minute}
	b := &bytes.Buffer{}
	err := htmltmpl.Execute(b, st)
	if err != nil {
		t.Fatalf("Error processing template: %v", err)
	}
	t.Logf("%s", b.Bytes())
}

func TestTemplateOn(t *testing.T) {
	st := Reading{On: true, StateDuration: time.Minute}
	b := &bytes.Buffer{}
	err := htmltmpl.Execute(b, st)
	if err != nil {
		t.Fatalf("Error processing template: %v", err)
	}
	t.Logf("%s", b.Bytes())
}
