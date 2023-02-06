#include <Arduino.h>
#include <XPLDirect.h>
#include <Joystick.h>
#include <Switch.h>
#include <Button.h>
#include <Encoder.h>
#include <LedShift.h>
#include <SoftTimer.h>
#include <AnalogIn.h>

XPLDirect xp(&Serial);

Switch swMode(0);
Button butPause(1);
Button butWarp(2);
Button butView(3);
Button butBrakeRelease(4);
Button butBrakeSet(5);
Button butFlapsUp(6);
Button butFlapsDown(7);
Button butStick(8);
Encoder encZoom(9, 10, 11, 4);
LedShift leds(16, 15, 14);

#define LED_BRAKE_REL 1
#define LED_BRAKE_SET 2
#define LED_PAUSE 3
#define LED_WARP 4
#define LED_VIEW 5
#define LED_CAMERA 6

AnalogIn stickX(A0, true, 50);
AnalogIn stickY(A1, true, 50);
AnalogIn sliderLeft(A2, false, 10);
AnalogIn sliderRight(A3, false, 10);

// Create Joystick
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
                   JOYSTICK_TYPE_JOYSTICK,
                   0, 0,                 // Buttons, HAT Switches
                   true, true, false,    // X,Y,Z Axes (pos)
                   true, true, false,    // Rx,Ry,Rz Axes (rot)
                   false, false,         // Rudder, Throttle (T1/T2)
                   false, false, false); // Accelerator, Brake, Steering

SoftTimer loopTimer(10);

// datarefs
long int refPaused;
long int refGroundSpeed;
float refParkingBrakeRatio;

float refPilotsHeadX;
float refPilotsHeadY;
float refPilotsHeadZ;
float refPilotsHeadPsi;
float refPilotsHeadThe;
float refPilotsHeadPhi;

// commands
int cmdPause;
int cmdWarp;
int cmdViewDefault;
int cmdViewForwardNothing;
int cmdViewChase;
int cmdSpeedBrakeUp;
int cmdSpeedBrakeDown;
int cmdBackward;
int cmdForward;
int cmdLeft;
int cmdLeftFast;
int cmdLeftSlow;
int cmdRight;
int cmdRightFast;
int cmdRightSlow;
int cmdUp;
int cmdUpFast;
int cmdUpSlow;
int cmdDown;
int cmdDownFast;
int cmdDownSlow;

// view mode
int modeView;
bool modeWarp;

enum modeCamera_t
{
  camRotation,
  camTranslation
} modeCamera;

enum modeMove_t
{
  camAbsolute,
  camIntegral
} modeMove;

void handle()
{
  swMode.handle();
  butPause.handle();
  butWarp.handle();
  butView.handle();
  butBrakeRelease.handle();
  butBrakeSet.handle();
  butFlapsUp.handle();
  butFlapsDown.handle();
  butStick.handle();
  encZoom.handle();
  leds.handle();
}

void setup()
{
  // init led sequence
  leds.set_all(ledOff);
  for (int pin = 0; pin < 7; pin++)
  {
    leds.set(pin, ledOn);
    leds.handle();
    delay(200);
  }
  leds.set_all(ledOff);

  // setup interface
  Serial.begin(XPLDIRECT_BAUDRATE);
  xp.begin("Throttle");

  // register datarefs
  xp.registerDataRef(F("sim/time/paused"), XPL_READ, 100, 0, &refPaused);
  xp.registerDataRef(F("sim/time/ground_speed"), XPL_READWRITE, 100, 0, &refGroundSpeed);
  xp.registerDataRef(F("sim/cockpit2/controls/parking_brake_ratio"), XPL_READWRITE, 100, 0, &refParkingBrakeRatio);
  xp.registerDataRef(F("sim/graphics/view/pilots_head_x"), XPL_READWRITE, 50, 0.01, &refPilotsHeadX);
  xp.registerDataRef(F("sim/graphics/view/pilots_head_y"), XPL_READWRITE, 50, 0.01, &refPilotsHeadY);
  xp.registerDataRef(F("sim/graphics/view/pilots_head_z"), XPL_READWRITE, 50, 0.01, &refPilotsHeadZ);
  xp.registerDataRef(F("sim/graphics/view/pilots_head_phi"), XPL_READWRITE, 50, 1, &refPilotsHeadPhi);
  xp.registerDataRef(F("sim/graphics/view/pilots_head_psi"), XPL_READWRITE, 50, 1, &refPilotsHeadPsi);
  xp.registerDataRef(F("sim/graphics/view/pilots_head_the"), XPL_READWRITE, 50, 1, &refPilotsHeadThe);
  
  // register commands
  cmdPause = xp.registerCommand(F("sim/operation/pause_toggle"));
  cmdWarp = xp.registerCommand(F("sim/operation/ground_speed_change"));
  cmdViewDefault = xp.registerCommand(F("sim/view/default_view"));
  cmdViewForwardNothing = xp.registerCommand(F("sim/view/forward_with_nothing"));
  cmdViewChase = xp.registerCommand(F("sim/view/chase"));
  cmdSpeedBrakeUp = xp.registerCommand(F("sim/flight_controls/speed_brakes_up_one"));
  cmdSpeedBrakeDown = xp.registerCommand(F("sim/flight_controls/speed_brakes_down_one"));

  // setup analog input pins
  for (int i = 18; i < 23; i++)
  {
    pinMode(i, INPUT);
  }

  // Set Range Values
  Joystick.setXAxisRange(0, 4095);
  Joystick.setYAxisRange(0, 4095);
  Joystick.setRxAxisRange(0, 4095);
  Joystick.setRyAxisRange(0, 4095);
  Joystick.begin(false);
}

void loop()
{
  // enforce sample time
  while (!loopTimer.isTick());

  // handle interface
  xp.xloop();

  // handle input devices
  handle();

  // brake release
  if (butBrakeRelease.pressed())
  {
    refParkingBrakeRatio = 0.0;
  }
  // brake set
  if (butBrakeSet.pressed())
  {
    refParkingBrakeRatio = 1.0;
  }
  // pause
  if (butPause.pressed())
  {
    xp.commandTrigger(cmdPause);
  }
  // warp
  if (butWarp.pressed())
  {
    modeWarp = !modeWarp;
    refGroundSpeed = modeWarp ? 16 : 1;
  }
  // handle view
  if (butView.pressed())
  {
    modeView = (modeView + 1) % 3;
    switch (modeView)
    {
    case 1:
      xp.commandTrigger(cmdViewForwardNothing);
      break;
    case 2:
      xp.commandTrigger(cmdViewChase);
      break;
    default:
      xp.commandTrigger(cmdViewDefault);
      break;
    }
  }

  // brake leds
  if (refParkingBrakeRatio > 0.99)
  {
    leds.set(LED_BRAKE_REL, ledOff);
    leds.set(LED_BRAKE_SET, ledOn);
  }
  if (refParkingBrakeRatio == 0.0)
  {
    leds.set(LED_BRAKE_REL, ledOn);
    leds.set(LED_BRAKE_SET, ledOff);
  }

  // pause led
  leds.set(LED_PAUSE, refPaused ? ledMedium : ledOff);
  
  // warp led
  switch (refGroundSpeed)
  {
  default:
    leds.set(LED_WARP, ledOff);
    break;
  case 2:
    leds.set(LED_WARP, ledSlow);
    break;
  case 4:
    leds.set(LED_WARP, ledMedium);
    break;
  case 8:
    leds.set(LED_WARP, ledFast);
    break;
  case 16:
    leds.set(LED_WARP, ledOn);
    break;
  }

  // camera
  if (encZoom.pressed())
  {
    modeCamera = (modeCamera == camRotation) ? camTranslation : camRotation;
  }
  if (butStick.pressed())
  {
    modeMove = (modeMove == camAbsolute) ? camIntegral : camAbsolute;
  }

  if (encZoom.up())
  {
    refPilotsHeadZ += 0.01;
  }
  if (encZoom.down())
  {
    refPilotsHeadZ -= 0.01;
  }

  if (modeCamera == camTranslation)
  {
    leds.set(LED_CAMERA, ledOn);
    refPilotsHeadX -= 0.0001 * stickX.value();
    refPilotsHeadY += 0.0001 * stickY.value();
  }
  else
  {
    if (modeMove == camAbsolute)
    {
      leds.set(LED_CAMERA, ledOff);
      refPilotsHeadPsi = 135 * stickX.value();
      refPilotsHeadThe = 90 * stickY.value();
    }
    else
    {
      leds.set(LED_CAMERA, ledMedium);
      refPilotsHeadPsi = 135 * stickX.value();
      refPilotsHeadThe += 0.05 * stickY.value();
    }
  }

  // throttle
  if (swMode.engaged())
  {
    Joystick.setXAxis(4095 * sliderLeft.value());
    Joystick.setYAxis(4095 * sliderRight.value());
  }
  else
  {
    Joystick.setRxAxis(4095 * sliderLeft.value());
    Joystick.setRyAxis(4095 * sliderRight.value());
  }
  Joystick.sendState();
}