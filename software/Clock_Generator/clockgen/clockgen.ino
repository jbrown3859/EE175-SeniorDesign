#include <Adafruit_SI5351.h>

Adafruit_SI5351 clockgen = Adafruit_SI5351();

void setup(void) {
  Serial.begin(9600);
  Serial.println("Si5351 Clockgen Test"); Serial.println("");

  /* Initialise the sensor */
  if (clockgen.begin() != ERROR_NONE) {
    /* There was a problem detecting the IC ... check your connections */
    Serial.print("Ooops, no Si5351 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  Serial.println("OK!");

  /* Setup PLLA to integer only mode @ 900MHz (must be 600..900MHz) */
  Serial.println("Set PLLA to 900MHz");
  clockgen.setupPLLInt(SI5351_PLL_A, 36);

  /* FRACTIONAL MODE --> More flexible but introduce clock jitter */
  //freq = 25 * (20.8/20) = 26 MHz
  clockgen.setupPLL(SI5351_PLL_B, 20, 8, 10);
  Serial.println("Set Output #1");
  clockgen.setupMultisynth(0, SI5351_PLL_B, 20, 0, 1);

  /* Enable the clocks */
  clockgen.enableOutputs(true);
}

void loop(void) {
}