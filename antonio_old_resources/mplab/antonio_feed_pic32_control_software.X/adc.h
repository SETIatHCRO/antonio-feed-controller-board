
#undef MYADCDEBUG

struct adc {
    char *name;
    uint8_t address;
    bool is_valid_value;
    float value;  // value out of 1024, not converted to units
};

void init_adc();

unsigned int getadctable(struct adc **adctblptr);

static void adc_start_callback();
static void adc_convert_callback();

void poll_adc();

static void adc_idle();
static void adc_acquire();
static void adc_acquire_wait();
static void adc_convert();
static void adc_done_read();
static void adc_loop();

#ifdef MYADCDEBUG
void dump_adc_table();
#endif
