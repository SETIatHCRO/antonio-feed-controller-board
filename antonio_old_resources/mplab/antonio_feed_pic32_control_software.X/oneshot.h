
struct oneshot_timer {
    char *name;
    int32_t countdown_ticks;
    void (*callback)();
    struct oneshot_timer *next;
};

void init_oneshot();
void poll_oneshot_timers();
void start_timer(struct oneshot_timer *app_timer,
                 void *callback_function, uint32_t ticks);
void stop_timer(struct oneshot_timer *app_timer);

