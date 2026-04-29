#pragma once

#include <stdint.h>

typedef void (*initcall_fn)(void);

typedef struct {
    initcall_fn fn;
    const char *name;
} initcall_entry_t;

#define OS_INITCALL(lvl, fn) \
    static const initcall_entry_t __attribute__((used, section(".initcall." #lvl))) \
    __initcall_##lvl##_##fn = { (initcall_fn)(fn), #fn }

#define arch_initcall(fn)     OS_INITCALL(1, fn)
#define subsys_initcall(fn)   OS_INITCALL(2, fn)
#define device_initcall(fn)   OS_INITCALL(3, fn)
#define driver_initcall(fn)   OS_INITCALL(4, fn)
#define late_initcall(fn)     OS_INITCALL(5, fn)

extern initcall_entry_t __initcall_start[];
extern initcall_entry_t __initcall_end[];

void board_init(void);
