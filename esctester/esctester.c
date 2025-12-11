#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_clock.h>
#include <furi_hal_pwm.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>
#include <stdio.h>

#define DEFAULT_FREQ 50
#define DEFAULT_DUTY 1

#define MIN_WIDTH 1000
#define DEFAULT_WIDTH 1000
#define MAX_WIDTH 2000

#define WIDTH_STEP_SMALL 1
#define WIDTH_STEP_BIG 10

uint16_t pWidth = DEFAULT_WIDTH;
bool armed = false;
// Only Manual mode is supported now

typedef enum {
    EventTypeTick,
    EventTypeInput,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} EscTesterEvent;

static void esctester_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);

    char temp_str[36];

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    canvas_draw_frame(canvas, 0, 0, 128, 64);
    canvas_draw_str(canvas, 19, 10, "Pin A7 pulse width");
    canvas_draw_line(canvas, 14, 30, 114, 30);
    canvas_draw_line(canvas, 14, 30, 14, 20);
    canvas_draw_line(canvas, 114, 30, 114, 20);

    // TODO if width range changes, adapt canvas
    canvas_draw_frame(canvas, (pWidth - MIN_WIDTH) / 10.2 + 14, 20, 3, 10);

    snprintf(temp_str, sizeof(temp_str), "%i us", pWidth);

    canvas_draw_str(canvas, 50, 40, temp_str);

    if(!armed) {
        canvas_draw_str(canvas, 28, 50, "Press OK for arm");
    } else {
        canvas_draw_str(canvas, 50, 50, "Manual");
    }
}

static void esctester_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    EscTesterEvent event = {.type = EventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void esctester_timer_callback(void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    EscTesterEvent event = {.type = EventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

void esctester_set_servo_pwm(uint32_t freq, uint32_t compare) {
    uint32_t freq_div = 64000000LU / freq;
    uint32_t prescaler = freq_div / 0x10000LU;
    uint32_t period = freq_div / (prescaler + 1);

    LL_TIM_SetPrescaler(TIM1, prescaler);
    LL_TIM_SetAutoReload(TIM1, period - 1);
    LL_TIM_OC_SetCompareCH1(TIM1, compare);
}

void esctester_update_pwm() {
    esctester_set_servo_pwm(DEFAULT_FREQ, pWidth * 3.2);
}

int32_t esctester_app(void* p) {
    UNUSED(p);

    EscTesterEvent event;

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(EscTesterEvent));

    ViewPort* view_port = view_port_alloc();

    // callbacks init
    view_port_draw_callback_set(view_port, esctester_draw_callback, NULL);
    view_port_input_callback_set(view_port, esctester_input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // Timer for automatic mode
    FuriTimer* timer =
        furi_timer_alloc(esctester_timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_ms_to_ticks((1000/DEFAULT_FREQ)*1.5));  // does not make sense update PWM faster than pulse interval lenght. 

    //GPIO init
    furi_hal_power_enable_otg(); // Turn 5V
    furi_hal_pwm_start(FuriHalPwmOutputIdTim1PA7, 50, 4); // Init Tim1
    esctester_set_servo_pwm(DEFAULT_FREQ, 0); // start with 0 (safe)
    view_port_update(view_port);

    while(1) {
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        uint16_t pWidthNew = pWidth;

        if(event.type == EventTypeInput) {
            if(event.input.key == InputKeyBack) {
                break;
            } else if(event.input.key == InputKeyOk && event.input.type == InputTypeRelease) {
                if(!armed) {
                    // Arm sequence: output small test value for 1s, then enable selected value
                    esctester_set_servo_pwm(DEFAULT_FREQ, 25 * 3.2);
                    view_port_update(view_port);
                    furi_delay_ms(1000);
                    esctester_update_pwm();
                    armed = true;
                }
            } else if(event.input.key == InputKeyLeft && event.input.type == InputTypeRelease) {
                // small step on release for precise setting
                pWidthNew -= WIDTH_STEP_SMALL;
            } else if(event.input.key == InputKeyRight && event.input.type == InputTypeRelease) {
                // small step on release for precise setting
                pWidthNew += WIDTH_STEP_SMALL;
            } else if(event.input.key == InputKeyDown) {
                // big step on every event for fast scrolling
                pWidthNew -= WIDTH_STEP_BIG;
            } else if(event.input.key == InputKeyUp) {
                // big step on every event for fast scrolling
                pWidthNew += WIDTH_STEP_BIG;
            }
            view_port_update(view_port);
        } else if(event.type == EventTypeTick) {
            view_port_update(view_port);
        }

        if (pWidthNew < MIN_WIDTH) pWidthNew = MIN_WIDTH;
        if (pWidthNew > MAX_WIDTH) pWidthNew = MAX_WIDTH;

        if (pWidthNew != pWidth) {
            pWidth = pWidthNew;
            esctester_update_pwm();
        }
	view_port_update(view_port);
    }

    // first stop PWM on component
    furi_hal_pwm_stop(FuriHalPwmOutputIdTim1PA7);
    // second power off component
    furi_hal_power_disable_otg();

    furi_timer_free(timer);
    furi_message_queue_free(event_queue);

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}
