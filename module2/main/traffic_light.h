#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATE_GREEN = 0,
    STATE_GREEN_BLINK,
    STATE_YELLOW,
    STATE_RED,
    STATE_RED_YELLOW,
    STATE_YELLOW_BLINK,
} TrafficState;

typedef enum {
    EVT_STATE_TIMEOUT = 1,
    EVT_BLINK_TICK,
    EVT_BUTTON,
} TrafficEvent;

enum {
    LIGHT_RED = 1 << 0,
    LIGHT_YELLOW = 1 << 1,
    LIGHT_GREEN = 1 << 2,
};

typedef struct {
    TrafficState state;
    bool blink_on;
} TrafficLight;

void traffic_light_init(TrafficLight *light);
void traffic_light_handle_event(TrafficLight *light, TrafficEvent event);
uint32_t traffic_light_duration_ms(TrafficState state);
uint8_t traffic_light_outputs(const TrafficLight *light);
const char *traffic_light_state_name(TrafficState state);

#ifdef __cplusplus
}
#endif
#endif
