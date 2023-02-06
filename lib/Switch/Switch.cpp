#include <Arduino.h>
#include "Switch.h"

#define DEBOUNCE_DELAY 100;

// Buttons
Switch::Switch(uint8_t pin)
{
  _pin = pin;
  _state = 0;
  pinMode(_pin, INPUT_PULLUP);
}

void Switch::handle()
{
  if (!digitalRead(_pin))
  {
    if (_state == 0)
    {
      _state = DEBOUNCE_DELAY;
    }
  }
  else if (_state > 0)
  {
    _state--;
  }
}

bool Switch::engaged()
{
  return _state > 0;
}
