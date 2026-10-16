#include "traffic_light.h"

// Навчальні тривалості із заготовки, у мілісекундах.
#define T_GREEN_MS           10000
#define T_GREEN_BLINK_MS      3000
#define T_YELLOW_MS           3000
#define T_RED_MS             10000
#define T_RED_YELLOW_MS       2000
void traffic_light_init(TrafficLight *light)
{
    light->state = STATE_GREEN;
    light->blink_on = true;
}

void traffic_light_handle_event(TrafficLight *light, TrafficEvent event)
{
    if (event == EVT_BLINK_TICK) {
        if (light->state == STATE_GREEN_BLINK || light->state == STATE_YELLOW_BLINK) {
            light->blink_on = !light->blink_on;
        }
        return;
    }
    if (event == EVT_BUTTON) {
        // Повернення до звичайного циклу починається з червоного.
        light->state = light->state == STATE_YELLOW_BLINK
                           ? STATE_RED : STATE_YELLOW_BLINK;
        light->blink_on = true;
        return;
    }
    if (event != EVT_STATE_TIMEOUT) {
        return;
    }
    switch (light->state) {
    case STATE_GREEN:
        light->state = STATE_GREEN_BLINK;
        break;
    case STATE_GREEN_BLINK:
        light->state = STATE_YELLOW;
        break;
    case STATE_YELLOW:
        light->state = STATE_RED;
        break;
    case STATE_RED:
        light->state = STATE_RED_YELLOW;
        break;
    case STATE_RED_YELLOW:
        light->state = STATE_GREEN;
        break;
    case STATE_YELLOW_BLINK:
        return; // Особливий режим триває до наступного натискання.
    default:
        light->state = STATE_RED;
        break;
    }
    light->blink_on = true;
}

uint32_t traffic_light_duration_ms(TrafficState state)
{
    switch (state) {
    case STATE_GREEN:        return T_GREEN_MS;
    case STATE_GREEN_BLINK:  return T_GREEN_BLINK_MS;
    case STATE_YELLOW:       return T_YELLOW_MS;
    case STATE_RED:          return T_RED_MS;
    case STATE_RED_YELLOW:   return T_RED_YELLOW_MS;
    case STATE_YELLOW_BLINK: return 0; // Таймер стану вимкнений.
    default:                return T_RED_MS;
    }
}

uint8_t traffic_light_outputs(const TrafficLight *light)
{
    switch (light->state) {
    case STATE_GREEN:        return LIGHT_GREEN;
    case STATE_GREEN_BLINK:  return light->blink_on ? LIGHT_GREEN : 0;
    case STATE_YELLOW:       return LIGHT_YELLOW;
    case STATE_RED:          return LIGHT_RED;
    case STATE_RED_YELLOW:   return LIGHT_RED | LIGHT_YELLOW;
    case STATE_YELLOW_BLINK: return light->blink_on ? LIGHT_YELLOW : 0;
    default:                return LIGHT_RED;
    }
}

const char *traffic_light_state_name(TrafficState state)
{
    switch (state) {
    case STATE_GREEN:        return "GREEN";
    case STATE_GREEN_BLINK:  return "GREEN_BLINK";
    case STATE_YELLOW:       return "YELLOW";
    case STATE_RED:          return "RED";
    case STATE_RED_YELLOW:   return "RED_YELLOW";
    case STATE_YELLOW_BLINK: return "YELLOW_BLINK";
    default:                return "UNKNOWN";
    }
}
