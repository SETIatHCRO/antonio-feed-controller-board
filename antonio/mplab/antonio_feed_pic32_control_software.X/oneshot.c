#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdio.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>

#include "oneshot.h"

// used to calculate ticks (milliseconds) since last oneshot timer poll
static uint32_t last_oneshot_timer_poll_tickcount;

static struct oneshot_timer *timer_list = NULL;

void init_oneshot() {
    timer_list = NULL;

    last_oneshot_timer_poll_tickcount = GetTickCount();
}

void poll_oneshot_timers() {
    uint32_t current_tickcount = GetTickCount();
    uint32_t elapsed_ticks =
        current_tickcount - last_oneshot_timer_poll_tickcount;
    last_oneshot_timer_poll_tickcount = current_tickcount;
    struct oneshot_timer *timer_ptr = timer_list;

    if (elapsed_ticks == 0) {
        return;
    }

    while (timer_ptr) {
        if ((timer_ptr -> countdown_ticks) > 0) {
            timer_ptr -> countdown_ticks -= (int32_t) elapsed_ticks;
            if (!((timer_ptr -> countdown_ticks) > 0)) {
                timer_ptr -> countdown_ticks = 0;
                timer_ptr -> callback();
            }
        }
        timer_ptr = timer_ptr -> next;
    }
}

void start_timer(struct oneshot_timer *app_timer,
                 void *callback_function, uint32_t ticks) {
    struct oneshot_timer *timer_ptr = timer_list;

    // check if timer already in list
    while (timer_ptr) {
        if (timer_ptr == app_timer) {
            break;
        }
        timer_ptr = timer_ptr -> next;
    }

    // if not in list, add timer to beginning
    if (!(timer_ptr)) {
        app_timer -> next = timer_list;
        timer_list = app_timer;
        timer_ptr = timer_list;
    }

    timer_ptr -> countdown_ticks = ticks +
            (GetTickCount() - last_oneshot_timer_poll_tickcount);
    timer_ptr -> callback = callback_function;
}

/*
 * remove timer from list
 */
void stop_timer(struct oneshot_timer *app_timer) {
    struct oneshot_timer *timer_ptr = timer_list;

    if (timer_list == app_timer) {
        timer_list = timer_list -> next;
    }
    else {
        while (timer_ptr -> next) {
            if ((timer_ptr -> next) == app_timer) {
                timer_ptr -> next = timer_ptr -> next -> next;
                break;
            }
            timer_ptr = timer_ptr -> next;
        }
    }
}
