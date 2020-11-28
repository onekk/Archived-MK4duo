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
#pragma once

/**
 * hostaction.h
 *
 * Copyright (c) 2020 Alberto Cotronei @MagoKimbra
 */

class Host_Action {

  public: /** Constructor */

    Host_Action() {}

  private: /** Private Parameters */

    static HostPromptEnum prompt_reason;

  public: /** Public Function */

    static void response_handler(const uint8_t response);
    static void action_notify(const char * const message);
    static void action_notify_P(PGM_P const message);

    static void pause(const bool eol=true)  { print_action(PSTR("pause"), eol); }
    static void paused(const bool eol=true) { print_action(PSTR("paused"), eol); }
    static void resume()                    { print_action(PSTR("resume")); }
    static void resumed()                   { print_action(PSTR("resumed")); }
    static void cancel()                    { print_action(PSTR("cancel")); }
    static void power_off()                 { print_action(PSTR("poweroff")); }

    static void filrunout(const uint8_t t);

    static void prompt_begin(const HostPromptEnum reason, PGM_P const pstr, const char extra_char='\0');
    static void prompt_button(PGM_P const pstr);

    static void prompt_show() { print_prompt(PSTR("show")); }
    static void prompt_end()  { print_prompt(PSTR("end")); }

    static void prompt_do(const HostPromptEnum reason, PGM_P const pstr, PGM_P const btn1=nullptr, PGM_P const btn2=nullptr);

    static inline void prompt_open(const HostPromptEnum reason, PGM_P const pstr, PGM_P const btn1=nullptr, PGM_P const btn2=nullptr) {
      if (prompt_reason == PROMPT_NOT_DEFINED) prompt_do(reason, pstr, btn1, btn2);
    }

  private: /** Private Function */

    static void print_action(PGM_P const pstr, const bool eol=true);
    static void print_prompt(PGM_P const ptype, const bool eol=true);
    static void print_prompt_plus(PGM_P const ptype, PGM_P const pstr, const char extra_char='\0');

    static void filament_load_prompt();

};

extern Host_Action host_action;
