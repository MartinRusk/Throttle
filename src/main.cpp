#include <Arduino.h>
#include <Joystick.h>
#include <XPLDevices.h>

Timer loopTimer(10);

Switch swMode(0);
Button butPause(1);
Button butWarp(2);
Button butView(3);
Button butBrakeRelease(4);
Button butBrakeSet(5);
RepeatButton butSpeedBrakeUp(6, 400);
RepeatButton butSpeedBrakeDown(7, 400);
Button butStick(8);
Encoder encZoom(9, 10, 11, enc4Pulse);
LedShift leds(16, 15, 14);

#define LED_BRAKE_REL 1
#define LED_BRAKE_SET 2
#define LED_PAUSE 3
#define LED_WARP 4
#define LED_VIEW 5
#define LED_CAMERA 6

AnalogIn stickX(A0, bipolar, 5);
AnalogIn stickY(A1, bipolar, 5);
AnalogIn sliderLeft(A2, unipolar, 10);
AnalogIn sliderRight(A3, unipolar, 10);

// Create Joystick
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
                   JOYSTICK_TYPE_JOYSTICK,
                   0, 0,                 // Buttons, HAT Switches
                   true, true, false,    // X,Y,Z Axes (pos)
                   true, true, false,    // Rx,Ry,Rz Axes (rot)
                   false, false,         // Rudder, Throttle (T1/T2)
                   false, false, false); // Accelerator, Brake, Steering

// datarefs
long int refPaused;
long int refGroundSpeed;
float refParkingBrakeRatio;

float refPilotsHeadX = -0.25;
float refPilotsHeadY = 0.67;
float refPilotsHeadZ = -0.05;
float refPilotsHeadPsi = 0;
float refPilotsHeadThe = 0;
float refPilotsHeadPhi = 0;

float savePilotsHeadX;
float savePilotsHeadY;
float savePilotsHeadZ;
float savePilotsHeadPsi;
float savePilotsHeadThe;
float savePilotsHeadPhi;

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
  butSpeedBrakeUp.handle();
  butSpeedBrakeDown.handle();
  butStick.handle();
  encZoom.handle();
  leds.handle();
}

void saveHeadPos()
{
  savePilotsHeadX = refPilotsHeadX;
  savePilotsHeadY = refPilotsHeadY;
  savePilotsHeadZ = refPilotsHeadZ;
  savePilotsHeadPsi = refPilotsHeadPsi;
  savePilotsHeadThe = refPilotsHeadThe;
  savePilotsHeadPhi = refPilotsHeadPhi;
}

void restoreHeadPos()
{
  refPilotsHeadX = savePilotsHeadX;
  refPilotsHeadY = savePilotsHeadY;
  refPilotsHeadZ = savePilotsHeadZ;
  refPilotsHeadPsi = savePilotsHeadPsi;
  refPilotsHeadThe = savePilotsHeadThe;
  refPilotsHeadPhi = savePilotsHeadPhi;
}

void setup()
{
  // init led sequence
  leds.set_all(ledOff);
  for (int pin = 0; pin < 7; pin++)
  {
    leds.set(pin, ledOn);
    leds.handle();
    delay(100);
  }
  leds.set_all(ledOff);

  // setup interface
  Serial.begin(XPLDIRECT_BAUDRATE);
  XP.begin("Throttle");

  // register datarefs
  XP.registerDataRef(F("sim/time/paused"), XPL_READ, 100, 0, &refPaused);
  XP.registerDataRef(F("sim/time/ground_speed"), XPL_READWRITE, 100, 0, &refGroundSpeed);
  XP.registerDataRef(F("sim/cockpit2/controls/parking_brake_ratio"), XPL_READWRITE, 100, 0, &refParkingBrakeRatio);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_x"), XPL_READWRITE, 50, 0, &refPilotsHeadX);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_y"), XPL_READWRITE, 50, 0, &refPilotsHeadY);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_z"), XPL_READWRITE, 50, 0, &refPilotsHeadZ);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_phi"), XPL_READWRITE, 50, 0, &refPilotsHeadPhi);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_psi"), XPL_READWRITE, 50, 0, &refPilotsHeadPsi);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_the"), XPL_READWRITE, 50, 0, &refPilotsHeadThe);

  // register commands
  cmdPause = XP.registerCommand(F("sim/operation/pause_toggle"));
  cmdWarp = XP.registerCommand(F("sim/operation/ground_speed_change"));
  cmdViewDefault = XP.registerCommand(F("sim/view/default_view"));
  cmdViewForwardNothing = XP.registerCommand(F("sim/view/forward_with_nothing"));
  cmdViewChase = XP.registerCommand(F("sim/view/chase"));
  cmdSpeedBrakeUp = XP.registerCommand(F("sim/flight_controls/speed_brakes_up_one"));
  cmdSpeedBrakeDown = XP.registerCommand(F("sim/flight_controls/speed_brakes_down_one"));

  // calibrate stick
  stickX.calibrate();
  stickY.calibrate();

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
  while (!loopTimer.isTick())
  {
    // handle input devices
    handle();
  }

  // Serial.print("L: ");
  // Serial.print(sliderLeft.value());
  // Serial.print(" R: ");
  // Serial.print(sliderRight.value());
  // Serial.print(" X: ");
  // Serial.print(stickX.value());
  // Serial.print(" Y: ");
  // Serial.print(stickY.value());
  // Serial.println();

  // handle interface
  XP.xloop();

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
  // brake release
  if (butSpeedBrakeUp.pressed())
  {
    XP.commandTrigger(cmdSpeedBrakeUp);
  }
  // brake set
  if (butSpeedBrakeDown.pressed())
  {
    XP.commandTrigger(cmdSpeedBrakeDown);
  }
  // pause
  if (butPause.pressed())
  {
    XP.commandTrigger(cmdPause);
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
    // case 1:
    //   restoreHeadPos();
    //   break;
    case 1:
      saveHeadPos();
      XP.commandTrigger(cmdViewForwardNothing);
      break;
    case 2:
      XP.commandTrigger(cmdViewChase);
      break;
    default:
      XP.commandTrigger(cmdViewDefault);
      restoreHeadPos();
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

  if (modeView == 0)
  {
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
      refPilotsHeadZ -= 0.01;
    }
    if (encZoom.down())
    {
      refPilotsHeadZ += 0.01;
    }

    if (modeCamera == camTranslation)
    {
      leds.set(LED_CAMERA, ledOn);
      refPilotsHeadX -= 0.0005 * stickX.value();
      refPilotsHeadY += 0.0005 * stickY.value();
    }
    else
    {
      if (modeMove == camAbsolute)
      {
        leds.set(LED_CAMERA, ledOff);
        refPilotsHeadPsi = -135 * stickX.value();
        refPilotsHeadThe = 60 * stickY.value();
      }
      else
      {
        leds.set(LED_CAMERA, ledMedium);
        refPilotsHeadPsi = -135 * stickX.value();
        refPilotsHeadThe += 0.5 * stickY.value();
      }
    }
  }
  // sliders
  if (swMode.on())
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
