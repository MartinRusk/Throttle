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
                   false, false,         // Rudder, Throttle (T1/T2)
                   false, false, false); // Accelerator, Brake, Steering

// datarefs
long refPaused;
long refGroundSpeed;
float refParkingBrakeRatio;
float readParkingBrakeRatio;

// float refPilotsHeadX = -0.25;
// float refPilotsHeadY = 0.67;
// float refPilotsHeadZ = -0.05;
float refPilotsHeadPsi = 0;
float refPilotsHeadThe = 0;
// float refPilotsHeadPhi = 0;

// float savePilotsHeadX;
// float savePilotsHeadY;
// float savePilotsHeadZ;
// float savePilotsHeadPsi;
// float savePilotsHeadThe;
// float savePilotsHeadPhi;

float readPilotsHeadPsi;
float readPilotsHeadThe;
float zeroPilotsHeadPsi;
float zeroPilotsHeadThe;

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

int cmdQuickLook0;
int cmdQuickLook0mem;

Timer tmrMain(1000);
Timer tmrThumbstick(50);
Timer tmrSlider(10);
Timer tmrViewChange(400);

enum modeCamera_t
{
  camStandard,
  camTranslation,
  camRotation,
} modeCamera;

// enum modeMove_t
// {
//   camAbsolute,
//   camIntegral
// } modeMove;

enum modeView_t
{
  viewCockpit,
  viewExtern
} modeView;

enum viewChange_t
{
  viewStable,
  viewSaving,
  viewLoading
} viewChange;

enum modeWarp_t
{
  warpNormal,
  warpFast
} modeWarp;

int classifyValue(float value, int negMax, int negNorm, int negMin, int posMin, int posNorm, int posMax)
{
  if (value < -0.7) return negMax;
  if (value < -0.4) return negNorm;
  if (value < -0.1) return negMin;
  if (value > 0.7) return posMax;
  if (value > 0.4) return posNorm;
  if (value > 0.1) return posMin;
  return -1;
}

// void saveHeadPos()
// {
//   savePilotsHeadX = refPilotsHeadX;
//   savePilotsHeadY = refPilotsHeadY;
//   savePilotsHeadZ = refPilotsHeadZ;
//   savePilotsHeadPsi = refPilotsHeadPsi;
//   savePilotsHeadThe = refPilotsHeadThe;
//   savePilotsHeadPhi = refPilotsHeadPhi;
// }

// void restoreHeadPos()
// {
//   refPilotsHeadX = savePilotsHeadX;
//   refPilotsHeadY = savePilotsHeadY;
//   refPilotsHeadZ = savePilotsHeadZ;
//   refPilotsHeadPsi = savePilotsHeadPsi;
//   refPilotsHeadThe = savePilotsHeadThe;
//   refPilotsHeadPhi = savePilotsHeadPhi;
//   zeroPilotsHeadPsi = refPilotsHeadPsi;
//   zeroPilotsHeadThe = refPilotsHeadThe;
// }

float roundangle(float angle)
{
  return (1000.0 * round(0.001 * angle));
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
  // XP.registerDataRef(F("sim/graphics/view/pilots_head_x"), XPL_READ, 50, 0, &refPilotsHeadX);
  // XP.registerDataRef(F("sim/graphics/view/pilots_head_y"), XPL_READ, 50, 0, &refPilotsHeadY);
  // XP.registerDataRef(F("sim/graphics/view/pilots_head_z"), XPL_READ, 50, 0, &refPilotsHeadZ);
  // XP.registerDataRef(F("sim/graphics/view/pilots_head_phi"), XPL_READ, 50, 0, &refPilotsHeadPhi);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_psi"), XPL_WRITE, 50, 0, &refPilotsHeadPsi);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_the"), XPL_WRITE, 50, 0, &refPilotsHeadThe);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_psi"), XPL_READ, 50, 0, &readPilotsHeadPsi);
  XP.registerDataRef(F("sim/graphics/view/pilots_head_the"), XPL_READ, 50, 0, &readPilotsHeadThe);

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

  // handle view change
  switch (viewChange)
  {
  default:
    if (butView.pressed())
    {
      viewChange = viewSaving;
      tmrViewChange.getTime();
      if (modeView == viewCockpit)
      {
        XP.commandTrigger(cmdQuickLook0mem);
        // saveHeadPos();
      }
      else
      {
        XP.commandTrigger(cmdQuickLook0);
      }
    }
    break;

  case viewSaving:
    if (tmrViewChange.elapsed())
    {
      viewChange = viewLoading;
      if (modeView == viewCockpit)
      {
        XP.commandTrigger(cmdViewChase);
      }
      else
      {
        // restoreHeadPos();
      }
    }
    break;

  case viewLoading:
    if (tmrViewChange.elapsed())
    {
      viewChange = viewStable;
      if (modeView == viewCockpit)
      {
        modeView = viewExtern;
        modeCamera = camTranslation;
      }
      else
      {
        modeView = viewCockpit;
        modeCamera = camStandard;
      }
    }
    break;
  }

  // camera modes
  if (encZoom.pressed())
  {
    if (modeView == viewCockpit)
    {
      if (modeCamera == camTranslation)
      {
        modeCamera = camStandard;
        zeroPilotsHeadPsi = roundangle(readPilotsHeadPsi);
        zeroPilotsHeadThe = roundangle(readPilotsHeadThe);
        // saveHeadPos();
      }
      else
      {
        modeCamera = camTranslation;
      }
    }
  }

  if (butStick.pressed())
  {
    if (modeView == viewCockpit)
    {
      if (modeCamera == camRotation)
      {
        modeCamera = camStandard;
        zeroPilotsHeadPsi = roundangle(readPilotsHeadPsi);
        zeroPilotsHeadThe = roundangle(readPilotsHeadThe);
        // saveHeadPos();
      }
      else
      {
        modeCamera = camRotation;
      }
    }
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
      refPilotsHeadPsi = roundangle(zeroPilotsHeadPsi - (135.0 * stickX.value()));
      refPilotsHeadThe = roundangle(zeroPilotsHeadThe + (60.0 * stickY.value()));
      break;
    case camTranslation:
      XP.commandTrigger(classifyValue(stickX.value(),
                                      cmdRightFast, cmdRight, cmdRightSlow,
                                      cmdLeftSlow, cmdLeft, cmdLeftFast));
      XP.commandTrigger(classifyValue(stickY.value(),
                                      cmdDownFast, cmdDown, cmdDownSlow,
                                      cmdUpSlow, cmdUp, cmdUpFast));
      break;
    case camRotation:
      XP.commandTrigger(classifyValue(stickX.value(),
                                      cmdRotRightFast, cmdRotRight, cmdRotRightSlow,
                                      cmdRotLeftSlow, cmdRotLeft, cmdRotLeftFast));
      XP.commandTrigger(classifyValue(stickY.value(),
                                      cmdRotDownFast, cmdRotDown, cmdRotDownSlow,
                                      cmdRotUpSlow, cmdRotUp, cmdRotUpFast));
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

    // if (modeView == viewCockpit)
    // {
    //   if (modeCamera == camTranslation)
    //   {
    //     if (encZoom.up())
    //     {
    //       XP.commandTrigger(cmdForward);
    //       // refPilotsHeadZ -= 0.01;
    //     }
    //     if (encZoom.down())
    //     {
    //       XP.commandTrigger(cmdBackward);
    //       // refPilotsHeadZ += 0.01;
    //     }
    //     leds.set(LED_CAMERA, ledOn);
    //     refPilotsHeadX -= 0.0005 * stickX.value();
    //     refPilotsHeadY += 0.0005 * stickY.value();
    //   }
    //   else
    //   {
    //     if (modeMove == camAbsolute)
    //     {
    //       leds.set(LED_CAMERA, ledOff);
    //       refPilotsHeadPsi = -135 * stickX.value();
    //       refPilotsHeadThe = 60 * stickY.value();
    //     }
    //     else
    //     {
    //       leds.set(LED_CAMERA, ledMedium);
    //       refPilotsHeadPsi = -135 * stickX.value();
    //       refPilotsHeadThe += 0.5 * stickY.value();
    //     }
    //   }
    // }

  // Joysticks
  if (tmrSlider.elapsed())
  { // sliders
    sliderLeft.handle();
    sliderRight.handle();
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

  // // send cycle time
  // if (tmrMain.elapsed())
  // {
  //   char tmp[16];
  //   sprintf(tmp, " %ld Cycles/s", tmrMain.count());
  //   XP.sendDebugMessage(tmp);
  // }
}
