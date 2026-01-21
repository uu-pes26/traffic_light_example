#include "pico/stdlib.h"
#include "pico/util/queue.h"

#define BUTTON_DEBOUNCE_DELAY_MS    50
#define YELLOW_TIMEOUT_MS           3000
#define EVENT_QUEUE_LENGTH          10 

typedef enum _state {red_state, yellow_state, green_state} state_t; 
typedef enum _event {go_evt, stop_evt, timeout_evt, no_evt} evt_t; 

const static uint led_red = 0; 
const static uint led_yellow = 1; 
const static uint led_green = 2;

const static uint go_btn = 20; 
const static uint stop_btn = 21; 

/* Event queue */
queue_t evt_queue;

/* Last button ISR time */
unsigned long button_time = 0;

void button_isr(uint pin, uint32_t events) 
{
    if ((to_ms_since_boot(get_absolute_time())-button_time) > BUTTON_DEBOUNCE_DELAY_MS) 
    {
        button_time = to_ms_since_boot(get_absolute_time());
        
        evt_t evt;
        switch(pin)
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
    /* Enable interrupt line shared by all GPIO pins */
    irq_set_enabled(IO_IRQ_BANK0, true);
    /* Set callback button_isr, triggered on any IRQs on that line*/
    gpio_set_irq_callback(button_isr);
    /* Trigger IRQs on falling edges on go_btn*/
    gpio_set_irq_enabled(go_btn, GPIO_IRQ_EDGE_FALL, true); 
    /* For convenience, the three methods above can be replaced by the method below*/
    //gpio_set_irq_enabled_with_callback(go_btn, GPIO_IRQ_EDGE_FALL, true, button_isr); 

    /* trigger IRQs on falling edges on stop_btn*/
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

int main(void)
{
    app_init();

    evt_t evt = no_evt;
    state_t state = red_state; 

    while (true)
    {
        evt = get_event();

        switch(state)
        {
            case red_state:
                gpio_put(led_red, 1);

                if(evt == go_evt)
                {
                    gpio_put(led_red, 0);
                    state = green_state;
                }
            break;

            case yellow_state:
                gpio_put(led_yellow, 1);

                if(evt == timeout_evt)
                {
                    gpio_put(led_yellow, 0);
                    state = red_state;
                }
            break;

            case green_state:
                gpio_put(led_green, 1);

                if(evt == stop_evt)
                {
                    gpio_put(led_green, 0);
                    state = yellow_state;

                    /* Start alarm here */
                    add_alarm_in_ms(YELLOW_TIMEOUT_MS, alarm_callback, NULL, false); 
                }
            break;
        }
    }
}
