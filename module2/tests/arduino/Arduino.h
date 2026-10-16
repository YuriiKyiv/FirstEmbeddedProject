#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H
#include <cstdint>
#define IRAM_ATTR
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define CHANGE 3
struct hw_timer_t {
    bool running = false;
    bool enabled = false;
    bool autoreload = false;
    uint64_t counter = 0;
    uint64_t alarm = 0;
    void (*callback)() = nullptr;
};
struct TestSerial {
    void begin(unsigned long baud);
    void println(const char *message);
};
extern TestSerial Serial;
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t level);
int digitalRead(uint8_t pin);
int digitalPinToInterrupt(uint8_t pin);
void attachInterrupt(int pin, void (*callback)(), int mode);
unsigned long millis();
void noInterrupts();
void interrupts();
hw_timer_t *timerBegin(uint8_t number, uint16_t divider, bool countUp);
void timerAttachInterrupt(hw_timer_t *timer, void (*callback)(), bool edge);
void timerAlarmWrite(hw_timer_t *timer, uint64_t alarm, bool autoreload);
void timerAlarmEnable(hw_timer_t *timer);
void timerAlarmDisable(hw_timer_t *timer);
void timerWrite(hw_timer_t *timer, uint64_t value);
void timerStart(hw_timer_t *timer);
void timerStop(hw_timer_t *timer);
#endif
