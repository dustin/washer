package main

type tracker struct {
	state Reading

	input chan Reading
	reqs  chan chan Reading
}

var global_tracker tracker

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
	global_tracker.input = make(chan Reading)
	global_tracker.reqs = make(chan chan Reading)

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
