#include <JeeLib.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>

const int THIS_ID(9);
const int LED_PORT(4);
const int POLL_FREQ(1000);
const int XMIT_FREQ(10000);
const int NUM_PORTS(1);

const int LCD_PIN_TX(7);
const int LCD_PIN_RX(4); // Digital on Port 1 (just used for analog)

MilliTimer pollTimer, xmitTimer;
bool pending(false);
unsigned long lastAck(0);

SoftwareSerial lcd(LCD_PIN_RX, LCD_PIN_TX);

typedef struct {
    byte port;
    int reading;
} data_t;

data_t data[NUM_PORTS];
bool shouldSend[NUM_PORTS];

ISR(WDT_vect) { Sleepy::watchdogEvent(); }

const char *spinChars[] = {
    "/", "-", "?0", "|", 0};
int spinPos(0);

void spin(unsigned long x=0) {
    lcd.print(spinChars[spinPos++]);
    lcd.print("?h");
    if (spinChars[spinPos] == 0) {
        spinPos = 0;
    }
    delay(x);
}

void lcdInit() {
    pinMode(LCD_PIN_TX, OUTPUT);
    lcd.begin(9600);

    lcd.print("?G216");  // set display geometry,  2 x 16 characters in this case
    delay(500);

    lcd.print("?B40");    // set backlight to ff hex, maximum brightness
    delay(1000);                // pause to allow LCD EEPROM to program

    // see moderndevice.com for a handy custom char generator (software app)
    lcd.print("?f");                   // clear the LCD
    delay(10);

    lcd.print("?c0");                  // turn cursor off

    // Need a \ for the spinner
    lcd.print("?D00010080402010000");

    lcd.print("[configuring  ]?h?h");
    spin();

    lcd.print("?s6");     // set tabs to six spaces
    spin(1000);               // pause to allow LCD EEPROM to program

    // lcd.print("?D00000000000000000");       // define special characters
    // spin(300);                                  // delay to allow write to EEPROM

    //crashes LCD without delay
    lcd.print("?D11010101010101010");
    spin(300);

    lcd.print("?D21818181818181818");
    spin(300);

    lcd.print("?D31c1c1c1c1c1c1c1c");
    spin(300);

    lcd.print("?D41e1e1e1e1e1e1e1e");
    spin(300);

    lcd.print("?D51f1f1f1f1f1f1f1f");
    spin(300);

    lcd.print("?D60000000000040E1F");
    spin(300);

    lcd.print("?D70000000103070F1F");
    spin(300);

    lcd.print("?f");
}

void setup () {
    lcdInit();
    pollTimer.set(POLL_FREQ);
    xmitTimer.set(XMIT_FREQ);

    // Port initialization
    for (int i = 0; i < NUM_PORTS; ++i) {
        data[i].port = i;
        data[i].reading = 0;
        shouldSend[i] = false;
    }

    pinMode(LED_PORT, OUTPUT);

    Serial.begin(57600);
    rf12_initialize(THIS_ID, RF12_433MHZ, 4);
    Serial.print("Initialized ");
    Serial.println(THIS_ID);
}

bool shouldSendAny() {
    bool rv(false);
    for (int i = 0; i < NUM_PORTS; ++i) {
        rv |= shouldSend[i];
    }
    return rv;
}

void loop () {
    if (rf12_recvDone() && rf12_crc == 0) {
        Serial.println("Got an ACK.");
        lastAck = millis();
    }

    if (pollTimer.poll()) {
        pending = true;

        for (int i = 0; i < NUM_PORTS; ++i) {
            int r(analogRead(data[i].port));
            if (r != data[i].reading) {
                data[i].reading = r;
                shouldSend[i] = true;
            }
            data[i].reading = r;
        }
        lcd.print("?fRead: ");
        lcd.print(data[0].reading);
        lcd.print("?nlast ACK: ");
        lcd.print((millis() - lastAck) / 1000);
        pollTimer.set(POLL_FREQ);
    }

    if (xmitTimer.poll()) {
        for (int i = 0; i < NUM_PORTS; ++i) {
            shouldSend[i] = true;
        }
    }

    if (shouldSendAny()) {

        Serial.print("Transmitting ");
        for (int i = 0; i < NUM_PORTS; ++i) {
            if (shouldSend[i] && rf12_canSend()) {
                Serial.println(data[i].reading);

                pending = false;
                rf12_sendStart(RF12_HDR_ACK, &data[i], sizeof(data[i]));
                shouldSend[i] = false;
            }
        }

        xmitTimer.set(XMIT_FREQ);
    }
}

