#include <avr/io.h>
#include "pleasant-timer.h"

static const uint8_t wave_error = 0xFF;

/* 8-bit timers ------------------------------------------------------------ */

static uint8_t wgm_mode_8_bits(enum timer_wave_type wave_type,
                               enum timer_wrap_type wrap_type) {
  switch (wave_type | wrap_type) {
  case (TIMER_WAVE_TYPE_NORMAL | TIMER_WRAP_TYPE_8_BITS):
    return 0;
  case (TIMER_WAVE_TYPE_PHASE_CORRECT_PWM | TIMER_WRAP_TYPE_8_BITS):
    return 1;
  case (TIMER_WAVE_TYPE_NORMAL | TIMER_WRAP_TYPE_COMPARE_A):
    return 2;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_8_BITS):
    return 3;
  case (TIMER_WAVE_TYPE_PHASE_CORRECT_PWM | TIMER_WRAP_TYPE_COMPARE_A):
    return 5;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_COMPARE_A):
    return 7;
  default:
    return wave_error;
  }
}

bool timer0_init(struct timer_configuration configuration) {
  TCCR0A = 0;
  TCCR0B = 0;

  TIMSK0 = configuration.interrupts;
  TCCR0B |= configuration.clock_source;

  TCCR0A |= (configuration.compare_output_mode_a << 6);
  TCCR0A |= (configuration.compare_output_mode_b << 4);

  uint8_t wgm = wgm_mode_8_bits(configuration.wave_type,
                                configuration.wrap_type);
  if (wgm == wave_error) return false;

  TCCR0A |= (wgm & (1 << 0) ? (1 << WGM00) : 0);
  TCCR0A |= (wgm & (1 << 1) ? (1 << WGM01) : 0);
  TCCR0B |= (wgm & (1 << 2) ? (1 << WGM02) : 0);

  return true;
}

bool timer2_init(struct timer_configuration configuration) {
  TCCR2A = 0;
  TCCR2B = 0;

  TIMSK2 = configuration.interrupts;
  TCCR2B |= configuration.clock_source;

  TCCR2A |= (configuration.compare_output_mode_a << 6);
  TCCR2A |= (configuration.compare_output_mode_b << 4);

  uint8_t wgm = wgm_mode_8_bits(configuration.wave_type,
                                configuration.wrap_type);
  if (wgm == wave_error) return false;

  TCCR2A |= (wgm & (1 << 0) ? (1 << WGM20) : 0);
  TCCR2A |= (wgm & (1 << 1) ? (1 << WGM21) : 0);
  TCCR2B |= (wgm & (1 << 2) ? (1 << WGM22) : 0);

  return true;
}

/* 16-bit timers ----------------------------------------------------------- */

static uint8_t wgm_mode_16_bits(enum timer_wave_type wave_type,
                                enum timer_wrap_type wrap_type) {
  switch (wave_type | wrap_type) {
  case (TIMER_WAVE_TYPE_PHASE_CORRECT_PWM | TIMER_WRAP_TYPE_10_BITS):
    return 3;
  case (TIMER_WAVE_TYPE_NORMAL | TIMER_WRAP_TYPE_COMPARE_A):
    return 4;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_8_BITS):
    return 5;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_9_BITS):
    return 6;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_10_BITS):
    return 7;
  case (TIMER_WAVE_TYPE_PHASE_AND_FREQUENCY_CORRECT_PWM
        | TIMER_WRAP_TYPE_INPUT_CAPTURE):
    return 8;
  case (TIMER_WAVE_TYPE_PHASE_AND_FREQUENCY_CORRECT_PWM
        | TIMER_WRAP_TYPE_COMPARE_A):
    return 9;
  case (TIMER_WAVE_TYPE_PHASE_CORRECT_PWM | TIMER_WRAP_TYPE_INPUT_CAPTURE):
    return 10;
  case (TIMER_WAVE_TYPE_PHASE_CORRECT_PWM | TIMER_WRAP_TYPE_COMPARE_A):
    return 11;
  case (TIMER_WAVE_TYPE_NORMAL | TIMER_WRAP_TYPE_INPUT_CAPTURE):
    return 12;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_INPUT_CAPTURE):
    return 14;
  case (TIMER_WAVE_TYPE_FAST_PWM | TIMER_WRAP_TYPE_COMPARE_A):
    return 15;
  default:
    return wave_error;
  }
}

bool timer1_init(struct timer_configuration configuration) {
  TCCR1A = 0;
  TCCR1B = 0;

  TIMSK1 = configuration.interrupts;
  TCCR1B |= configuration.clock_source;

  TCCR1A |= (configuration.compare_output_mode_a << 6);
  TCCR1A |= (configuration.compare_output_mode_b << 4);

  TCCR1B |= (configuration.input_capture_edge << 6);
  TCCR1B |= (configuration.input_capture_noise_canceler_enabled << 7);

  uint8_t wgm = wgm_mode_16_bits(configuration.wave_type,
                                 configuration.wrap_type);
  if (wgm == wave_error) return false;

  TCCR1A |= (wgm & (1 << 0) ? (1 << WGM10) : 0);
  TCCR1A |= (wgm & (1 << 1) ? (1 << WGM11) : 0);
  TCCR1B |= (wgm & (1 << 2) ? (1 << WGM12) : 0);
  TCCR1B |= (wgm & (1 << 3) ? (1 << WGM13) : 0);

  return true;
}
