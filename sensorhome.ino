#include <JeeLib.h>
#include <avr/sleep.h>

const int THIS_ID(1);
const int LED_PORT(9);
const int MIN_REPORT_FREQ(60000);
const int HIGH_THRESH(10);

/*
 Output:

 - Normal readings:

     < PORT READING HIGH SEQ (ON|OFF) MS_SINCE_CHANGE

 - Info messages

     # text

 - Exceptional messages

     * MS_SINCE_LAST_HEARD

 - Other events:

     + PORT (ON|OFF) MS_IN_PREV_STATE

 */

// The JeeLink LED is backwards.  I don't know why.
#define JEE_LED_ON 0
#define JEE_LED_OFF 1

typedef struct {
    int reading;
    int high;
    byte port;
    byte seq;
} data_t;

static bool shouldSend(false);
static unsigned long lastHeard(0);
static unsigned long lastChange(0);
static bool state(false);

static MilliTimer lastHeardTimer;

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void setup () {
    pinMode(LED_PORT, OUTPUT);
    digitalWrite(LED_PORT, JEE_LED_OFF);

    Serial.begin(57600);
    rf12_initialize(THIS_ID, RF12_433MHZ, 4);
    Serial.print("# Initialized ");
    Serial.println(THIS_ID);

    lastHeardTimer.set(MIN_REPORT_FREQ);
}

static void maybeChangeState(byte port, int reading) {
    bool thisState(reading > HIGH_THRESH);
    if (state != thisState) {
        unsigned long now(millis());
        Serial.print("+ ");
        delay(1);
        Serial.print(port);
        delay(5);
        Serial.print(thisState ? " ON " : " OFF ");
        delay(1);
        Serial.println(now - lastChange);
        lastChange = millis();
    }
    state = thisState;
    digitalWrite(LED_PORT, state ? JEE_LED_ON : JEE_LED_OFF);
}

void loop () {
    data_t data;

    if (rf12_recvDone() && rf12_crc == 0 && rf12_len == sizeof(data)) {

        data = *((data_t*)rf12_data);

        maybeChangeState(data.port, data.reading);

        // < PORT READING (ON|OFF) MS_SINCE_CHANGE
        Serial.print("< ");
        Serial.print(data.port, DEC);
        Serial.print(" ");
        delay(1);
        Serial.print(data.reading, DEC);
        Serial.print(" ");
        delay(1);
        Serial.print(data.high, DEC);
        Serial.print(" ");
        delay(1);
        Serial.print(data.seq, DEC);
        delay(5);
        Serial.print(state ? " ON " : " OFF ");
        delay(5);
        Serial.print(millis() - lastChange, DEC);
        delay(5);
        Serial.println("");

        lastHeardTimer.set(MIN_REPORT_FREQ);
        lastHeard = millis();

        if (RF12_WANTS_ACK) {
            rf12_sendStart(RF12_ACK_REPLY, &data.seq, sizeof(data.seq));
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
        Serial.print("* ");
        delay(1);
        Serial.println(millis() - lastHeard);
    }
}


