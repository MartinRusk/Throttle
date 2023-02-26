#include <unity.h>
#include <Arduino.h>
#include <AnalogIn.h>  

int input = 512;
class test : public AnalogIn
{
  public:
    test(bool a, float b);
    int raw();
};

test::test(bool a, float b) : AnalogIn(A0, a, b)
{
}

int test::raw()
{
  return input;
}

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_function(void) {
  test inst(true, 1);
  input = 512;

  TEST_ASSERT_EQUAL_INT16(512, inst.raw());
  // float a;
  // for (int i = 0; i<1000; i++)
  // {
  //   a = inst.value();
  // }
  // TEST_ASSERT_EQUAL_FLOAT(0, a);
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_function);
  return UNITY_END();
}

void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  delay(2000);
  runUnityTests();
}
void loop() {}
