
#undef MYTC74DEBUG

struct tc74 {
    char *name;
    uint8_t address;
    bool is_initialized;
    bool is_valid_temp;
    float temp_c;
};

void init_tc74();

unsigned int gettc74table(struct tc74 **tc74tblptr);

static void tc74_start_callback();
static void tc74_timeout_callback();

void poll_tc74();

static void tc74_devices_idle();
static void tc74_devices_start();
static void tc74_devices_start_wait();
static void tc74_devices_send_addr();
static void tc74_devices_send_addr_wait();
static void tc74_devices_send_read_cmnd();
static void tc74_devices_send_read_cmnd_wait();
static void tc74_devices_read_data();
static void tc74_devices_send_nack();
static void tc74_devices_send_nack_wait();
static void tc74_devices_stop();
static void tc74_devices_stop_wait();
static void tc74_devices_loop();
