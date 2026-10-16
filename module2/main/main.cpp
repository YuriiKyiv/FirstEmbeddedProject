#include <Arduino.h>
#include "traffic_light.h"

// Arduino-ESP32 2.x: APB 80 МГц / 80 = 1 МГц, один тік = 1 мкс.
static constexpr uint16_t TIMER_DIVIDER = 80;
static constexpr uint32_t BLINK_HALF_PERIOD_MS = 500;
static constexpr uint32_t DEBOUNCE_MS = 200;
static constexpr uint8_t PIN_GREEN = 5;
static constexpr uint8_t PIN_YELLOW = 6;
static constexpr uint8_t PIN_RED = 7;
static constexpr uint8_t PIN_BUTTON = 4;

static hw_timer_t *stateTimer = nullptr;
static hw_timer_t *blinkTimer = nullptr;
static volatile bool stateTimeoutFlag = false;
static volatile bool blinkTickFlag = false;
static volatile bool buttonFlag = false;
static TrafficLight light;
static bool ready = false;
static int rawButton = HIGH;
static int stableButton = HIGH;
static uint32_t buttonChangedMs = 0;

// ISR не змінюють стан, GPIO чи таймери й не викликають millis()/Serial.
void IRAM_ATTR onStateTimer() { stateTimeoutFlag = true; }
void IRAM_ATTR onBlinkTimer() { blinkTickFlag = true; }
void IRAM_ATTR onButton() { buttonFlag = true; }

struct EventFlags {
    bool stateTimeout;
    bool blinkTick;
    bool button;
};

static EventFlags takeEvents()
{
    // Всі ISR реєструються в setup() на тому самому ядрі, де працює loop().
    // Читання і скидання мають бути одним неподільним кроком щодо цих ISR.
    noInterrupts();
    const EventFlags events = {stateTimeoutFlag, blinkTickFlag, buttonFlag};
    stateTimeoutFlag = false;
    blinkTickFlag = false;
    buttonFlag = false;
    interrupts();
    return events;
}

static void setLights(uint8_t outputs)
{
    // Спочатку гасимо попередню комбінацію.
    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_YELLOW, LOW);
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_RED, (outputs & LIGHT_RED) ? HIGH : LOW);
    digitalWrite(PIN_YELLOW, (outputs & LIGHT_YELLOW) ? HIGH : LOW);
    digitalWrite(PIN_GREEN, (outputs & LIGHT_GREEN) ? HIGH : LOW);
}

static void enterState()
{
    timerAlarmDisable(stateTimer);
    timerStop(stateTimer);
    timerAlarmDisable(blinkTimer);
    timerStop(blinkTimer);

    // Події від попереднього стану не повинні впливати на новий.
    noInterrupts();
    stateTimeoutFlag = false;
    blinkTickFlag = false;
    interrupts();

    timerWrite(stateTimer, 0);
    timerWrite(blinkTimer, 0);
    setLights(traffic_light_outputs(&light));

    const uint32_t durationMs = traffic_light_duration_ms(light.state);
    if (durationMs != 0) {
        // false: одноразова подія; наступний стан сам переозброїть таймер.
        timerAlarmWrite(stateTimer, (uint64_t)durationMs * 1000ULL, false);
        timerAlarmEnable(stateTimer);
        timerStart(stateTimer);
    }
    if (light.state == STATE_GREEN_BLINK || light.state == STATE_YELLOW_BLINK) {
        // true: автоматичне повторення кожні 500 мс.
        timerAlarmWrite(blinkTimer, (uint64_t)BLINK_HALF_PERIOD_MS * 1000ULL, true);
        timerAlarmEnable(blinkTimer);
        timerStart(blinkTimer);
    }
    Serial.println(traffic_light_state_name(light.state));
}

static void dispatch(TrafficEvent event)
{
    const TrafficState previous = light.state;
    traffic_light_handle_event(&light, event);
    if (light.state != previous) {
        enterState();
    } else if (event == EVT_BLINK_TICK) {
        setLights(traffic_light_outputs(&light));
    }
}

static bool buttonPressed(bool edge)
{
    const uint32_t now = millis();
    const int sample = digitalRead(PIN_BUTTON);
    if (edge || sample != rawButton) {
        rawButton = sample;
        buttonChangedMs = now;
    }
    // Беззнакове віднімання працює і при переповненні millis().
    if (rawButton != stableButton && (uint32_t)(now - buttonChangedMs) >= DEBOUNCE_MS) {
        stableButton = rawButton;
        return stableButton == LOW;
    }
    return false;
}

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_YELLOW, OUTPUT);
    pinMode(PIN_RED, OUTPUT);
    setLights(0);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    rawButton = digitalRead(PIN_BUTTON);
    stableButton = HIGH;
    buttonChangedMs = millis();

    stateTimer = timerBegin(0, TIMER_DIVIDER, true);
    blinkTimer = timerBegin(1, TIMER_DIVIDER, true);
    if (stateTimer == nullptr || blinkTimer == nullptr) {
        setLights(LIGHT_RED);
        Serial.println("ERROR: hardware timer initialization failed");
        return;
    }
    timerAttachInterrupt(stateTimer, &onStateTimer, false);
    timerAttachInterrupt(blinkTimer, &onBlinkTimer, false);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), &onButton, CHANGE);

    traffic_light_init(&light);
    enterState();
    ready = true;
}

void loop()
{
    if (!ready) {
        return;
    }
    const EventFlags events = takeEvents();
    if (buttonPressed(events.button)) {
        dispatch(EVT_BUTTON);
        return; // Відкидаємо таймерні події старого стану з цього знімка.
    }
    if (events.stateTimeout) {
        dispatch(EVT_STATE_TIMEOUT);
        return; // Не застосовуємо одночасний крок миготіння до нового стану.
    }
    if (events.blinkTick) {
        dispatch(EVT_BLINK_TICK);
    }
}
