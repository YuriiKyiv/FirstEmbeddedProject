#include <assert.h>
#include <stdio.h>
#include "traffic_light.h"

static void expect(TrafficLight *light, TrafficEvent event,
                   TrafficState state, uint8_t outputs)
{
    traffic_light_handle_event(light, event);
    assert(light->state == state);
    assert(traffic_light_outputs(light) == outputs);
}

int main(void)
{
    TrafficLight light;
    traffic_light_init(&light);
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        assert(light.state == STATE_GREEN);
        assert(traffic_light_duration_ms(light.state) == 10000);
        expect(&light, EVT_BLINK_TICK, STATE_GREEN, LIGHT_GREEN);
        expect(&light, EVT_STATE_TIMEOUT, STATE_GREEN_BLINK, LIGHT_GREEN);
        assert(traffic_light_duration_ms(light.state) == 3000);
        for (unsigned phase = 0; phase < 5; ++phase) {
            expect(&light, EVT_BLINK_TICK, STATE_GREEN_BLINK,
                   phase % 2 == 0 ? 0 : LIGHT_GREEN);
        }
        expect(&light, EVT_STATE_TIMEOUT, STATE_YELLOW, LIGHT_YELLOW);
        assert(traffic_light_duration_ms(light.state) == 3000);
        expect(&light, EVT_STATE_TIMEOUT, STATE_RED, LIGHT_RED);
        assert(traffic_light_duration_ms(light.state) == 10000);
        expect(&light, EVT_STATE_TIMEOUT, STATE_RED_YELLOW, LIGHT_RED | LIGHT_YELLOW);
        assert(traffic_light_duration_ms(light.state) == 2000);
        expect(&light, EVT_STATE_TIMEOUT, STATE_GREEN, LIGHT_GREEN);
    }
    for (unsigned state = 0; state < 5; ++state) {
        traffic_light_init(&light);
        for (unsigned i = 0; i < state; ++i) {
            traffic_light_handle_event(&light, EVT_STATE_TIMEOUT);
        }
        expect(&light, EVT_BUTTON, STATE_YELLOW_BLINK, LIGHT_YELLOW);
        assert(traffic_light_duration_ms(light.state) == 0);
        expect(&light, EVT_STATE_TIMEOUT, STATE_YELLOW_BLINK, LIGHT_YELLOW);
        expect(&light, EVT_BLINK_TICK, STATE_YELLOW_BLINK, 0);
        expect(&light, EVT_BLINK_TICK, STATE_YELLOW_BLINK, LIGHT_YELLOW);
        expect(&light, EVT_BUTTON, STATE_RED, LIGHT_RED);
        expect(&light, EVT_STATE_TIMEOUT, STATE_RED_YELLOW, LIGHT_RED | LIGHT_YELLOW);
        expect(&light, EVT_STATE_TIMEOUT, STATE_GREEN, LIGHT_GREEN);
    }
    puts("PASS: event-driven state machine, durations, blink phases, all special-mode entries.");
    return 0;
}
