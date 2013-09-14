package main

var global_tracker = struct {
	state Reading

	input chan Reading
	reqs  chan chan Reading
}{
	input: make(chan Reading),
	reqs:  make(chan chan Reading),
}

func tracker_loop() {
	for {
		select {
		case i := <-global_tracker.input:
			global_tracker.state = i
		case c := <-global_tracker.reqs:
			c <- global_tracker.state
		}
	}
}

func init() {
	go tracker_loop()
}

func saveState(s Reading) {
	global_tracker.input <- s
}

func getState() Reading {
	r := make(chan Reading)
	global_tracker.reqs <- r
	return <-r
}
