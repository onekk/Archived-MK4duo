/**
 * MK4duo Firmware for 3D Printer, Laser and CNC
 *
 * Based on Marlin, Sprinter and grbl
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 * Copyright (c) 2020 Alberto Cotronei @MagoKimbra
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * laser.cpp - Laser control library for Arduino using 16 bit timers- Version 1
 * Copyright (c) 2013 Timothy Schmidt.  All right reserved.
 * Copyright (c) 2016 Franco (nextime) Lanza
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "../../../MK4duo.h"
#include "sanitycheck.h"

#if ENABLED(LASER)

Laser laser;

float     Laser::ppm          = 0.0;

uint8_t   Laser::intensity    = 255,
          Laser::mode         = CONTINUOUS;

uint32_t  Laser::duration     = 0,
          Laser::dur          = 0;

bool      Laser::status       = LASER_OFF,
          Laser::firing       = LASER_OFF,
          Laser::diagnostics  = false;

millis_l  Laser::last_firing  = 0;

uint16_t  Laser::time         = 0,
          Laser::lifetime     = 0;

#if ENABLED(LASER_RASTER)

  unsigned char Laser::raster_data[LASER_MAX_RASTER_LINE] = { 0 },
                Laser::rasterlaserpower                   = 0;

  float         Laser::raster_aspect_ratio  = 0.0,
                Laser::raster_mm_per_pulse  = 0.0;

  int           Laser::raster_raw_length    = 0,
                Laser::raster_num_pixels    = 0;

  uint8_t       Laser::raster_direction     = 0;

#endif

void Laser::init() {

  #if HAS_LASER_POWER
    HAL::pinMode(LASER_PWR_PIN, OUTPUT);
    #if HAS_LASER_PWM
      HAL::pinMode(LASER_PWM_PIN, OUTPUT);
      WRITE(LASER_PWR_PIN, LASER_UNARM);    // Laser FIRING is active LOW, so preset the pin
    #endif
  #endif

  #if ENABLED(LASER_PERIPHERALS)
    OUT_WRITE(LASER_PERIPHERALS_PIN, HIGH); // Laser peripherals are active LOW, so preset the pin
    SET_INPUT_PULLUP(LASER_PERIPHERALS_STATUS_PIN);   // Set the peripherals status pin to pull-up.
  #endif // LASER_PERIPHERALS

  #if ENABLED(LASER_RASTER)
    laser.raster_aspect_ratio = LASER_RASTER_ASPECT_RATIO;
    laser.raster_mm_per_pulse = LASER_RASTER_MM_PER_PULSE;
    laser.raster_direction = 1;
  #endif // LASER_RASTER

  laser.extinguish();

}

void Laser::set_power() {

  #if ENABLED(INTENSITY_IN_BYTE)
    if (parser.seenval('S')) intensity = parser.value_byte();
  #else
    if (parser.seenval('S')) {
      float pwr = parser.value_float();
      LIMIT(pwr, 0 , 100.0f);
      intensity = 255 * pwr * 0.01f;
    }
  #endif

  if (parser.seenval('L')) duration     = parser.value_ulong();
  if (parser.seenval('P')) ppm          = parser.value_float();
  if (parser.seenval('D')) diagnostics  = parser.value_bool();

  if (parser.seenval('B')) set_mode(parser.value_int());

  status = LASER_ON;

}

void Laser::fire(uint8_t intensity/*=255*/) {

  laser.firing = LASER_ON;
  laser.last_firing = micros(); // microseconds of last laser firing

  #if ENABLED(LASER_PWM_INVERT)
    intensity = 255 - intensity;
  #endif

  #if LASER_CONTROL == 1
    HAL::analogWrite(LASER_PWR_PIN, intensity); // Range 0-255
  #elif LASER_CONTROL == 2
    HAL::analogWrite(LASER_PWM_PIN, intensity); // Range 0-255
    HAL::digitalWrite(LASER_PWR_PIN, LASER_ARM);
  #endif

  if (laser.diagnostics) SERIAL_EM("Laser fired");

}

void Laser::extinguish() {

  if (laser.firing == LASER_ON) {

    laser.firing = LASER_OFF;

    if (laser.diagnostics) SERIAL_EM("Laser being extinguished");

    #if LASER_CONTROL == 1
      #if ENABLED(LASER_PWM_INVERT)
        HAL::analogWrite(LASER_PWR_PIN, 255);
      #else
        HAL::analogWrite(LASER_PWR_PIN, 0);
      #endif
    #elif LASER_CONTROL == 2
      #if ENABLED(LASER_PWM_INVERT)
        HAL::analogWrite(LASER_PWM_PIN, 255);
      #else
        HAL::analogWrite(LASER_PWM_PIN, 0);
      #endif
      HAL::digitalWrite(LASER_PWR_PIN, LASER_UNARM);
    #endif

    laser.time += millis() - (laser.last_firing / 1000UL);

    if (laser.diagnostics)
      SERIAL_EM("Laser extinguished");

  }
}

void Laser::set_mode(uint8_t pmode) {
  LIMIT(pmode, CONTINUOUS, RASTER);
  mode = pmode;
}

#if ENABLED(LASER_PERIPHERALS)

  bool Laser::peripherals_ok() { return !HAL::digitalRead(LASER_PERIPHERALS_STATUS_PIN); }

  void Laser::peripherals_on() {
    HAL::digitalWrite(LASER_PERIPHERALS_PIN, LOW);
    if (laser.diagnostics)
      SERIAL_EM("Laser Peripherals Enabled");
  }

  void Laser::peripherals_off() {
    if (!peripherals_ok()) {
      HAL::digitalWrite(LASER_PERIPHERALS_PIN, HIGH);
      if (laser.diagnostics)
        SERIAL_EM("Laser Peripherals Disabled");
    }
  }

  void Laser::wait_for_peripherals() {
    unsigned long timeout = millis() + LASER_PERIPHERALS_TIMEOUT;
    if (laser.diagnostics)
      SERIAL_EM("Waiting for peripheral control board signal...");

    while(!peripherals_ok()) {
      if (millis() > timeout) {
        if (laser.diagnostics)
          SERIAL_LM(ER, "Peripheral control board failed to respond");

        printer.stop();
        break;
      }
    }
  }

#endif // LASER_PERIPHERALS

#endif // ENABLED(LASER) && ENABLED(ARDUINO_ARCH_SAM)
