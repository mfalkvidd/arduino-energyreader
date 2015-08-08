#define MIN_BLINKS 2
#define INTERRUPT 1 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define LOW_PRESET_PIN 7
#define LDR_ANALOG_PIN 14
#define debug true
volatile unsigned int blinks = 0;
unsigned int preset_resistance = 1000;
unsigned int mainLoopCounter = 0;
unsigned long lastReport = 0;

void calibrate()
{
  unsigned int val = 1023;
  for (unsigned int i = 0; i < 400; i++)
  {
    unsigned int newval = analogRead(LDR_ANALOG_PIN);
    if (newval < val)
    {
      val = newval;
    }
  }
  log("Preset is ");
  log(preset_resistance);
  log(" Value is ");
  for (unsigned int i = 0; i < val; i += 10) {
    log("*");
  }
  log(" ");
  log(val);
  log("\n");
  if (preset_resistance == 1000 && val > 1023 * 0.4) {
    preset_resistance = 167; // 220 kohm parallel with 1000kohm equals 167kohm
    pinMode(LOW_PRESET_PIN, OUTPUT);
    digitalWrite(LOW_PRESET_PIN, LOW); // pull to ground
    log("Too much light, connecting second resistor\n");
  } else if (preset_resistance == 167 && val < 1023 * 0.3 * 167 / 1000)
  {
    preset_resistance = 1000;
    pinMode(LOW_PRESET_PIN, INPUT);
    log("Too little light, disconnecting second resistor\n");
  }

}

void setup()
{
  if (debug) {
    Serial.begin(9600);
    log("Debug is on\n");
  }
  attachInterrupt(INTERRUPT, onPulse, RISING);
}

void log(unsigned int number) {
  if (debug) {
    Serial.print(number);
  }
}
void log(unsigned long number) {
  if (debug) {
    Serial.print(number);
  }
}
void log(String text) {
  if (debug) {
    Serial.print(text);
  }
}

void loop()
{
  unsigned int blinksSinceLast = blinks;
  blinks = 0;
  log("                                                                    ");
  log(blinksSinceLast);
  log(" interrupts have occurred, ");
  unsigned long timeNow = millis();
  unsigned long timeSinceLast = timeNow - lastReport;
  log(timeSinceLast);
  log(" milliseconds since last report. ");
  unsigned long watts = (double)blinksSinceLast * 3600 / 10 * 1000 / timeSinceLast;
  log(watts);
  log("W\n");
  lastReport = timeNow;

  mainLoopCounter++;
  if (mainLoopCounter == 10 || blinksSinceLast < MIN_BLINKS)
  {
    mainLoopCounter = 0;
    calibrate();
  }

  delay(3000);
}



void onPulse()
{
  blinks++;
}
