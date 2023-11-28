#include <Arduino.h>
#include <Joystick.h>
#include <XPLDevices.h>

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
                   true, true,           // Rudder, Throttle (T1/T2)
                   false, false, false); // Accelerator, Brake, Steering

// datarefs
long refPaused = 0;
long refGroundSpeed = 1;
float refParkingBrakeRatio = 1.0;
float readParkingBrakeRatio = 0.5;

// commands
int cmdPause;
int cmdWarp;
int cmdViewDefault;
int cmdViewForwardNothing;
int cmdViewChase;
int cmdSpeedBrakeUp;
int cmdSpeedBrakeDown;

int cmdForward;
int cmdBackward;
int cmdForwardFast;
int cmdBackwardFast;
int cmdForwardSlow;
int cmdBackwardSlow;
int cmdZoomIn;
int cmdZoomOut;
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
int cmdRotLeft;
int cmdRotLeftFast;
int cmdRotLeftSlow;
int cmdRotRight;
int cmdRotRightFast;
int cmdRotRightSlow;
int cmdRotUp;
int cmdRotUpFast;
int cmdRotUpSlow;
int cmdRotDown;
int cmdRotDownFast;
int cmdRotDownSlow;
int cmdPopupPFD;
int cmdPopupMFD;

int cmdQuickLook0;
int cmdQuickLook0mem;

Timer tmrThumbstick(50);
Timer tmrSlider(10);

Timer tmrView(1000);
Timer tmrZoom(1000);

enum modeCamera_t
{
  camStandard,
  camTranslation,
  camRotation,
} modeCamera;

enum modeView_t
{
  viewCockpit,
  viewForward,
  viewChase
} modeView;

enum modeWarp_t
{
  warpNormal,
  warpFast
} modeWarp;

int classifyValue(float value, int negMax, int negNorm, int negMin, int posMin, int posNorm, int posMax)
{
  if (value < -0.99) return negMax;
  if (value < -0.6) return negNorm;
  if (value < -0.1) return negMin;
  if (value > 0.99) return posMax;
  if (value > 0.6) return posNorm;
  if (value > 0.1) return posMin;
  return -1;
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
  XP.registerDataRef(F("sim/cockpit2/controls/parking_brake_ratio"), XPL_WRITE, 100, 0, &refParkingBrakeRatio);
  XP.registerDataRef(F("sim/cockpit2/controls/parking_brake_ratio"), XPL_READ, 100, 0, &readParkingBrakeRatio);

  // register commands
  cmdSpeedBrakeUp = XP.registerCommand(F("sim/flight_controls/speed_brakes_up_one"));
  cmdSpeedBrakeDown = XP.registerCommand(F("sim/flight_controls/speed_brakes_down_one"));
  cmdPause = XP.registerCommand(F("sim/operation/pause_toggle"));
  cmdWarp = XP.registerCommand(F("sim/operation/ground_speed_change"));
  cmdViewDefault = XP.registerCommand(F("sim/view/default_view"));
  cmdViewForwardNothing = XP.registerCommand(F("sim/view/forward_with_nothing"));
  cmdViewChase = XP.registerCommand(F("sim/view/chase"));
  cmdQuickLook0 = XP.registerCommand(F("sim/view/quick_look_0"));  
  cmdQuickLook0mem = XP.registerCommand(F("sim/view/quick_look_0_mem"));
  cmdForward= XP.registerCommand(F("sim/general/forward"));
  cmdForwardSlow= XP.registerCommand(F("sim/general/forward_slow"));
  cmdForwardFast= XP.registerCommand(F("sim/general/forward_fast"));
  cmdBackward= XP.registerCommand(F("sim/general/backward"));
  cmdBackwardSlow= XP.registerCommand(F("sim/general/backward_slow"));
  cmdBackwardFast= XP.registerCommand(F("sim/general/backward_fast"));
  cmdZoomIn= XP.registerCommand(F("sim/general/zoom_in"));
  cmdZoomOut= XP.registerCommand(F("sim/general/zoom_out"));
  cmdLeft = XP.registerCommand(F("sim/general/left"));
  cmdLeftSlow = XP.registerCommand(F("sim/general/left_slow"));
  cmdLeftFast = XP.registerCommand(F("sim/general/left_fast"));
  cmdRight = XP.registerCommand(F("sim/general/right"));
  cmdRightSlow = XP.registerCommand(F("sim/general/right_slow"));
  cmdRightFast = XP.registerCommand(F("sim/general/right_fast"));
  cmdDown = XP.registerCommand(F("sim/general/down"));
  cmdDownSlow = XP.registerCommand(F("sim/general/down_slow"));
  cmdDownFast = XP.registerCommand(F("sim/general/down_fast"));
  cmdUp = XP.registerCommand(F("sim/general/up"));
  cmdUpSlow = XP.registerCommand(F("sim/general/up_slow"));
  cmdUpFast = XP.registerCommand(F("sim/general/up_fast"));
  cmdRotLeft = XP.registerCommand(F("sim/general/rot_left"));
  cmdRotLeftSlow = XP.registerCommand(F("sim/general/rot_left_slow"));
  cmdRotLeftFast = XP.registerCommand(F("sim/general/rot_left_fast"));
  cmdRotRight = XP.registerCommand(F("sim/general/rot_right"));
  cmdRotRightSlow = XP.registerCommand(F("sim/general/rot_right_slow"));
  cmdRotRightFast = XP.registerCommand(F("sim/general/rot_right_fast"));
  cmdRotDown = XP.registerCommand(F("sim/general/rot_down"));
  cmdRotDownSlow = XP.registerCommand(F("sim/general/rot_down_slow"));
  cmdRotDownFast = XP.registerCommand(F("sim/general/rot_down_fast"));
  cmdRotUp = XP.registerCommand(F("sim/general/rot_up"));
  cmdRotUpSlow = XP.registerCommand(F("sim/general/rot_up_slow"));
  cmdRotUpFast = XP.registerCommand(F("sim/general/rot_up_fast"));
  cmdPopupPFD =  XP.registerCommand(F("sim/GPS/g1000n1_popup"));
  cmdPopupMFD =  XP.registerCommand(F("sim/GPS/g1000n3_popup"));

  // calibrate stick
  stickX.calibrate();
  stickY.calibrate();

  // Set Range Values
  Joystick.setXAxisRange(-4095, 4095);
  Joystick.setYAxisRange(-4095, 4095);
  Joystick.setRxAxisRange(0, 4095);
  Joystick.setRyAxisRange(0, 4095);
  Joystick.setThrottleRange(0, 4095);
  Joystick.setRudderRange(0, 4095);
  Joystick.begin(false);
}

void loop()
{
  // handle interface
  XP.xloop();

  // handle devices
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

  // speed brake up
  if (butSpeedBrakeUp.pressed())
  {
    XP.commandTrigger(cmdSpeedBrakeUp);
  }
  // spped brake down
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
    modeWarp = (modeWarp == warpNormal) ? warpFast : warpNormal;
    refGroundSpeed = (modeWarp == warpFast) ? 16 : 1;
  }

  // view modes
  if (butView.pressed())
  {
    tmrView.getTime();
  }

  if (butView.released())
  {
    if (tmrView.elapsed() && (modeView == viewCockpit))
    {
      XP.commandTrigger(cmdQuickLook0);
      XP.commandTrigger(cmdPopupPFD);
      XP.commandTrigger(cmdPopupMFD);
    }
    else
    {
      switch (modeView)
      {
      case viewCockpit:
        modeView = viewChase;
        XP.commandTrigger(cmdQuickLook0mem);
        XP.commandTrigger(cmdViewChase);
        break;
      default:
        modeView = viewCockpit;
        XP.commandTrigger(cmdViewDefault);
        XP.commandTrigger(cmdQuickLook0);
      }
    }
  }

  // camera modes
  if (encZoom.pressed())
  {
    tmrZoom.getTime();
  }

  if (encZoom.released())
  {
    switch (modeCamera)
    {
      case camStandard:
        if (tmrZoom.elapsed())
        {
          modeCamera = camRotation;
        }
        else
        {
          modeCamera = camTranslation;
        }
        break;
      default:
        modeCamera = camStandard;
    }
  }

  if (butStick.pressed())
  {
    if (modeView == viewCockpit)
    {
      XP.commandTrigger(cmdQuickLook0);
    }
    modeCamera = camStandard;
  }

  if (encZoom.up())
  {
    XP.commandTrigger(modeCamera == camTranslation ? cmdForward : cmdZoomIn);
  }
  if (encZoom.down())
  {
    XP.commandTrigger(modeCamera == camTranslation ? cmdBackward : cmdZoomOut);
  }

  // rate limitation for analog values
  if (tmrThumbstick.elapsed())
  {
    stickX.handle();
    stickY.handle();
    switch (modeCamera)
    {
    default:
      Joystick.setXAxis(4095 * stickX.value());
      Joystick.setYAxis(4095 * stickY.value());
      break;
    case camTranslation:
      XP.commandTrigger(classifyValue(stickX.value(),
                                      cmdRightFast, cmdRight, cmdRightSlow,
                                      cmdLeftSlow, cmdLeft, cmdLeftFast));
      XP.commandTrigger(classifyValue(stickY.value(),
                                      cmdDownFast, cmdDown, cmdDownSlow,
                                      cmdUpSlow, cmdUp, cmdUpFast));
      Joystick.setXAxis(0);
      Joystick.setYAxis(0);
      break;
    case camRotation:
      XP.commandTrigger(classifyValue(stickX.value(),
                                      cmdRotRightFast, cmdRotRight, cmdRotRightSlow,
                                      cmdRotLeftSlow, cmdRotLeft, cmdRotLeftFast));
      XP.commandTrigger(classifyValue(stickY.value(),
                                      cmdRotDownFast, cmdRotDown, cmdRotDownSlow,
                                      cmdRotUpSlow, cmdRotUp, cmdRotUpFast));
      Joystick.setXAxis(0);
      Joystick.setYAxis(0);
      break;
    }
  }

  // Set LEDs
  leds.set(LED_BRAKE_REL, readParkingBrakeRatio == 0.0 ? ledOn : ledOff);
  leds.set(LED_BRAKE_SET, readParkingBrakeRatio > 0.99 ? ledOn : ledOff);
  leds.set(LED_PAUSE, refPaused ? ledMedium : ledOff);
  leds.set(LED_VIEW, modeView == viewCockpit ? ledOff : ledOn);
  leds.set(LED_CAMERA, modeCamera == camTranslation ? ledOn : modeCamera == camRotation ? ledMedium : ledOff);
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

  // Joysticks
  if (tmrSlider.elapsed())
  { // sliders
    sliderLeft.handle();
    sliderRight.handle();
    if (swMode.on())
    {
      Joystick.setRxAxis(4095 * sliderLeft.value());
      Joystick.setRyAxis(4095 * sliderRight.value());
    }
    else
    {
      Joystick.setRudder(4095 * sliderLeft.value());
      Joystick.setThrottle(4095 * sliderRight.value());
    }
    Joystick.sendState();
  }
}
