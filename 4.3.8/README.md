#Archive copy of MK4Duo as it is dead and maybe it will disappear.

## MK4duo 3D Printer Firmware for all Atmel AVR boards and Arduino Due

## Version 4.3.8 revision 20022019

### New features are:
* One version for all Atmel AVR boards and for Arduino Due or other boards based on Atmel SAM3X8E
* Stepping-algorithm optmized now for DRV8825 and A4988 (no need for double or quadstepping; no delays)
* High speed stepping of approx. 295.000 steps/s, if needed (maybe more with less DOUBLE_STEP_FREQUENCY?)

---
# MK4duo 3D Printer Firmware
  * [Configuration & Compilation](/Documentation/Compilation.md)
  * Supported
    * [Features](/Documentation/Features.md)
    * [Hardware](/Documentation/Hardware.md)
    * [GCodes](/Documentation/GCodes.md)
  * Notes
    * [Bresenham Algorithm](/Documentation/Bresenham.md)
    * [Auto Bed Leveling](/Documentation/BedLeveling.md)
    * [Filament Sensor](/Documentation/FilamentSensor.md)
    * [Ramps Servo Power](/Documentation/RampsServoPower.md)
    * [LCD Language - Font - System](Documentation/LCDLanguageFont.md)
  * Version
    * [Change Log](/Documentation/changelog.md)

## Credits
- [MagoKimbra - Alberto Cotronei](https://github.com/MagoKimbra)

## License

MK4duo is published under the [GPL license](/Documentation/COPYING.md) because I believe in open development.
Do not use this code in products (3D printers, CNC etc) that are closed source or are crippled by a patent.
