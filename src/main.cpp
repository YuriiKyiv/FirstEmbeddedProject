#include <Arduino.h>

const int SENSOR_PIN = 6; // Pin connected to the photoresistor voltage divider (GPIO In, ADC)
const int RELAY_PIN = 4;  // Pin driving the relay transistor (GPIO Out)

// Hysteresis thresholds (ADC 0...4095).
// Below THRESHOLD_DARK  -> dark  -> relay ON.
// Above THRESHOLD_LIGHT -> light -> relay OFF.
// Between the thresholds the state does not change, so the relay does not "chatter".
const int THRESHOLD_DARK = 2200;
const int THRESHOLD_LIGHT = 2900;

// EMA filter: y = alpha * x + (1 - alpha) * y_prev.
// Smaller alpha -> stronger smoothing, but slower response.
// When sampling every 20 ms, alpha = 0.1 gives a time constant of ~200 ms.
const float EMA_ALPHA = 0.1f;

const unsigned long LOOP_PERIOD_MS = 20;  // Sensor sampling period
const unsigned long LOG_PERIOD_MS = 100;   // Teleplot output period

bool relayOn = false;              // Current relay state
float filteredValue = 0;           // EMA filter output (y[i-1])
unsigned long lastLogMs = 0;       // Time of the last Serial output

void setup() {
  Serial.begin(115200);
  Serial.println("LDR relay controller started");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with the relay off

  // ADC resolution (12 bits = 0...4095)
  analogReadResolution(12);

  // --- Temporary sensor pin diagnostics ---
  pinMode(SENSOR_PIN, INPUT_PULLDOWN); delay(20);
  int withPullDown = analogRead(SENSOR_PIN);
  pinMode(SENSOR_PIN, INPUT_PULLUP);   delay(20);
  int withPullUp = analogRead(SENSOR_PIN);
  pinMode(SENSOR_PIN, INPUT);          delay(20);
  int noPull = analogRead(SENSOR_PIN);
  Serial.printf("DIAG GPIO%d: pulldown=%d pullup=%d none=%d mV=%d\n",
                SENSOR_PIN, withPullDown, withPullUp, noPull, analogReadMilliVolts(SENSOR_PIN));

  // Seed the filter with a real reading to avoid a "ramp-up" from zero
  filteredValue = analogRead(SENSOR_PIN);
}

void loop() {
  // 1. Read the value from the R1(LDR) + R2 divider
  int rawValue = analogRead(SENSOR_PIN);

  // 1a. Smooth it with the EMA filter
  filteredValue = EMA_ALPHA * rawValue + (1.0f - EMA_ALPHA) * filteredValue;
  int adcValue = (int)(filteredValue + 0.5f); // rounded value for comparison

  // 2. Compare the filtered value against the thresholds (hysteresis)
  if (adcValue < THRESHOLD_DARK && !relayOn) {
    relayOn = true;
    digitalWrite(RELAY_PIN, HIGH); // Dark -> turn the relay on
    Serial.print("Dark  -> relay ON  (ADC = ");
    Serial.print(adcValue);
    Serial.println(")");
  } else if (adcValue > THRESHOLD_LIGHT && relayOn) {
    relayOn = false;
    digitalWrite(RELAY_PIN, LOW);  // Light -> turn the relay off
    Serial.print("Light -> relay OFF (ADC = ");
    Serial.print(adcValue);
    Serial.println(")");
  }
  // 3. Between the thresholds nothing changes

  // Teleplot output format: ">name:value" -> a separate plot for each variable
  if (millis() - lastLogMs >= LOG_PERIOD_MS) {
    lastLogMs = millis();
    Serial.print(">Raw:");
    Serial.println(rawValue);
    Serial.print(">Filtered:");
    Serial.println(adcValue);
    Serial.print(">Relay:");
    Serial.println(relayOn ? 1 : 0);
  }

  delay(LOOP_PERIOD_MS);
}
