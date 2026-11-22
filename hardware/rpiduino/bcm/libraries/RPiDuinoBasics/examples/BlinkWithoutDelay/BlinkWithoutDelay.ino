constexpr unsigned long IntervalMs = 500;
static unsigned long lastChange;
static bool ledOn;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    lastChange = millis();
}

void loop()
{
    const unsigned long now = millis();
    if (now - lastChange < IntervalMs) return;

    lastChange = now;
    ledOn = !ledOn;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
}
