#ifndef I_ITCM_H
#define I_ITCM_H

// Place hot rendering functions in a dedicated section so they're
// colocated in memory. This improves instruction cache utilization
// on Cortex-M7's small (4-16KB) icache by keeping the hottest code
// together instead of scattered across the binary.
#if TARGET_PLAYDATE && !TARGET_SIMULATOR
#define HOT_FUNC __attribute__((section(".text.hot")))
#else
#define HOT_FUNC
#endif

#endif
