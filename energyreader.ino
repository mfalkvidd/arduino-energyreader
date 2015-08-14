#include <SPI.h>
#include <MySensor.h>
#define CHILD_ID_POWER 0
#define CHILD_ID_LIGHT_LEVEL 1
#define CHILD_ID_RESISTOR 2
unsigned long lastSend;
MyMessage msgPower(CHILD_ID_POWER, V_WATT);
MyMessage msgLight(CHILD_ID_LIGHT_LEVEL, V_LIGHT_LEVEL);
MyMessage msgResistor(CHILD_ID_RESISTOR, V_IMPEDANCE);
MySensor gw;

#define LOOPTIME 5000
#define ULONG_MAX 4294967295
#define MIN_BLINKS 2
#define BLINK_INTERRUPT 1 // Interrupt 0 is used by the gw in MySensors
#define LOW_PRESET_PIN 7
#define LDR_ANALOG_PIN 14
#define mydebug true
volatile unsigned int ledblinks = 0;
unsigned int preset_resistance = 1000;
unsigned int mainLoopCounter = 0;
unsigned long lastReport = 0;
unsigned int lastLightLevel = 0;
unsigned int blinksSinceLast = 0;
unsigned int batteryLevelPercent = 88; // TODO: Measure and set
unsigned long watts = 0;


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
  lastLightLevel = val;
}

void incomingMessage(const MyMessage &message) {
  if (message.type == V_VAR1) {
    unsigned long oldPulseCount = message.getULong();
    log("Received last pulse count from gw: ");
    log(oldPulseCount);
  }
}

void setup()
{
  gw.begin(incomingMessage);
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Energy Meter", "1.0");

  // Register this device as power sensor
  gw.present(CHILD_ID_POWER, V_WATT);
  gw.present(CHILD_ID_LIGHT_LEVEL, V_LIGHT_LEVEL);
  gw.present(CHILD_ID_RESISTOR, V_IMPEDANCE);

  // Fetch last known pulse count value from gw
  gw.request(CHILD_ID_POWER, V_VAR1);

  lastSend = millis();

  if (mydebug) {
    Serial.begin(115200);
    log("Debug is on\n");
  }
  pinMode(LOW_PRESET_PIN, INPUT);
  attachInterrupt(BLINK_INTERRUPT, onPulse, RISING);
}

void log(unsigned int number) {
  if (mydebug) {
    Serial.print(number);
  }
}
void log(unsigned long number) {
  if (mydebug) {
    Serial.print(number);
  }
}
void log(String text) {
  if (mydebug) {
    Serial.print(text);
  }
}

void loop()
{
  detachInterrupt(BLINK_INTERRUPT);
  unsigned long timeNow = millis();
  gw.process();
  blinksSinceLast = ledblinks;
  ledblinks = 0;

  log("                                                                    ");
  log(blinksSinceLast);
  log(" interrupts have occurred, ");
  unsigned long timeSinceLast;
  timeSinceLast = timeNow - lastReport; // unsigned arithmetic will handle the overflow automatically
  log(timeSinceLast);
  log(" milliseconds since last report. ");
  if (blinksSinceLast > MIN_BLINKS) {
    // Only calculate new value if we got enough blinks. If we didn't get enough blinks, we're probably blinded and then we'd better report the last known value
    watts = blinksSinceLast * 3600.0 * 1000 / 10.0 * 1000.0 / timeSinceLast; // 3600 seconds per hour, 1000 factor for Domoticz to be happy, 10,000 blinks per kWh
  }
  log(watts);
  log("W\n");

  mainLoopCounter++;
  if (mainLoopCounter == 10)
  {
    mainLoopCounter = 0;
    calibrate();
  }
  sendResults();
  lastReport = millis();
  EIFR = bit (INTF1);  // clear flag for interrupt 1
  attachInterrupt(BLINK_INTERRUPT, onPulse, RISING);
  delay(LOOPTIME);
}

void onPulse()
{
  ledblinks++;
}

void sendResults() {
  if (!gw.send(msgPower.set(watts, 1))) {
    log("No ACK for power\n");
  }
  if (!gw.send(msgLight.set(lastLightLevel, 1))) {
    log("No ACK for lightLevel\n");
  }
  if (!gw.send(msgResistor.set(preset_resistance, 1))) {
    log("No ACK for resistance\n");
  }
  gw.sendBatteryLevel(batteryLevelPercent);
}

