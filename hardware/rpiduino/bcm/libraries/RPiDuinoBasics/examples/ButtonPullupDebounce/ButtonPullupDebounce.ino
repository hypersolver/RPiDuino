constexpr uint8_t ButtonPin = 17;  // BCM GPIO17, physical pin 11.
constexpr unsigned long DebounceMs = 30;
static unsigned long lastChange;
static int rawState;
static int stableState = HIGH;
static bool ledOn;

void setup()
{
    pinMode(ButtonPin, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    rawState = digitalRead(ButtonPin);
    lastChange = millis();
}

void loop()
{
    const unsigned long now = millis();
    const int reading = digitalRead(ButtonPin);
    if (reading != rawState) {
        rawState = reading;
        lastChange = now;
    }

    if (rawState == stableState || now - lastChange < DebounceMs) return;
    stableState = rawState;
    if (stableState == LOW) {
        ledOn = !ledOn;
        digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
    }
}
