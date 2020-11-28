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

#define CODE_M81

/**
 * M81: Turn off Power, including Power Supply, if there is one.
 *
 *      This code should ALWAYS be available for FULL SHUTDOWN!
 */
inline void gcode_M81() {
  tempManager.disable_all_heaters();
  planner.finish_and_disable();

  #if HAS_FAN
    LOOP_FAN() {
      fans[f]->speed = 0;
      fans[f]->paused_speed = 0;
      fans[f]->setIdle(false);
    }
  #endif

  #if ENABLED(LASER)
    laser.extinguish();
    #if ENABLED(LASER_PERIPHERALS)
      laser.peripherals_off();
    #endif
  #endif

  #if ENABLED(CNCROUTER)
    cnc.disable_router();
  #endif

  HAL::delayMilliseconds(1000); // Wait 1 second before switching off

  #if HAS_SUICIDE
    printer.suicide();
  #elif HAS_POWER_SWITCH
    powerManager.power_off();
  #endif

  LCD_MESSAGEPGM_P(PSTR(MACHINE_NAME " " STR_OFF "."));

}
