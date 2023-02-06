#include <Arduino.h>
#include "AnalogIn.h"

AnalogIn::AnalogIn(uint8_t pin, bool bipolar)
{
  _pin = pin;
  _bipolar = bipolar;
  _timeConst = 1.0;
  if (_bipolar)
  {
    calibrate();
  }
  else
  {
    _offset = 0;
    _scalePos = 1.0 / 1023;
    _scaleNeg = 0.0;
  }
}

AnalogIn::AnalogIn(uint8_t pin, bool bipolar, float timeConst) : AnalogIn(pin, bipolar)
{
  _timeConst = 1.0 / timeConst;
}

float AnalogIn::value()
{
  int _raw = raw();
  _value = (_timeConst * _raw * (_raw >= 0 ? _scalePos : _scaleNeg)) + (1 - _timeConst) * _value;
  return _value;
}

int AnalogIn::raw()
{
  return analogRead(_pin - _offset);
}

void AnalogIn::calibrate()
{
  long sum = 0;
  for (int i = 0; i < 64; i++)
  {
    sum += analogRead(_pin);
  }
  _offset = (int)(sum / 64);
  _scalePos = 1.0 / (float)(1023 - _offset);
  _scaleNeg = 1.0 / (float)(_offset);
}