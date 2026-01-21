#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "hardware/sync.h"

#define BUTTON_DEBOUNCE_DELAY_MS    50
#define YELLOW_TIMEOUT_MS           3000
#define EVENT_QUEUE_LENGTH          10 

typedef enum _event {go_evt, stop_evt, timeout_evt, no_evt} evt_t; 

const static uint led_red = 0; 
const static uint led_yellow = 1; 
const static uint led_green = 2; 

const static uint go_btn = 20; 
const static uint stop_btn = 21; 

/* Event queue */
queue_t evt_queue; 

/* Function pointer primitive */ 
typedef void (*state_func_t)( void );

typedef struct _state 
{
    uint8_t id;
    state_func_t Enter;
} state_t;

/* Last button ISR time */
unsigned long button_time = 0;

void button_isr(uint gpio, uint32_t events) 
{
    if ((to_ms_since_boot(get_absolute_time())-button_time) > BUTTON_DEBOUNCE_DELAY_MS) 
    {
        button_time = to_ms_since_boot(get_absolute_time());
        
        evt_t evt;
        switch(gpio)
        {
            case go_btn: 
                evt = go_evt; 
                queue_try_add(&evt_queue, &evt); 
            break; 

            case stop_btn: 
                evt = stop_evt; 
                queue_try_add(&evt_queue, &evt); 
            break;
        }
    }
}

int64_t alarm_callback(alarm_id_t id, void * user_data)
{
    evt_t evt = timeout_evt; 
    queue_try_add(&evt_queue, &evt); 

    /* Return 0 so that the alarm is not rescheduled */
    return 0; 
}

static void app_init(void)
{
    /* Setup LEDs */
    gpio_init(led_red); 
    gpio_init(led_yellow); 
    gpio_init(led_green);  
    gpio_set_dir(led_red, GPIO_OUT);
    gpio_set_dir(led_yellow, GPIO_OUT);
    gpio_set_dir(led_green, GPIO_OUT);
    
    /* Setup buttons */
    gpio_init(go_btn);
    gpio_init(stop_btn);
    gpio_set_irq_enabled_with_callback(go_btn, GPIO_IRQ_EDGE_FALL, true, &button_isr); 
    gpio_set_irq_enabled(stop_btn, GPIO_IRQ_EDGE_FALL, true); 

    /* Event queue setup */
    queue_init(&evt_queue, sizeof(evt_t), EVENT_QUEUE_LENGTH); 
}

evt_t get_event(void)
{
    evt_t evt = no_evt; 
    if (queue_try_remove(&evt_queue, &evt))
    { 
        return evt; 
    }
    return no_evt; 
}

void leds_off(void) 
{
    gpio_put(led_red, 0); 
    gpio_put(led_yellow, 0); 
    gpio_put(led_green, 0); 
}

void enter_state_red(void)
{
    leds_off();
    gpio_put(led_red, 1);
}

void enter_state_yellow(void)
{
    leds_off();
    gpio_put(led_yellow, 1);
    add_alarm_in_ms(YELLOW_TIMEOUT_MS, alarm_callback, NULL, false);
}

void enter_state_green(void)
{
    leds_off();
    gpio_put(led_green, 1);
}

const state_t state_red = {
    0, 
    enter_state_red
};

const state_t state_yellow = {
    1, 
    enter_state_yellow
};

const state_t state_green = {
    2, 
    enter_state_green
};

const state_t state_table[3][4] = {
    /*  STATE       GO              STOP            TIMEOUT         NO-EVT */
    {/* RED */      state_green,    state_red,      state_red,      state_red},
    {/* YELLOW */   state_yellow,   state_yellow,   state_red,      state_yellow},    
    {/* GREEN */    state_green,    state_yellow,   state_green,    state_green}
};

int main(void)
{
    app_init();

    state_t current_state = state_red;
    evt_t evt = no_evt;

    for(;;)
    {
        current_state.Enter(); 
        while(current_state.id == state_table[current_state.id][evt].id)
        {
            evt = get_event();
        }
        current_state = state_table[current_state.id][evt];
    }
}
