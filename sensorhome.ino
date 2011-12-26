#include <JeeLib.h>
#include <avr/sleep.h>

const int THIS_ID(1);
const int LED_PORT(9);
const int MIN_REPORT_FREQ(60000);
const int HIGH_THRESH(10);

// The JeeLink LED is backwards.  I don't know why.
#define JEE_LED_ON 0
#define JEE_LED_OFF 1

typedef struct {
    byte port;
    int reading;
} data_t;

bool shouldSend(false);
unsigned long lastHeard(0);
unsigned long lastChange(0);
bool state(false);

MilliTimer lastHeardTimer;

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void setup () {
    pinMode(LED_PORT, OUTPUT);
    digitalWrite(LED_PORT, JEE_LED_OFF);

    Serial.begin(57600);
    rf12_initialize(THIS_ID, RF12_433MHZ, 4);
    Serial.print("Initialized ");
    Serial.println(THIS_ID);

    lastHeardTimer.set(MIN_REPORT_FREQ);
}

void maybeChangeState(int r) {
    bool thisState(r > HIGH_THRESH);
    if (state != thisState) {
        lastChange = millis();
    }
    state = thisState;
    digitalWrite(LED_PORT, state ? JEE_LED_ON : JEE_LED_OFF);
}

void loop () {
    data_t data;

    if (rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof(data)) {

        data = *((data_t*)rf12_data);

        maybeChangeState(data.reading);

        Serial.print("Read ");
        delay(10);
        Serial.print(data.reading, DEC);
        delay(10);
        Serial.print(" from port ");
        delay(10);
        Serial.print(data.port, DEC);
        delay(10);
        Serial.print(". last state change ");
        delay(10);
        Serial.print(millis() - lastChange);
        delay(10);
        Serial.println("ms ago");

        lastHeardTimer.set(MIN_REPORT_FREQ);
        lastHeard = millis();

        if (RF12_WANTS_ACK) {
            rf12_sendStart(RF12_ACK_REPLY, 0, 0);
            rf12_sendWait(1); // don't power down too soon
        }

        // power down for 2 seconds (multiple of 16 ms)
        rf12_sleep(RF12_SLEEP);
        Sleepy::loseSomeTime(2000);
        rf12_sleep(RF12_WAKEUP);
    } else {
        // switch into idle mode until the next interrupt
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_mode();
    }

    if (lastHeardTimer.poll()) {
        lastHeardTimer.set(MIN_REPORT_FREQ);
        Serial.print("No message in ");
        delay(10);
        Serial.print((millis() - lastHeard) / 1000);
        delay(10);
        Serial.println(" seconds");
    }
}


