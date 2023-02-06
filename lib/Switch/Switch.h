#ifndef Switch_h
#define Switch_h

class Switch
{
public:
  Switch(uint8_t pin);
  void handle();
  bool engaged();

private:
  uint8_t _pin;
  uint8_t _state;
};

#endif
