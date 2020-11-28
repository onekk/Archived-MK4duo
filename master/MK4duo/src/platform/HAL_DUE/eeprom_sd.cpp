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

#ifdef ARDUINO_ARCH_SAM

#include "../../../MK4duo.h"

#if HAS_EEPROM_SD

char MemoryStore::eeprom_data[EEPROM_SIZE];

size_t  MemoryStore::capacity()     { return EEPROM_SIZE + 1; }
bool    MemoryStore::access_start() { return false; }
bool    MemoryStore::access_write() { card.write_eeprom(); return false; }

bool MemoryStore::write_data(int &pos, const uint8_t *value, size_t size, uint16_t *crc) {

  while (size--) {
    uint8_t v = *value;
    eeprom_data[pos] = v;
    crc16(crc, &v, 1);
    pos++;
    value++;
  };

  return false;
}

bool MemoryStore::read_data(int &pos, uint8_t *value, size_t size, uint16_t *crc, const bool writing/*=true*/) {

  while (size--) {
    uint8_t c = eeprom_data[pos];
    if (writing) *value = c;
    crc16(crc, &c, 1);
    pos++;
    value++;
  };

  return false;
}

#endif // HAS_EEPROM_SD
#endif // ARDUINO_ARCH_SAM
