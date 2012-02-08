package main

type ApplianceState Reading

type tracker struct {
	state ApplianceState

	input chan ApplianceState
	reqs  chan chan ApplianceState
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
	global_tracker.input = make(chan ApplianceState)
	global_tracker.reqs = make(chan chan ApplianceState)

	go tracker_loop()
}

func saveState(s ApplianceState) {
	global_tracker.input <- s
}

func getState() ApplianceState {
	r := make(chan ApplianceState)
	global_tracker.reqs <- r
	return <-r
}
