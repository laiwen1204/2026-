#include "scheduler.h"

typedef struct {
    void     (*task_func)(void);
    uint32_t  period_ms;
    uint32_t  last_run_time;
} task_t;

static uint16_t task_number;

static task_t scheduler[] = {
    { protocol_proc, 10,    0 },
    { ar_proc,       100,   0 },
    { led_blink,     500,   0 },
    { adc_proc,      200,   0 },
    { oled_proc,     200,   0 },
    { pt100_proc,    200,   0 },
    { alarm_proc,    200,   0 },
    { rtc_proc,      1000,  0 },
};

void scheduler_init(void)
{
    task_number = sizeof(scheduler) / sizeof(task_t);
}

void scheduler_run(void)
{
    uint16_t i;
    for (i = 0; i < task_number; i++) {
        uint32_t now_time = GetSysRunTime();
        if (now_time >= scheduler[i].last_run_time + scheduler[i].period_ms) {
            scheduler[i].last_run_time = now_time;
            scheduler[i].task_func();
        }
    }
}
