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
 * mcode
 *
 * Copyright (c) 2020 Alberto Cotronei @MagoKimbra
 */

#if HAS_COLOR_LEDS

#define CODE_M150

/**
 * M150: Set Status LED Color - Use R-U-B-W for R-G-B-W
 *       and Brightness       - Use P (for NEOPIXEL only)
 *
 * Always sets all 3 or 4 components. If a component is left out, set to 0.
 *
 * Examples:
 *
 *   M150 R255       ; Turn LED red
 *   M150 R255 U127  ; Turn LED orange (PWM only)
 *   M150            ; Turn LED off
 *   M150 R U B      ; Turn LED white
 *   M150 W          ; Turn LED white using a white LED
 *   M150 P127       ; Set LED 50% brightness
 *   M150 P          ; Set LED full brightness
 */
inline void gcode_M150() {
  if (parser.seen('S')) {
    short_timer_t end_timer(millis());
    const uint8_t second = parser.value_byte();
    do {
      const uint8_t red   = random(256);
      const uint8_t green = random(256);
      const uint8_t blue  = random(256);
      leds.set_color(MakeLEDColor(red, green, blue, 0, 255)
        #if ENABLED(NEOPIXEL_LED) && ENABLED(NEOPIXEL_IS_SEQUENTIAL)
          , true
        #endif
      );
      HAL::delayMilliseconds(100);
    } while (end_timer.pending(SECOND_TO_MILLIS(second)));
  } 
  else {
    leds.set_color(MakeLEDColor(
      parser.seen('R') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
      parser.seen('U') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
      parser.seen('B') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
      parser.seen('W') ? (parser.has_value() ? parser.value_byte() : 255) : 0,
      parser.seen('P') ? (parser.has_value() ? parser.value_byte() : 255) : leds.getBrightness()
    ));
  }
}

#endif // HAS_COLOR_LEDS
