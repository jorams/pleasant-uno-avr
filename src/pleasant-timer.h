/*
 * Pleasant Timer allows you to configure the device's various timers, without
 * having to fiddle with all of the different registers associated with their
 * configuration.
 */

#ifndef PLEASANT_TIMER_H
#define PLEASANT_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/* Wave Generation Mode -------------------------------------------------------
 * The wave generation mode used by each timer is computed through the use of
 * the following two enums. The combination of values selected specifies which
 * WGM mode is used for the timer.
 *
 * Not all combinations are supported. If an unsupported combination of values
 * is selected, the timer's initialization function will return false.
 */

enum timer_wave_type {
  TIMER_WAVE_TYPE_NORMAL                          = 1,
  TIMER_WAVE_TYPE_PHASE_CORRECT_PWM               = 2,
  TIMER_WAVE_TYPE_FAST_PWM                        = 4,
  TIMER_WAVE_TYPE_PHASE_AND_FREQUENCY_CORRECT_PWM = 8 /* 16-bit specific */
};

enum timer_wrap_type {
  TIMER_WRAP_TYPE_8_BITS        = 16,
  TIMER_WRAP_TYPE_COMPARE_A     = 32,
  TIMER_WRAP_TYPE_9_BITS        = 64,  /* 16-bit specific */
  TIMER_WRAP_TYPE_10_BITS       = 128, /* 16-bit specific */
  TIMER_WRAP_TYPE_INPUT_CAPTURE = 256  /* 16-bit specific */
};

/* Compare Output Mode --------------------------------------------------------
 * When a timer's value matches a compare register, the value of an associated
 * Output Compare pin can be modified. The operation performed can be
 * configured.
 *
 * Note that when a PWM wave type is used, the behaviour of each operation is
 * much more complicated than its name suggests. Refer to the datasheet to
 * determine exactly what happens.
 */

enum timer_compare_output_mode {
  TIMER_COMPARE_OUTPUT_MODE_OFF    = 0,
  TIMER_COMPARE_OUTPUT_MODE_TOGGLE = 1,
  TIMER_COMPARE_OUTPUT_MODE_CLEAR  = 2,
  TIMER_COMPARE_OUTPUT_MODE_SET    = 3
};

/* Clock source ---------------------------------------------------------------
 * Each timer is updated according to an associated clock source, which is
 * either based on the processor's clock speed (F_CPU), or on an external
 * signal.
 */

enum timer_clock_source {
  TIMER_CLOCK_SOURCE_OFF              = 0,
  TIMER_CLOCK_SOURCE_DIV_1            = 1,
  TIMER_CLOCK_SOURCE_DIV_8            = 2,
  TIMER_CLOCK_SOURCE_DIV_64           = 3,
  TIMER_CLOCK_SOURCE_DIV_256          = 4,
  TIMER_CLOCK_SOURCE_DIV_1024         = 5,
  TIMER_CLOCK_SOURCE_EXTERNAL_FALLING = 6,
  TIMER_CLOCK_SOURCE_EXTERNAL_RISING  = 7
};

/* Timer interrupts -----------------------------------------------------------
 * Interrupts can be triggered at various times. They can be enabled by OR-ing
 * various timer_interrupt values together.
 */

enum timer_interrupt {
  TIMER_INTERRUPT_OFF           = 0,
  TIMER_INTERRUPT_OVERFLOW      = (1 << 0),
  TIMER_INTERRUPT_COMPARE_A     = (1 << 1),
  TIMER_INTERRUPT_COMPARE_B     = (1 << 2),
  TIMER_INTERRUPT_INPUT_CAPTURE = (1 << 5) /* 16-bit specific */
};

/* Input capture edge ---------------------------------------------------------
 * The 16-bit timer supports "input capture", allowing you to take a snapshot
 * of the timer's value by triggering the Input Capture pin. The edge at which
 * this snapshot is taken is configurable.
 *
 * Note that using the input capture wrap type causes input capture to be
 * disabled.
 */

enum timer_input_capture_edge {
  TIMER_INPUT_CAPTURE_EDGE_FALLING = 0,
  TIMER_INPUT_CAPTURE_EDGE_RISING  = 1
};

/* Configuration --------------------------------------------------------------
 * The timers can be configured by creating a timer configuration with the
 * desired values, then calling the timer's initialization function.
 */

struct timer_configuration {
  enum timer_wave_type           wave_type;
  enum timer_wrap_type           wrap_type;
  enum timer_clock_source        clock_source;
  enum timer_interrupt           interrupts;
  enum timer_compare_output_mode compare_output_mode_a;
  enum timer_compare_output_mode compare_output_mode_b;
  enum timer_input_capture_edge input_capture_edge; /* 16-bit specific */
  bool input_capture_noise_canceler_enabled;        /* 16-bit specific */
};

/*
 * Initialize timer 0, which is an 8-bit timer.
 */
bool timer0_init(struct timer_configuration configuration);
#define TIMER0_VALUE     TCNT0
#define TIMER0_COMPARE_A OCR0A
#define TIMER0_COMPARE_B OCR0B

/*
 * Initialize timer 1, which is a 16-bit timer.
 */
bool timer1_init(struct timer_configuration configuration);
#define TIMER1_VALUE         TCNT1
#define TIMER1_COMPARE_A     OCR1A
#define TIMER1_COMPARE_B     OCR1B
#define TIMER1_INPUT_CAPTURE ICR1

/*
 * Initialize timer 2, which is an 8-bit timer.
 */
bool timer2_init(struct timer_configuration configuration);
#define TIMER2_VALUE     TCNT2
#define TIMER2_COMPARE_A OCR2A
#define TIMER2_COMPARE_B OCR2B

#endif /* PLEASANT_TIMER_H */
