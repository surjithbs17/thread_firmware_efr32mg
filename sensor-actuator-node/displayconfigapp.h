// Copyright 2016 Silicon Laboratories, Inc.

// This is the application level configuration file used by the graphics
// library to control and override certain compile time options used by the
// library.


#ifndef _DISPLAY_CONFIG_APP_H_
#define _DISPLAY_CONFIG_APP_H_

// Include pixel matrix allocation support.
#define PIXEL_MATRIX_ALLOC_SUPPORT

// Enable allocation of pixel matrices from the static pixel matrix pool.
// NOTE:
// The allocator does not support free'ing pixel matrices. It allocates
// continuosly from the static pool without keeping track of the sizes of
// old allocations. I.e. this is a one-shot allocator, and the  user should
// allocate buffers once at the beginning of the program.
//
#define USE_STATIC_PIXEL_MATRIX_POOL

// Specify the size of the static pixel matrix pool. For the sensuator demo
// we need one pixel matrix (framebuffer) covering the whole display.
#define PIXEL_MATRIX_POOL_SIZE   (DISPLAY0_HEIGHT * DISPLAY0_WIDTH/8)

// On the EFR32MG WSTK, the DISPLAY driver Platform Abstraction Layer (PAL)
// uses the RTC to time and toggle the EXTCOMIN pin of the Sharp memory
// LCD per default. However, the some examples need to use the RTC to
// keep track of time, i.e. generate interrupt every second. Therefore
// the those examples implements a function (rtcIntCallbackRegister)) that
// enables the PAL to register the callback function that needs to be called in
// order to toggle the EXTCOMIN pin.
// I.e we need to undefine the INCLUDE_PAL_GPIO_PIN_AUTO_TOGGLE (which
// is defined by default and uses the RTC) and we need to define the
// PAL_TIMER_REPEAT_FUNCTION. */
#undef INCLUDE_PAL_GPIO_PIN_AUTO_TOGGLE
#define PAL_TIMER_REPEAT_FUNCTION  (rtcIntCallbackRegister)
extern int rtcIntCallbackRegister (void(*pFunction)(void*),
                                   void* argument,
                                   unsigned int frequency);
#endif //_DISPLAY_CONFIG_APP_H_
