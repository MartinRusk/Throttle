#include <Arduino.h>
#include "Joystick.h"

// configuration
#define VERSION "3.1.2"
#define BOARD_ID "0100"

// printout debug data
#define DEBUG 0

// hardware setup
#define NUM_LEDS 6
#define DM13A_DAI 16
#define DM13A_DCK 15
#define DM13A_LAT 14
#define MAX_BUTTONS 5  // maximum number of buttons
#define MAX_SWITCHES 5 // maximum number of switches
#define MAX_ENCODERS 1 // maximum number of encoders

// delay for keep alive message [ms]
#define KEEPALIVE_DELAY 500
// delay for key repeat [ms]
#define REPEAT_DELAY 250
// delay for key debouncing [cycles]
#define DEBOUNCE_DELAY 50

// storage for input devices
struct button_t
{
    uint8_t _state;
} Buttons[MAX_BUTTONS];

struct switch_t
{
    uint8_t _state;
} Switches[MAX_SWITCHES];

struct encoder_t
{
    uint8_t _state;
    int8_t _count;
} Encoders[MAX_ENCODERS];

enum repeat_t
{
    single,
    repeat
};

// Create Joystick
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
                   JOYSTICK_TYPE_JOYSTICK,
                   0, 0,                // Buttons, HAT Switches
                   true, true, false,    // X,Y,Z Axes
                   true, true, false,    // Rx,Ry,Rz Axes
                   false, false,         // Rudder, Throttle
                   false, false, false); // Accelerator, Brake, Steering

// keep alive timer
uint32_t tmr_next = 0;
// repeat timer
uint32_t tmr_rep = 0;

#if DEBUG
// counter to check runtime behavior
uint16_t count = 0;
bool init_mark = false;
#endif

// Buttons
void initButton(button_t *but)
{
    but->_state = 0;
}
void handleButton(button_t *but, const char *name, repeat_t rep, bool input)
{
    if (input)
    {
        if (but->_state == 0)
        {
            Serial.write(name);
            Serial.write("=1\n");
            but->_state = DEBOUNCE_DELAY;
            if (rep == repeat)
            {
                tmr_rep = millis() + REPEAT_DELAY;
            }
        }
        if (rep == repeat)
        {
            if (millis() > tmr_rep)
            {
                Serial.write(name);
                Serial.write("=1\n");
                tmr_rep += REPEAT_DELAY;
            }
        }
    }
    else if (but->_state > 0)
    {
        if (--but->_state == 0)
        {
            Serial.write(name);
            Serial.write("=0\n");
        }
    }
}

// Switches
void initSwitch(switch_t *swi)
{
    swi->_state = 0;
}
void handleSwitch(switch_t *swi, const char *name, bool input)
{
    if (input && (swi->_state == 0))
    {
        Serial.write(name);
        Serial.write(".SW.ON\n");
        swi->_state = DEBOUNCE_DELAY;
    }
    if (!input && (swi->_state > 0))
    {
        if (--swi->_state == 0)
        {
            Serial.write(name);
            Serial.write(".SW.OFF\n");
        }
    }
}

// Encoders
void initEncoder(encoder_t *enc)
{
    enc->_count = 0;
    enc->_state = 0;
}
void handleEncoder(encoder_t *enc, const char *up, const char *dn, bool input1, bool input2, uint8_t pulses)
{
    // collect new state
    enc->_state = ((enc->_state & 0x03) << 2) | (input2 << 1) | input1;
    // evaluate state change
    switch (enc->_state)
    {
    case 0:
    case 5:
    case 10:
    case 15:
        break;
    case 1:
    case 7:
    case 8:
    case 14:
        enc->_count++;
        break;
    case 2:
    case 4:
    case 11:
    case 13:
        enc->_count--;
        break;
    case 3:
    case 12:
        enc->_count += 2;
        break;
    default:
        enc->_count -= 2;
        break;
    }
    // evaluate counter with individual pulses per detent
    if (enc->_count >= pulses)
    {
        Serial.write(up);
        Serial.write("\n");
        enc->_count -= pulses;
    }
    if (enc->_count <= -pulses)
    {
        Serial.write(dn);
        Serial.write("\n");
        enc->_count += pulses;
    }
}

// LEDs
void writeLEDs(uint16_t leds)
{
    shiftOut(DM13A_DAI, DM13A_DCK, MSBFIRST, (leds & 0xFF00) >> 8);
    shiftOut(DM13A_DAI, DM13A_DCK, MSBFIRST, (leds & 0x00FF));
    digitalWrite(DM13A_LAT, HIGH);
    digitalWrite(DM13A_LAT, LOW);
}
void setupLEDs()
{
    pinMode(DM13A_LAT, OUTPUT);
    pinMode(DM13A_DCK, OUTPUT);
    pinMode(DM13A_DAI, OUTPUT);
    digitalWrite(DM13A_LAT, LOW);
    digitalWrite(DM13A_DAI, LOW);
    // init loop thru all LEDs
    writeLEDs(0xFFFF);
    delay(500);
    for (uint8_t i = 0; i < NUM_LEDS; ++i) 
    {
        writeLEDs(1 << (i + 1)); // LED0 not connected
        delay(100);
    }
    writeLEDs(0x0000);
}
void handleLEDs()
{
    if (Serial.available() > NUM_LEDS)
    {   // full dataword available from RSG driver
        // capture full databurst and parse through it
        String leddata = Serial.readStringUntil('\n');
        uint16_t leds = 0;
        for (uint8_t i = 0; i < NUM_LEDS; ++i)
        {
            if (leddata.charAt(i + 1) == '1')
            {
                leds |= (1 << (i + 1)); // LED0 not connected
            }
        }
        writeLEDs(leds);
    }
}

void setup()
{
    // setup interface
    Serial.begin(115200);

    // setup LED driver
    setupLEDs();

    // initialize data structures for input devices
    for (uint8_t but = 0; but < MAX_BUTTONS; but++)
    {
        initButton(&Buttons[but]);
    }
    for (uint8_t swi = 0; swi < MAX_SWITCHES; swi++)
    {
        initSwitch(&Switches[swi]);
    }
    for (uint8_t enc = 0; enc < MAX_ENCODERS; enc++)
    {
        initEncoder(&Encoders[enc]);
    }

    // setup digital input pins
    for (int i = 0; i < 13; i++)
    {
        pinMode(i, INPUT_PULLUP);
    }

    // setup analog input pins
    for (int i = 18; i < 23; i++)
    {
        pinMode(i, INPUT);
    }

    // Set Range Values
    Joystick.setXAxisRange(0, 1023);
    Joystick.setYAxisRange(0, 1023);
    Joystick.setRxAxisRange(0, 1023);
    Joystick.setRyAxisRange(0, 1023);
    // Initialize Joystick, disable AutoSendState
    Joystick.begin(false);
}

void loop()
{
    // keep alive for RSG connection
    if (millis() >= tmr_next)
    { // timer interval for keepalive
        Serial.write("####RealSimGear#mrusk-G1000XFD2#1#");
        Serial.write(VERSION);
        Serial.write("#");
        Serial.write(BOARD_ID);
        Serial.write("\n");
#if DEBUG
        // print out number of cycles per 500ms to verify runtime
        Serial.write("Loop count: ");
        Serial.println(count);
        count = 0;
#endif
        // 500ms keep alive cycle
        tmr_next += KEEPALIVE_DELAY;
    }

#if DEBUG
    count++;
#endif

    // handle incoming LED data
    handleLEDs();

    // avoid compiler warning if a device type is not used
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    uint8_t btn = 0;
    uint8_t swi = 0;
    uint8_t enc = 0;
#pragma GCC diagnostic pop

    // handle devices
    handleSwitch(&Switches[swi++], "SW_BRAKE_SET", !digitalRead(4));
    handleSwitch(&Switches[swi++], "SW_BRAKE_RELEASE", !digitalRead(5));
    handleSwitch(&Switches[swi++], "SW_NN1", !digitalRead(6));
    handleSwitch(&Switches[swi++], "SW_NN2", !digitalRead(7));
    handleButton(&Buttons[btn++], "BTN_PAUSE", single, !digitalRead(1));
    handleButton(&Buttons[btn++], "BTN_WARP", single, !digitalRead(2));
    handleButton(&Buttons[btn++], "BTN_VIEW", single, !digitalRead(3));
    handleSwitch(&Switches[swi++], "SW_NN3", !digitalRead(0));
    handleEncoder(&Encoders[enc++], "ENC_ZOOM_IN", "ENC_ZOOM_OUT", !digitalRead(9), !digitalRead(10), 4);
    handleButton(&Buttons[btn++], "BTN_ZOOM_PUSH", single, !digitalRead(11));
    handleButton(&Buttons[btn++], "BTN_PAN_PUSH", single, !digitalRead(8));

#if DEBUG
    // show number of used input devices and halt program in case of error since memory is corrupted
    if (!init_mark)
    {
        init_mark = true;
        Serial.print("Buttons:  ");
        Serial.println(btn);
        if (btn > MAX_BUTTONS)
        {
            Serial.println("ERROR: Too many buttons used");
            while (true)
                ;
        }
        Serial.print("Switches: ");
        Serial.println(swi);
        if (swi > MAX_SWITCHES)
        {
            Serial.println("ERROR: Too many Switches used");
            while (true)
                ;
        }
        Serial.print("Encoders: ");
        Serial.println(enc);
        if (enc > MAX_ENCODERS)
        {
            Serial.println("ERROR: Too many encoders used");
            while (true)
                ;
        }
    }
#endif


    // handle joystick
    Joystick.setXAxis(1023-analogRead(A0));
    Joystick.setYAxis(1023-analogRead(A1));
    Joystick.setRxAxis(analogRead(A3));
    Joystick.setRyAxis(analogRead(A2));
    Joystick.sendState();
}