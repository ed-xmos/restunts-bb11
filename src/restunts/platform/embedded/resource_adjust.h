// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <xcore/port.h>

// Macro to adjust input pad sample timing for the round trip delay. Supports 0 (default) to 5 core clock cycles.
// Larger numbers increase hold time but reduce setup time.
#define PORT_DELAY      0x7007
#define _DELAY_SHIFT    0x3
#define set_pad_sample_delay(port, delay)  {__xcore_resource_setc((port), ((delay) << _DELAY_SHIFT) | PORT_DELAY);}

// Macro to adjust input pad capture clock edge
#define PORT_SAMPLE     0x4007
#define _SAMPLE_SHIFT   0x3
#define SAMPLE_RISING   0x0 // Default for input port. Reduces setup time, increases hold time.
#define SAMPLE_FALLING  0x1 // Increases setup time, reduces hold time
#define set_pad_sample_edge(port, edge)  {__xcore_resource_setc((port), ((edge) << _SAMPLE_SHIFT) | PORT_SAMPLE);}


// Pad control defines
#define PAD_CONTROL     0x0006
#define DRIVE_2MA       0x0
#define DRIVE_4MA       0x1
#define DRIVE_8MA       0x2
#define DRIVE_12MA      0x3
#define _DRIVE_SHIFT     20
#define ENABLE_SLEW     (1 << 22)
#define ENABLE_SCHMITT  (1 << 23)
#define PAD_DRIVE_MODE  0x0003
#define DRIVE_BOTH      0x0
#define DRIVE_HIGH      0x1
#define DRIVE_LOW       0x2
#define _MODE_SHIFT     0

// Macro to adjust the pad output drive strength
#define set_pad_drive_strength(port, strength)  {__xcore_resource_setc((port), ((strength) << _DRIVE_SHIFT) | PAD_CONTROL);}
// Macro to enable the schmitt input
#define set_pad_input_schmitt(port)  {__xcore_resource_setc((port), ENABLE_SCHMITT | PAD_CONTROL);}
#define set_pad_output_slew(port)  {__xcore_resource_setc((port), ENABLE_SLEW | PAD_CONTROL);}
// Macro to enable the drive mode (open drain/source or complementary)
#define set_pad_drive_mode(port, mode)  {__xcore_resource_setc((port), ((mode) << _MODE_SHIFT) | PAD_DRIVE_MODE);}

// Macro to adjust the clock block rise or fall delays up to 512 core clocks
#define set_clock_rise_delay_asm(clk, delay)  {__xcore_resource_setc((clk), ((delay) << 3) | 0x9007);}
#define set_clock_fall_delay_asm(clk, delay)  {__xcore_resource_setc((clk), ((delay) << 3) | 0x8007);}