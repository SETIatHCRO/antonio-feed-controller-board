#ifndef ACCEL_1_H
#define	ACCEL_1_H

#ifdef	__cplusplus
extern "C" {
#endif

#undef MYACCELDEBUG

struct accel_reg_op {
    uint8_t num;
    uint8_t rw;  // 0 - write, 1 - read
    uint8_t val;
    bool is_chained;
};

void getaccel_command(char *args[]);

void init_accel();

void get_accel_process(char *args[]);

static void get_accel_process_raw(char *args[]);

static void accel_timeout_callback();

void poll_accel();

static void accel_idle();
static void accel_start();
static void accel_wait();
static void accel_send_addr();
static void accel_send_addr_wait();
static void accel_send_reg();
static void accel_send_reg_wait();
static void accel_write_data();
static void accel_write_data_wait();
static void accel_repeat_start();
static void accel_repeat_start_wait();
static void accel_resend_addr();
static void accel_resend_addr_wait();
static void accel_read_data();
static void accel_send_ack();
static void accel_send_ack_wait();
static void accel_send_nack();
static void accel_send_nack_wait();
static void accel_stop();
static void accel_stop_wait();
static void accel_loop();
static void accel_accumulate_stats();
void accelonesec_command(char *args[]);


#ifdef	__cplusplus
}
#endif

#endif	/* ACCEL_1_H */