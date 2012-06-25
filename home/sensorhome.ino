#include <JeeLib.h>
#include <avr/sleep.h>

const int THIS_ID(1);
const int LED_PORT(9);
const int MIN_REPORT_FREQ(60000);
const int HIGH_THRESH(50);
// How long the light remains in "blink" state between on and off
const unsigned long LIGHT_BLINK_TIMEOUT(5l * 60l * 1000l);
const int LIGHT_BLINK_FREQ_ON(250);
const int LIGHT_BLINK_FREQ_OFF(750);

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

enum light_state {
    LS_ON, LS_BLINKING, LS_OFF
};

typedef struct {
    int reading;
    int high;
    byte port;
    byte seq;
} data_t;

static bool shouldSend(false);
static unsigned long lastHeard(0);
static unsigned long lastChange(0);
static bool state(true);
static bool lightLit(false);
static enum light_state lightState(LS_ON);
static unsigned long offAfter(0);

static MilliTimer lastHeardTimer;
static MilliTimer lightBlinkTimer;

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

void setup () {
    pinMode(LED_PORT, OUTPUT);
    digitalWrite(LED_PORT, JEE_LED_OFF);

    Serial.begin(57600);
    rf12_initialize(THIS_ID, RF12_433MHZ, 4);
    Serial.print("# Initialized ");
    Serial.print(THIS_ID);
    Serial.print(" with threshold of ");
    Serial.println(HIGH_THRESH);

    lastHeardTimer.set(MIN_REPORT_FREQ);
}

static void maybeChangeState(byte port, int reading) {
    bool thisState(reading > HIGH_THRESH);
    if (state != thisState) {
        unsigned long now(millis());
        Serial.print("+ ");
        Serial.flush();
        Serial.print(port);
        Serial.flush();
        Serial.print(thisState ? " ON " : " OFF ");
        Serial.flush();
        Serial.println(now - lastChange);
        lastChange = millis();

        if (!thisState) {
            Serial.println("# light_state -> blinking");
            offAfter = millis() + LIGHT_BLINK_TIMEOUT;
            lightBlinkTimer.set(LIGHT_BLINK_FREQ_ON);
            lightState = LS_BLINKING;
        } else {
            Serial.println("# light_state -> on");
            lightState = LS_ON;
        }
    }
    state = thisState;
}

static void handleLED() {
    switch(lightState) {
    case LS_OFF:
        digitalWrite(LED_PORT, JEE_LED_OFF);
        break;
    case LS_ON:
        digitalWrite(LED_PORT, JEE_LED_ON);
        break;
    case LS_BLINKING:
        if (millis() > offAfter) {
            Serial.println("# light_state -> off");
            lightState = LS_OFF;
        } else if (lightBlinkTimer.poll()) {
            if (lightLit) {
                digitalWrite(LED_PORT, JEE_LED_OFF);
                lightBlinkTimer.set(LIGHT_BLINK_FREQ_OFF);
                lightLit = false;
            } else {
                digitalWrite(LED_PORT, JEE_LED_ON);
                lightBlinkTimer.set(LIGHT_BLINK_FREQ_ON);
                lightLit = true;
            }
        }
    }
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
        Serial.flush();
        Serial.print(data.reading, DEC);
        Serial.print(" ");
        Serial.flush();
        Serial.print(data.high, DEC);
        Serial.print(" ");
        Serial.flush();
        Serial.print(data.seq, DEC);
        Serial.flush();
        Serial.print(state ? " ON " : " OFF ");
        Serial.flush();
        Serial.print(millis() - lastChange, DEC);
        Serial.flush();
        Serial.println("");

        lastHeardTimer.set(MIN_REPORT_FREQ);
        lastHeard = millis();

        if (RF12_WANTS_ACK) {
            rf12_sendStart(RF12_ACK_REPLY, &data.seq, sizeof(data.seq));
            rf12_sendWait(1); // don't power down too soon
        }

        // power down for 2 seconds (multiple of 16 ms)
        if (lightState != LS_BLINKING) {
            rf12_sleep(RF12_SLEEP);
            Sleepy::loseSomeTime(2000);
            rf12_sleep(RF12_WAKEUP);
        }
    } else {
        // switch into idle mode until the next interrupt
        if (lightState != LS_BLINKING) {
            set_sleep_mode(SLEEP_MODE_IDLE);
            sleep_mode();
        }
    }

    handleLED();

    if (lastHeardTimer.poll()) {
        lastHeardTimer.set(MIN_REPORT_FREQ);
        Serial.print("* ");
        Serial.flush();
        Serial.println(millis() - lastHeard);
    }
}
