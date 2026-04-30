#pragma once

#include <stdint.h>

typedef void *pl_rtc_handle_t;

pl_rtc_handle_t pl_rtc_get_handle(void);
void pl_rtc_init(void);
