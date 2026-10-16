#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "../main/main.cpp"

// Реальні setup()/loop(); моделюються лише Arduino API, GPIO та апаратні таймери.
static hw_timer_t timers[2];
static uint64_t clockUs;
static int pins[8];
static void (*buttonIsr)();
static void (*pendingIsr)();
static bool irqEnabled = true;
static bool failTimers = false;
static bool trace = false;
static int lastOutputs = -1;
static TrafficState lastState;
static std::vector<std::string> messages;
TestSerial Serial;

void TestSerial::begin(unsigned long baud) { assert(baud == 115200); }
void TestSerial::println(const char *message) { assert(irqEnabled); messages.emplace_back(message); }
void pinMode(uint8_t, uint8_t) {}
void digitalWrite(uint8_t pin, uint8_t level)
{
    assert(irqEnabled);
    pins[pin] = level;
    assert(!pins[PIN_GREEN] || (!pins[PIN_RED] && !pins[PIN_YELLOW]));
}
int digitalRead(uint8_t pin) { return pins[pin]; }
int digitalPinToInterrupt(uint8_t pin) { return pin; }
void attachInterrupt(int pin, void (*callback)(), int mode)
{
    assert(pin == PIN_BUTTON && mode == CHANGE);
    buttonIsr = callback;
}
unsigned long millis() { return static_cast<uint32_t>(clockUs / 1000); }
void noInterrupts() { assert(irqEnabled); irqEnabled = false; }
void interrupts()
{
    assert(!irqEnabled);
    irqEnabled = true;
    if (pendingIsr) {
        const auto callback = pendingIsr;
        pendingIsr = nullptr;
        callback();
    }
}
hw_timer_t *timerBegin(uint8_t number, uint16_t divider, bool countUp)
{
    assert(number < 2 && divider == 80 && countUp);
    return failTimers ? nullptr : &timers[number];
}
void timerAttachInterrupt(hw_timer_t *timer, void (*callback)(), bool edge)
{
    assert(!edge);
    timer->callback = callback;
}
void timerAlarmWrite(hw_timer_t *timer, uint64_t alarm, bool autoreload)
{
    timer->alarm = alarm;
    timer->autoreload = autoreload;
}
void timerAlarmEnable(hw_timer_t *timer) { timer->enabled = true; }
void timerAlarmDisable(hw_timer_t *timer) { timer->enabled = false; }
void timerWrite(hw_timer_t *timer, uint64_t value) { timer->counter = value; }
void timerStart(hw_timer_t *timer) { timer->running = true; }
void timerStop(hw_timer_t *timer) { timer->running = false; }

static void observe()
{
    const int outputs = (pins[PIN_RED] ? LIGHT_RED : 0) |
                        (pins[PIN_YELLOW] ? LIGHT_YELLOW : 0) |
                        (pins[PIN_GREEN] ? LIGHT_GREEN : 0);
    assert(outputs == traffic_light_outputs(&light));
    if (trace && (outputs != lastOutputs || light.state != lastState)) {
        std::printf("%7llu %-14s %d %d %d\n",
                    static_cast<unsigned long long>(clockUs / 1000),
                    traffic_light_state_name(light.state),
                    pins[PIN_RED], pins[PIN_YELLOW], pins[PIN_GREEN]);
    }
    lastOutputs = outputs;
    lastState = light.state;
}

static void reset(uint64_t startMs = 0)
{
    for (auto &timer : timers) { timer = hw_timer_t{}; }
    for (auto &pin : pins) { pin = LOW; }
    pins[PIN_BUTTON] = HIGH;
    clockUs = startMs * 1000;
    stateTimeoutFlag = blinkTickFlag = buttonFlag = false;
    ready = false;
    irqEnabled = true;
    buttonIsr = pendingIsr = nullptr;
    lastOutputs = -1;
    messages.clear();
    setup();
    if (ready) { observe(); }
}

static void advanceMs(uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        clockUs += 1000;
        for (auto &timer : timers) {
            if (!timer.running) { continue; }
            timer.counter += 1000;
            if (timer.enabled && timer.counter >= timer.alarm) {
                if (timer.autoreload) { timer.counter = 0; }
                else { timer.enabled = false; }
                assert(timer.callback != nullptr);
                timer.callback();
            }
        }
        loop();
        observe();
    }
}

static void setButton(int level)
{
    assert(pins[PIN_BUTTON] != level && buttonIsr != nullptr);
    pins[PIN_BUTTON] = level;
    buttonIsr();
    loop();
    observe();
}

static void testCycle()
{
    reset();
    assert(timers[0].alarm == 10000000 && !timers[0].autoreload);
    assert(!timers[1].running);
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        advanceMs(9999);
        assert(light.state == STATE_GREEN);
        advanceMs(1);
        assert(light.state == STATE_GREEN_BLINK && light.blink_on);
        assert(timers[0].alarm == 3000000 && !timers[0].autoreload);
        assert(timers[1].alarm == 500000 && timers[1].autoreload);
        for (unsigned phase = 0; phase < 5; ++phase) {
            advanceMs(499);
            assert(light.blink_on == (phase % 2 == 0));
            advanceMs(1);
            assert(light.blink_on == (phase % 2 != 0));
        }
        advanceMs(500); // Одночасно спрацьовують обидва таймери.
        assert(light.state == STATE_YELLOW && !timers[1].running);
        advanceMs(3000);
        assert(light.state == STATE_RED);
        advanceMs(10000);
        assert(light.state == STATE_RED_YELLOW);
        advanceMs(2000);
        assert(light.state == STATE_GREEN);
    }
}

static void testButton(uint64_t startMs)
{
    reset(startMs);
    setButton(LOW);
    advanceMs(100);
    setButton(HIGH);
    advanceMs(20);
    setButton(LOW);
    advanceMs(199);
    assert(light.state == STATE_GREEN);
    advanceMs(1);
    assert(light.state == STATE_YELLOW_BLINK);
    assert(!timers[0].enabled && !timers[0].running);
    advanceMs(1500); // Утримання кнопки не створює повторної події.
    assert(light.state == STATE_YELLOW_BLINK && !light.blink_on);
    setButton(HIGH);
    advanceMs(200);
    setButton(LOW);
    advanceMs(199);
    // Натискання, завершення стану та крок миготіння в одному знімку.
    clockUs += 1000;
    onStateTimer();
    onBlinkTimer();
    loop();
    observe();
    assert(light.state == STATE_RED);
    assert(timers[0].counter == 0 && timers[0].alarm == 10000000);
    assert(!timers[1].running && !blinkTickFlag && !stateTimeoutFlag);
    advanceMs(10000);
    assert(light.state == STATE_RED_YELLOW);
}

static void testFlags()
{
    reset();
    clockUs += 10000000;
    loop();
    assert(light.state == STATE_GREEN); // Час сам по собі не перемикає стан.
    onStateTimer();
    assert(light.state == STATE_GREEN && stateTimeoutFlag);
    loop();
    assert(light.state == STATE_GREEN_BLINK);
    assert(timers[0].counter == 0 && timers[1].counter == 0);
    // ISR одразу після зчитування/скидання залишається для наступного loop().
    pendingIsr = &onBlinkTimer;
    loop();
    assert(light.blink_on && blinkTickFlag);
    loop();
    assert(!light.blink_on && !blinkTickFlag);
    onStateTimer();
    onBlinkTimer();
    loop();
    assert(light.state == STATE_YELLOW && light.blink_on);
    assert(!timers[1].running);
    failTimers = true;
    reset();
    loop();
    assert(!ready && pins[PIN_RED] && !pins[PIN_GREEN] && !pins[PIN_YELLOW]);
    failTimers = false;
}

int main()
{
    testCycle();
    testButton(0);
    testButton(UINT32_MAX - 150ULL); // Переповнення millis() під час антидребезгу.
    testFlags();
    std::puts("PASS: real setup()/loop() with mocked Arduino GPIO/timers:");
    std::puts("      two cycles, one-shot/periodic alarms, bounce, held button, millis rollover,");
    std::puts("      simultaneous events, ISR flags, initialization failure, no conflicting LEDs.");
    std::puts("Host simulation; hardware timers/interrupts mocked; not a physical-board capture.");
    std::puts("time_ms state          R Y G");
    trace = true;
    reset();
    advanceMs(28800);
    setButton(LOW);
    advanceMs(200); // Підтверджене натискання на 29-й секунді.
    advanceMs(500);
    setButton(HIGH);
    advanceMs(1300);
    setButton(LOW);
    advanceMs(200); // Підтверджене натискання на 31-й секунді.
    advanceMs(12000);
    return 0;
}
