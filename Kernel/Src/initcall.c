#include "initcall.h"
#include "main.h"

/* 早期初始化函数声明（由 Platform 层提供） */
extern void MPU_Config(void);
extern void SystemClock_Config(void);
extern void PeriphCommonClock_Config(void);

void board_init(void)
{
    /* 阶段 1: 硬件关键路径 — 必须严格按序手动调用 */
    MPU_Config();
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();

    /* 阶段 2: 遍历 initcall section，自动执行所有注册的初始化 */
    uint32_t count = (uint32_t)(__initcall_end - __initcall_start);
    for (uint32_t i = 0; i < count; i++) {
        if (__initcall_start[i].fn)
            __initcall_start[i].fn();
    }
}
