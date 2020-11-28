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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * external_dac.cpp - To set stepper current via DAC on Alligator board
 */

#include "../../../MK4duo.h"
#include "sanitycheck.h"

#if MB(ALLIGATOR_R2) || MB(ALLIGATOR_R3)

ExternalDac externaldac;

/** Public Function */
void ExternalDac::begin() {
  uint8_t externalDac_buf[2] = { 0x20, 0x00 };  // all off

  HAL::spiBegin();

  // init onboard DAC
  HAL::delayMicroseconds(2U);
  HAL::digitalWrite(DAC0_SYNC_PIN, LOW);
  HAL::delayMicroseconds(2U);
  HAL::digitalWrite(DAC0_SYNC_PIN, HIGH);
  HAL::delayMicroseconds(2U);
  HAL::digitalWrite(DAC0_SYNC_PIN, LOW);

  HAL::spiSend(SPI_CHAN_DAC, externalDac_buf , 2);
  HAL::digitalWrite(DAC0_SYNC_PIN, HIGH);

  // init Piggy DAC
  HAL::delayMicroseconds(2U);
  HAL::digitalWrite(DAC1_SYNC_PIN, LOW);
  HAL::delayMicroseconds(2U);
  HAL::digitalWrite(DAC1_SYNC_PIN, HIGH);
  HAL::delayMicroseconds(2U);
  HAL::digitalWrite(DAC1_SYNC_PIN, LOW);

  HAL::spiSend(SPI_CHAN_DAC, externalDac_buf, 2);
  HAL::digitalWrite(DAC1_SYNC_PIN, HIGH);

}

void ExternalDac::set_driver_current(Driver* act) {
  constexpr uint8_t dac_order[] = { 96, 94, 98, 5, 28, 11, 30 };
  const uint8_t digipot_motor = 255 * act->data.ma / 1000 / 3.3;
  uint8_t index = 0;
  for (index = 0; index < COUNT(dac_order); index++) {
    if (act->data.pin.step == dac_order[index]) break;
  }
  setValue(index, digipot_motor);
}

void ExternalDac::print_M906() {
  SERIAL_LM(CFG, "Stepper driver current (mA)");
  SERIAL_SM(CFG, "  M906");
  SERIAL_MV(" X", driver.x->data.ma);
  SERIAL_MV(" Y", driver.y->data.ma);
  SERIAL_MV(" Z", driver.z->data.ma);
  SERIAL_EOL();
  LOOP_DRV_EXT() {
    SERIAL_SM(CFG, "  M906");
    SERIAL_MV(" T", int(d));
    SERIAL_MV(" E", driver.e[extruders[d]->get_driver()]->data.ma);
    SERIAL_EOL();
  }
}

/** Private Function */
void ExternalDac::setValue(const uint8_t channel, uint8_t value) {

  if (channel >= 7) return; // max channel (X,Y,Z,E0,E1,E2,E3)

  uint8_t externalDac_buf[2] = { 0x10, 0x00 };

  if (channel > 3)
    externalDac_buf[0] |= ((7 - channel) << 6);
  else
    externalDac_buf[0] |= ((3 - channel) << 6);

  externalDac_buf[0] |= (value >> 4);
  externalDac_buf[1] |= (value << 4);

  if (channel > 3) { // DAC Piggy E1,E2,E3
    HAL::digitalWrite(DAC1_SYNC_PIN, LOW);
    HAL::delayMicroseconds(2U);
    HAL::digitalWrite(DAC1_SYNC_PIN, HIGH);
    HAL::delayMicroseconds(2U);
    HAL::digitalWrite(DAC1_SYNC_PIN, LOW);
  }
  else { // DAC onboard X,Y,Z,E0
    HAL::digitalWrite(DAC0_SYNC_PIN, LOW);
    HAL::delayMicroseconds(2U);
    HAL::digitalWrite(DAC0_SYNC_PIN, HIGH);
    HAL::delayMicroseconds(2U);
    HAL::digitalWrite(DAC0_SYNC_PIN, LOW);
  }

  HAL::delayMicroseconds(2U);
  HAL::spiSend(SPI_CHAN_DAC, externalDac_buf, 2);

  HAL::digitalWrite(DAC0_SYNC_PIN, HIGH);
  HAL::digitalWrite(DAC1_SYNC_PIN, HIGH);

}

#endif // MB(ALLIGATOR_R2) || MB(ALLIGATOR_R3)
