#ifndef AnalogIn_h
#define AnalogIn_h

class AnalogIn
{
public:
  AnalogIn(uint8_t pin, bool bipolar, float timeConst);
  AnalogIn(uint8_t pin, bool bipolar);
  float value();
  int raw();
  void calibrate();
private:
  float _timeConst;
  float _value;
  float _scalePos;
  float _scaleNeg;
  int _offset;
  bool _bipolar;
  uint8_t _pin;
};

#endif
