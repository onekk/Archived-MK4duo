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
 * Czech
 * UTF-8 for Graphical Display
 *
 * LCD Menu Messages
 *
 * Translated by Petr Zahradnik, Computer Laboratory
 * Blog and video blog Zahradnik se bavi
 * http://www.zahradniksebavi.cz
 *
 */

#define DISPLAY_CHARSET_ISO10646_CZ

namespace language_cz {
  using namespace language_en; // Inherit undefined strings from English

  constexpr uint8_t CHARSIZE                        = 2;
  FSTRINGVALUE(LANGUAGE                             , _UxGT("Czech"));

  FSTRINGVALUE(WELCOME_MSG                          , MACHINE_NAME _UxGT(" pripraven."));
  FSTRINGVALUE(MSG_YES                              , _UxGT("ANO"));
  FSTRINGVALUE(MSG_NO                               , _UxGT("NE"));
  FSTRINGVALUE(MSG_BACK                             , _UxGT("Zpet"));
  FSTRINGVALUE(MSG_MEDIA_INSERTED                   , _UxGT("Karta vlo�ena"));
  FSTRINGVALUE(MSG_MEDIA_REMOVED                    , _UxGT("Karta vyjmuta"));
  FSTRINGVALUE(MSG_LCD_ENDSTOPS                     , _UxGT("Endstopy")); // max 8 znaku
  FSTRINGVALUE(MSG_LCD_SOFT_ENDSTOPS                , _UxGT("Soft Endstopy"));
  FSTRINGVALUE(MSG_MAIN                             , _UxGT("Hlavn� nab�dka"));
  FSTRINGVALUE(MSG_ADVANCED_SETTINGS                , _UxGT("Dal�� nastaven�"));
  FSTRINGVALUE(MSG_CONFIGURATION                    , _UxGT("Konfigurace"));
  FSTRINGVALUE(MSG_AUTOSTART                        , _UxGT("Autostart"));
  FSTRINGVALUE(MSG_DISABLE_STEPPERS                 , _UxGT("Uvolnit motory"));
  FSTRINGVALUE(MSG_DEBUG_MENU                       , _UxGT("Nab�dka laden�"));
  #if LCD_WIDTH >= 20
    FSTRINGVALUE(MSG_PROGRESS_BAR_TEST              , _UxGT("Test ukaz. prubehu"));
  #else
    FSTRINGVALUE(MSG_PROGRESS_BAR_TEST              , _UxGT("Test uk. prubehu"));
  #endif
  FSTRINGVALUE(MSG_AUTO_HOME                        , _UxGT("Domovsk� pozice"));
  FSTRINGVALUE(MSG_AUTO_HOME_X                      , _UxGT("Domu osa X"));
  FSTRINGVALUE(MSG_AUTO_HOME_Y                      , _UxGT("Domu osa Y"));
  FSTRINGVALUE(MSG_AUTO_HOME_Z                      , _UxGT("Domu osa Z"));
  FSTRINGVALUE(MSG_AUTO_Z_ALIGN                     , _UxGT("Auto srovn�n� Z"));
  FSTRINGVALUE(MSG_LEVEL_BED_HOMING                 , _UxGT("Meren� podlo�ky"));
  FSTRINGVALUE(MSG_LEVEL_BED_WAITING                , _UxGT("Kliknut�m spustte"));
  FSTRINGVALUE(MSG_LEVEL_BED_NEXT_POINT             , _UxGT("Dal�� bod"));
  FSTRINGVALUE(MSG_LEVEL_BED_DONE                   , _UxGT("Meren� hotovo!"));
  FSTRINGVALUE(MSG_Z_FADE_HEIGHT                    , _UxGT("V��ka srovn�v�n�"));
  FSTRINGVALUE(MSG_SET_HOME_OFFSETS                 , _UxGT("Nastavit ofsety"));
  FSTRINGVALUE(MSG_HOME_OFFSETS_APPLIED             , _UxGT("Ofsety nastaveny"));
  FSTRINGVALUE(MSG_SET_ORIGIN                       , _UxGT("Nastavit poc�tek"));
  FSTRINGVALUE(MSG_PREHEAT_1                        , _UxGT("Zahr�t ") PREHEAT_1_LABEL);
  FSTRINGVALUE(MSG_PREHEAT_1_H                      , _UxGT("Zahr�t ") PREHEAT_1_LABEL " H");
  FSTRINGVALUE(MSG_PREHEAT_1_END                    , _UxGT("Zahr�t ") PREHEAT_1_LABEL _UxGT(" end"));
  FSTRINGVALUE(MSG_PREHEAT_1_END_E                  , _UxGT("Zahr�t ") PREHEAT_1_LABEL _UxGT(" end E"));
  FSTRINGVALUE(MSG_PREHEAT_1_ALL                    , _UxGT("Zahr�t ") PREHEAT_1_LABEL _UxGT(" v�e"));
  FSTRINGVALUE(MSG_PREHEAT_1_BEDONLY                , _UxGT("Zahr�t ") PREHEAT_1_LABEL _UxGT(" podlo�"));
  FSTRINGVALUE(MSG_PREHEAT_1_SETTINGS               , _UxGT("Zahr�t ") PREHEAT_1_LABEL _UxGT(" nast"));
  FSTRINGVALUE(MSG_PREHEAT_2                        , _UxGT("Zahr�t ") PREHEAT_2_LABEL);
  FSTRINGVALUE(MSG_PREHEAT_2_H                      , _UxGT("Zahr�t ") PREHEAT_2_LABEL " H");
  FSTRINGVALUE(MSG_PREHEAT_2_END                    , _UxGT("Zahr�t ") PREHEAT_2_LABEL _UxGT(" end"));
  FSTRINGVALUE(MSG_PREHEAT_2_END_E                  , _UxGT("Zahr�t ") PREHEAT_2_LABEL _UxGT(" end E"));
  FSTRINGVALUE(MSG_PREHEAT_2_ALL                    , _UxGT("Zahr�t ") PREHEAT_2_LABEL _UxGT(" v�e"));
  FSTRINGVALUE(MSG_PREHEAT_2_BEDONLY                , _UxGT("Zahr�t ") PREHEAT_2_LABEL _UxGT(" podlo�"));
  FSTRINGVALUE(MSG_PREHEAT_2_SETTINGS               , _UxGT("Zahr�t ") PREHEAT_2_LABEL _UxGT(" nast"));
  FSTRINGVALUE(MSG_PREHEAT_CUSTOM                   , _UxGT("Zahr�t vlastn�"));
  FSTRINGVALUE(MSG_COOLDOWN                         , _UxGT("Zchladit"));
  FSTRINGVALUE(MSG_LASER_MENU                       , _UxGT("Ovl�d�n� laseru"));
  FSTRINGVALUE(MSG_LASER_OFF                        , _UxGT("Vypnout laser"));
  FSTRINGVALUE(MSG_LASER_ON                         , _UxGT("Zapnout laser"));
  FSTRINGVALUE(MSG_LASER_POWER                      , _UxGT("V�kon laseru"));
  FSTRINGVALUE(MSG_SPINDLE_REVERSE                  , _UxGT("Vreteno opacne"));
  FSTRINGVALUE(MSG_SWITCH_PS_ON                     , _UxGT("Zapnout nap�jen�"));
  FSTRINGVALUE(MSG_SWITCH_PS_OFF                    , _UxGT("Vypnout nap�jen�"));
  FSTRINGVALUE(MSG_EXTRUDE                          , _UxGT("Vytlacit (extr.)"));
  FSTRINGVALUE(MSG_RETRACT                          , _UxGT("Zatlacit (retr.)"));
  FSTRINGVALUE(MSG_MOVE_AXIS                        , _UxGT("Posunout osy"));
  FSTRINGVALUE(MSG_BED_LEVELING                     , _UxGT("Vyrovnat podlo�ku"));
  FSTRINGVALUE(MSG_LEVEL_BED                        , _UxGT("Vyrovnat podlo�ku"));
  FSTRINGVALUE(MSG_LEVEL_CORNERS                    , _UxGT("Vyrovnat rohy"));
  FSTRINGVALUE(MSG_NEXT_CORNER                      , _UxGT("Dal�� roh"));
  FSTRINGVALUE(MSG_EDIT_MESH                        , _UxGT("Upravit s�t bodu"));
  FSTRINGVALUE(MSG_EDITING_STOPPED                  , _UxGT("Konec �prav s�te"));
  FSTRINGVALUE(MSG_MESH_X                           , _UxGT("Index X"));
  FSTRINGVALUE(MSG_MESH_Y                           , _UxGT("Index Y"));
  FSTRINGVALUE(MSG_MESH_EDIT_Z                      , _UxGT("Hodnota Z"));

  FSTRINGVALUE(MSG_USER_MENU                        , _UxGT("Vlastn� pr�kazy"));
  FSTRINGVALUE(MSG_IDEX_MENU                        , _UxGT("Re�im IDEX"));
  FSTRINGVALUE(MSG_OFFSETS_MENU                     , _UxGT("Ofsety n�stroju"));
  FSTRINGVALUE(MSG_IDEX_MODE_AUTOPARK               , _UxGT("Auto-Park"));
  FSTRINGVALUE(MSG_IDEX_MODE_DUPLICATE              , _UxGT("Duplikace"));
  FSTRINGVALUE(MSG_IDEX_MODE_MIRRORED_COPY          , _UxGT("Zrcadlen�"));
  FSTRINGVALUE(MSG_IDEX_MODE_FULL_CTRL              , _UxGT("Pln� kontrola"));
  FSTRINGVALUE(MSG_X_OFFSET                         , _UxGT("2. tryska X"));
  FSTRINGVALUE(MSG_Y_OFFSET                         , _UxGT("2. tryska Y"));
  FSTRINGVALUE(MSG_Z_OFFSET                         , _UxGT("2. tryska Z"));

  FSTRINGVALUE(MSG_UBL_DOING_G29                    , _UxGT("Prov�d�m G29"));
  FSTRINGVALUE(MSG_UBL_TOOLS                        , _UxGT("UBL n�stroje"));
  FSTRINGVALUE(MSG_UBL_LEVEL_BED                    , _UxGT("Unified Bed Leveling"));
  FSTRINGVALUE(MSG_UBL_MANUAL_MESH                  , _UxGT("Manu�ln� s�t bodu"));
  FSTRINGVALUE(MSG_UBL_BC_INSERT                    , _UxGT("Vlo�te kartu, zmerte"));
  FSTRINGVALUE(MSG_UBL_BC_INSERT2                   , _UxGT("Zmerte"));
  FSTRINGVALUE(MSG_UBL_BC_REMOVE                    , _UxGT("Odstrante a zmerte"));
  FSTRINGVALUE(MSG_UBL_MOVING_TO_NEXT               , _UxGT("Presun na dal��"));
  FSTRINGVALUE(MSG_UBL_ACTIVATE_MESH                , _UxGT("Aktivovat UBL"));
  FSTRINGVALUE(MSG_UBL_DEACTIVATE_MESH              , _UxGT("Deaktivovat UBL"));
  FSTRINGVALUE(MSG_UBL_SET_TEMP_BED                 , _UxGT("Teplota podlo�ky"));
  FSTRINGVALUE(MSG_UBL_BED_TEMP_CUSTOM              , _UxGT("Teplota podlo�ky"));
  FSTRINGVALUE(MSG_UBL_SET_TEMP_HOTEND              , _UxGT("Teplota hotendu"));
  FSTRINGVALUE(MSG_UBL_HOTEND_TEMP_CUSTOM           , _UxGT("Teplota hotendu"));
  FSTRINGVALUE(MSG_UBL_MESH_EDIT                    , _UxGT("�prava s�te bodu"));
  FSTRINGVALUE(MSG_UBL_EDIT_CUSTOM_MESH             , _UxGT("Upravit vlastn� s�t"));
  FSTRINGVALUE(MSG_UBL_FINE_TUNE_MESH               , _UxGT("Doladit s�t bodu"));
  FSTRINGVALUE(MSG_UBL_DONE_EDITING_MESH            , _UxGT("Konec �prav s�te"));
  FSTRINGVALUE(MSG_UBL_BUILD_CUSTOM_MESH            , _UxGT("Vlastn� s�t"));
  FSTRINGVALUE(MSG_UBL_BUILD_MESH_MENU              , _UxGT("Vytvorit s�t"));
  FSTRINGVALUE(MSG_UBL_BUILD_MESH_M1                , _UxGT("S�t bodu ") PREHEAT_1_LABEL);
  FSTRINGVALUE(MSG_UBL_BUILD_MESH_M2                , _UxGT("S�t bodu ") PREHEAT_2_LABEL);
  FSTRINGVALUE(MSG_UBL_BUILD_COLD_MESH              , _UxGT("Studen� s�t bodu"));
  FSTRINGVALUE(MSG_UBL_MESH_HEIGHT_ADJUST           , _UxGT("Upravit v��ku s�te"));
  FSTRINGVALUE(MSG_UBL_MESH_HEIGHT_AMOUNT           , _UxGT("V��ka"));
  FSTRINGVALUE(MSG_UBL_VALIDATE_MESH_MENU           , _UxGT("Zkontrolovat s�t"));
  FSTRINGVALUE(MSG_UBL_VALIDATE_MESH_M1             , _UxGT("Kontrola s�te ") PREHEAT_1_LABEL);
  FSTRINGVALUE(MSG_UBL_VALIDATE_MESH_M2             , _UxGT("Kontrola s�te ") PREHEAT_2_LABEL);
  FSTRINGVALUE(MSG_UBL_VALIDATE_CUSTOM_MESH         , _UxGT("Kontrola vlast. s�te"));
  FSTRINGVALUE(MSG_UBL_CONTINUE_MESH                , _UxGT("Pokracovat v s�ti"));
  FSTRINGVALUE(MSG_UBL_MESH_LEVELING                , _UxGT("S�tov� rovn�n�"));
  FSTRINGVALUE(MSG_UBL_3POINT_MESH_LEVELING         , _UxGT("3-bodov� rovn�n�"));
  FSTRINGVALUE(MSG_UBL_GRID_MESH_LEVELING           , _UxGT("Mr�kov� rovn�n�"));
  FSTRINGVALUE(MSG_UBL_MESH_LEVEL                   , _UxGT("Srovnat podlo�ku"));
  FSTRINGVALUE(MSG_UBL_SIDE_POINTS                  , _UxGT("Postrann� body"));
  FSTRINGVALUE(MSG_UBL_MAP_TYPE                     , _UxGT("Typ s�te bodu"));
  FSTRINGVALUE(MSG_UBL_OUTPUT_MAP                   , _UxGT("Exportovat s�t"));
  FSTRINGVALUE(MSG_UBL_OUTPUT_MAP_HOST              , _UxGT("Exportovat do PC"));
  FSTRINGVALUE(MSG_UBL_OUTPUT_MAP_CSV               , _UxGT("Exportovat do CSV"));
  FSTRINGVALUE(MSG_UBL_OUTPUT_MAP_BACKUP            , _UxGT("Z�loha do PC"));
  FSTRINGVALUE(MSG_UBL_INFO_UBL                     , _UxGT("Info o UBL do PC"));
  FSTRINGVALUE(MSG_UBL_FILLIN_AMOUNT                , _UxGT("Hustota mr�ky"));
  FSTRINGVALUE(MSG_UBL_MANUAL_FILLIN                , _UxGT("Rucn� hustota"));
  FSTRINGVALUE(MSG_UBL_SMART_FILLIN                 , _UxGT("Chytr� hustota"));
  FSTRINGVALUE(MSG_UBL_FILLIN_MESH                  , _UxGT("Zaplnit mr�ku"));
  FSTRINGVALUE(MSG_UBL_INVALIDATE_ALL               , _UxGT("Zru�it v�echno"));
  FSTRINGVALUE(MSG_UBL_INVALIDATE_CLOSEST           , _UxGT("Zru�it posledn�"));
  FSTRINGVALUE(MSG_UBL_FINE_TUNE_ALL                , _UxGT("Upravit v�echny"));
  FSTRINGVALUE(MSG_UBL_FINE_TUNE_CLOSEST            , _UxGT("Upravit posledn�"));
  FSTRINGVALUE(MSG_UBL_STORAGE_MESH_MENU            , _UxGT("Ulo�i�te s�t�"));
  FSTRINGVALUE(MSG_UBL_STORAGE_SLOT                 , _UxGT("Pametov� slot"));
  FSTRINGVALUE(MSG_UBL_LOAD_MESH                    , _UxGT("Nac�st s�t bodu"));
  FSTRINGVALUE(MSG_UBL_SAVE_MESH                    , _UxGT("Ulo�it s�t bodu"));
  FSTRINGVALUE(MSG_MESH_LOADED                      , _UxGT("S�t %i nactena"));
  FSTRINGVALUE(MSG_MESH_SAVED                       , _UxGT("S�t %i ulo�ena"));
  FSTRINGVALUE(MSG_UBL_NO_STORAGE                   , _UxGT("Nedostatek m�sta"));
  FSTRINGVALUE(MSG_UBL_SAVE_ERROR                   , _UxGT("Ch.: Ulo�it UBL"));
  FSTRINGVALUE(MSG_UBL_RESTORE_ERROR                , _UxGT("Ch.: Obnovit UBL"));
  FSTRINGVALUE(MSG_UBL_Z_OFFSET_STOPPED             , _UxGT("Konec Z-Offsetu"));
  FSTRINGVALUE(MSG_UBL_STEP_BY_STEP_MENU            , _UxGT("UBL Postupne"));
  FSTRINGVALUE(MSG_UBL_1_BUILD_COLD_MESH            , _UxGT("1. Studen� s�t bodu"));
  FSTRINGVALUE(MSG_UBL_2_SMART_FILLIN               , _UxGT("2. Chytr� hustota"));
  FSTRINGVALUE(MSG_UBL_3_VALIDATE_MESH_MENU         , _UxGT("3. Zkontrolovat s�t"));
  FSTRINGVALUE(MSG_UBL_4_FINE_TUNE_ALL              , _UxGT("4. Upravit v�echny"));
  FSTRINGVALUE(MSG_UBL_5_VALIDATE_MESH_MENU         , _UxGT("5. Zkontrolovat s�t"));
  FSTRINGVALUE(MSG_UBL_6_FINE_TUNE_ALL              , _UxGT("6. Upravit v�echny"));
  FSTRINGVALUE(MSG_UBL_7_SAVE_MESH                  , _UxGT("7. Ulo�it s�t bodu"));

  FSTRINGVALUE(MSG_LED_CONTROL                      , _UxGT("Nastaven� LED"));
  FSTRINGVALUE(MSG_LEDS                             , _UxGT("Svetla"));
  FSTRINGVALUE(MSG_LED_PRESETS                      , _UxGT("Svetla Predvolby"));
  FSTRINGVALUE(MSG_SET_LEDS_RED                     , _UxGT("Cerven�"));
  FSTRINGVALUE(MSG_SET_LEDS_ORANGE                  , _UxGT("Oran�ov�"));
  FSTRINGVALUE(MSG_SET_LEDS_YELLOW                  , _UxGT("�lut�"));
  FSTRINGVALUE(MSG_SET_LEDS_GREEN                   , _UxGT("Zelen�"));
  FSTRINGVALUE(MSG_SET_LEDS_BLUE                    , _UxGT("Modr�"));
  FSTRINGVALUE(MSG_SET_LEDS_INDIGO                  , _UxGT("Indigo"));
  FSTRINGVALUE(MSG_SET_LEDS_VIOLET                  , _UxGT("Fialov�"));
  FSTRINGVALUE(MSG_SET_LEDS_WHITE                   , _UxGT("B�l�"));
  FSTRINGVALUE(MSG_SET_LEDS_DEFAULT                 , _UxGT("V�choz�"));
  FSTRINGVALUE(MSG_CUSTOM_LEDS                      , _UxGT("Vlastn� svetla"));
  FSTRINGVALUE(MSG_INTENSITY_R                      , _UxGT("Cerven� intenzita"));
  FSTRINGVALUE(MSG_INTENSITY_G                      , _UxGT("Zelen� intezita"));
  FSTRINGVALUE(MSG_INTENSITY_B                      , _UxGT("Modr� intenzita"));
  FSTRINGVALUE(MSG_INTENSITY_W                      , _UxGT("B�l� intenzita"));
  FSTRINGVALUE(MSG_LED_BRIGHTNESS                   , _UxGT("Jas"));

  FSTRINGVALUE(MSG_MOVING                           , _UxGT("Posouv�n�..."));
  FSTRINGVALUE(MSG_FREE_XY                          , _UxGT("Uvolnit XY"));
  FSTRINGVALUE(MSG_MOVE_X                           , _UxGT("Posunout X"));
  FSTRINGVALUE(MSG_MOVE_Y                           , _UxGT("Posunout Y"));
  FSTRINGVALUE(MSG_MOVE_Z                           , _UxGT("Posunout Z"));
  FSTRINGVALUE(MSG_MOVE_E                           , _UxGT("Extrud�r"));
  FSTRINGVALUE(MSG_MOVE_EN                          , _UxGT("Extrud�r *"));
  FSTRINGVALUE(MSG_HOTEND_TOO_COLD                  , _UxGT("Hotend je studen�"));
  FSTRINGVALUE(MSG_MOVE_Z_DIST                      , _UxGT("Posunout o %smm"));
  FSTRINGVALUE(MSG_MOVE_01MM                        , _UxGT("Posunout o 0,1mm"));
  FSTRINGVALUE(MSG_MOVE_1MM                         , _UxGT("Posunout o 1mm"));
  FSTRINGVALUE(MSG_MOVE_10MM                        , _UxGT("Posunout o 10mm"));
  FSTRINGVALUE(MSG_SPEED                            , _UxGT("Rychlost"));
  FSTRINGVALUE(MSG_BED_Z                            , _UxGT("V��ka podl."));
  FSTRINGVALUE(MSG_NOZZLE                           , _UxGT("Tryska"));
  FSTRINGVALUE(MSG_NOZZLE_N                         , _UxGT("Tryska H"));
  FSTRINGVALUE(MSG_BED                              , _UxGT("Podlo�ka"));
  FSTRINGVALUE(MSG_CHAMBER                          , _UxGT("Komora"));
  FSTRINGVALUE(MSG_FAN_SPEED                        , _UxGT("Rychlost vent."));
  FSTRINGVALUE(MSG_FAN_SPEED_N                      , _UxGT("Rychlost vent. ="));
  FSTRINGVALUE(MSG_EXTRA_FAN_SPEED                  , _UxGT("Rychlost ex. vent."));
  FSTRINGVALUE(MSG_EXTRA_FAN_SPEED_N                , _UxGT("Rychlost ex. vent. ="));
  FSTRINGVALUE(MSG_FLOW                             , _UxGT("Prutok"));
  FSTRINGVALUE(MSG_FLOW_N                           , _UxGT("Prutok E"));
  FSTRINGVALUE(MSG_CONTROL                          , _UxGT("Ovl�dan�"));
  FSTRINGVALUE(MSG_MIN                              , " " LCD_STR_THERMOMETER _UxGT(" Min"));
  FSTRINGVALUE(MSG_MAX                              , " " LCD_STR_THERMOMETER _UxGT(" Max"));
  FSTRINGVALUE(MSG_FACTOR                           , " " LCD_STR_THERMOMETER _UxGT(" Fakt"));
  FSTRINGVALUE(MSG_AUTOTEMP                         , _UxGT("Autoteplota"));
  FSTRINGVALUE(MSG_LCD_ON                           , _UxGT("Zap"));
  FSTRINGVALUE(MSG_LCD_OFF                          , _UxGT("Vyp"));
  FSTRINGVALUE(MSG_SELECT                           , _UxGT("Vybrat"));
  FSTRINGVALUE(MSG_SELECT_E                         , _UxGT("Vybrat *"));
  FSTRINGVALUE(MSG_ACC                              , _UxGT("Zrychl"));
  FSTRINGVALUE(MSG_JUNCTION_DEVIATION               , _UxGT("Odchylka spoje"));
  FSTRINGVALUE(MSG_VELOCITY                         , _UxGT("Rychlost"));
  FSTRINGVALUE(MSG_ACCELERATION                     , _UxGT("Akcelerace"));
  FSTRINGVALUE(MSG_A_RETRACT                        , _UxGT("A-retrakt"));
  FSTRINGVALUE(MSG_A_TRAVEL                         , _UxGT("A-prejezd"));
  FSTRINGVALUE(MSG_STEPS_PER_MM                     , _UxGT("Kroku/mm"));
  FSTRINGVALUE(MSG_A_STEPS                          , LCD_STR_A _UxGT("kroku/mm"));
  FSTRINGVALUE(MSG_B_STEPS                          , LCD_STR_B _UxGT("kroku/mm"));
  FSTRINGVALUE(MSG_C_STEPS                          , LCD_STR_C _UxGT("kroku/mm"));
  FSTRINGVALUE(MSG_E_STEPS                          , _UxGT("Ekroku/mm"));
  FSTRINGVALUE(MSG_EN_STEPS                         , _UxGT("*kroku/mm"));
  FSTRINGVALUE(MSG_TEMPERATURE                      , _UxGT("Teplota"));
  FSTRINGVALUE(MSG_MOTION                           , _UxGT("Pohyb"));
  FSTRINGVALUE(MSG_FILAMENT                         , _UxGT("Filament"));
  FSTRINGVALUE(MSG_VOLUMETRIC_ENABLED               , _UxGT("E na mm3"));
  FSTRINGVALUE(MSG_FILAMENT_DIAM                    , _UxGT("Fil. Prum."));
  FSTRINGVALUE(MSG_FILAMENT_DIAM_E                  , _UxGT("Fil. Prum. *"));
  FSTRINGVALUE(MSG_FILAMENT_UNLOAD                  , _UxGT("Vysunout mm"));
  FSTRINGVALUE(MSG_FILAMENT_LOAD                    , _UxGT("Zav�st mm"));
  FSTRINGVALUE(MSG_ADVANCE_K                        , _UxGT("K pro posun"));
  FSTRINGVALUE(MSG_ADVANCE_K_E                      , _UxGT("K pro posun *"));
  FSTRINGVALUE(MSG_CONTRAST                         , _UxGT("Kontrast LCD"));
  FSTRINGVALUE(MSG_STORE_EEPROM                     , _UxGT("Ulo�it nastaven�"));
  FSTRINGVALUE(MSG_LOAD_EEPROM                      , _UxGT("Nac�st nastaven�"));
  FSTRINGVALUE(MSG_RESTORE_FAILSAFE                 , _UxGT("Obnovit v�choz�"));
  FSTRINGVALUE(MSG_INIT_EEPROM                      , _UxGT("Inic. EEPROM"));
  FSTRINGVALUE(MSG_MEDIA_UPDATE                     , _UxGT("Aktualizace z SD"));
  FSTRINGVALUE(MSG_RESET_PRINTER                    , _UxGT("Reset tisk�rny"));
  FSTRINGVALUE(MSG_REFRESH                          , LCD_STR_REFRESH _UxGT("Obnovit"));
  FSTRINGVALUE(MSG_INFO_SCREEN                      , _UxGT("Info obrazovka"));
  FSTRINGVALUE(MSG_PREPARE                          , _UxGT("Priprava tisku"));
  FSTRINGVALUE(MSG_TUNE                             , _UxGT("Doladen� tisku"));
  FSTRINGVALUE(MSG_START_PRINT                      , _UxGT("Spustit tisk"));
  FSTRINGVALUE(MSG_BUTTON_PRINT                     , _UxGT("Tisk"));
  FSTRINGVALUE(MSG_BUTTON_CANCEL                    , _UxGT("Zru�it"));
  FSTRINGVALUE(MSG_PAUSE_PRINT                      , _UxGT("Pozastavit tisk"));
  FSTRINGVALUE(MSG_RESUME_PRINT                     , _UxGT("Obnovit tisk"));
  FSTRINGVALUE(MSG_STOP_PRINT                       , _UxGT("Zastavit tisk"));
  FSTRINGVALUE(MSG_OUTAGE_RECOVERY                  , _UxGT("Obnova v�padku"));
  FSTRINGVALUE(MSG_MEDIA_MENU                       , _UxGT("Tisknout z SD"));
  FSTRINGVALUE(MSG_NO_MEDIA                         , _UxGT("��dn� SD karta"));
  FSTRINGVALUE(MSG_DWELL                            , _UxGT("Usp�no..."));
  FSTRINGVALUE(MSG_USERWAIT                         , _UxGT("Cek�n� na u�iv..."));
  FSTRINGVALUE(MSG_PRINT_PAUSED                     , _UxGT("Tisk pozastaven"));
  FSTRINGVALUE(MSG_PRINTING                         , _UxGT("Tisknu..."));
  FSTRINGVALUE(MSG_PRINT_ABORTED                    , _UxGT("Tisk zru�en"));
  FSTRINGVALUE(MSG_NO_MOVE                          , _UxGT("��dn� pohyb."));
  FSTRINGVALUE(MSG_KILLED                           , _UxGT("PRERUSENO. "));
  FSTRINGVALUE(MSG_STOPPED                          , _UxGT("ZASTAVENO. "));
  FSTRINGVALUE(MSG_CONTROL_RETRACT                  , _UxGT("Retrakt mm"));
  FSTRINGVALUE(MSG_CONTROL_RETRACT_SWAP             , _UxGT("V�mena Re.mm"));
  FSTRINGVALUE(MSG_CONTROL_RETRACTF                 , _UxGT("Retraktovat  V"));
  FSTRINGVALUE(MSG_CONTROL_RETRACT_ZHOP             , _UxGT("Zvednuti Z mm"));
  FSTRINGVALUE(MSG_CONTROL_RETRACT_RECOVER          , _UxGT("UnRet mm"));
  FSTRINGVALUE(MSG_CONTROL_RETRACT_RECOVER_SWAP     , _UxGT("S UnRet mm"));
  FSTRINGVALUE(MSG_CONTROL_RETRACT_RECOVERF         , _UxGT("UnRet  V"));
  FSTRINGVALUE(MSG_CONTROL_RETRACT_RECOVER_SWAPF    , _UxGT("S UnRet V"));
  FSTRINGVALUE(MSG_AUTORETRACT                      , _UxGT("AutoRetr."));
  FSTRINGVALUE(MSG_FILAMENT_SWAP_LENGTH             , _UxGT("D�lka retrakce"));
  FSTRINGVALUE(MSG_TOOL_CHANGE                      , _UxGT("V�mena n�stroje"));
  FSTRINGVALUE(MSG_TOOL_CHANGE_ZLIFT                , _UxGT("Zdvih Z"));
  FSTRINGVALUE(MSG_SINGLENOZZLE_PRIME_SPD           , _UxGT("Rychlost prim�r."));
  FSTRINGVALUE(MSG_SINGLENOZZLE_RETRACT_SPD         , _UxGT("Rychlost retrak."));
  FSTRINGVALUE(MSG_NOZZLE_STANDBY                   , _UxGT("Tryska standby"));
  FSTRINGVALUE(MSG_FILAMENTCHANGE                   , _UxGT("Vymenit filament"));
  FSTRINGVALUE(MSG_FILAMENTCHANGE_E                 , _UxGT("Vymenit filament *"));
  FSTRINGVALUE(MSG_FILAMENTLOAD                     , _UxGT("Zav�st filament"));
  FSTRINGVALUE(MSG_FILAMENTLOAD_E                   , _UxGT("Zav�st filament *"));
  FSTRINGVALUE(MSG_FILAMENTUNLOAD                   , _UxGT("Vysunout filament"));
  FSTRINGVALUE(MSG_FILAMENTUNLOAD_E                 , _UxGT("Vysunout filament *"));
  FSTRINGVALUE(MSG_FILAMENTUNLOAD_ALL               , _UxGT("Vysunout v�e"));

  FSTRINGVALUE(MSG_INIT_MEDIA                       , _UxGT("Nac�st SD kartu"));
  FSTRINGVALUE(MSG_CHANGE_MEDIA                     , _UxGT("Vymenit SD kartu"));
  FSTRINGVALUE(MSG_ZPROBE_OUT                       , _UxGT("Sonda Z mimo podl"));
  FSTRINGVALUE(MSG_SKEW_FACTOR                      , _UxGT("Faktor zkosen�"));
  FSTRINGVALUE(MSG_BLTOUCH                          , _UxGT("BLTouch"));
  FSTRINGVALUE(MSG_BLTOUCH_SELFTEST                 , _UxGT("BLTouch self-test"));
  FSTRINGVALUE(MSG_BLTOUCH_RESET                    , _UxGT("BLTouch reset"));
  FSTRINGVALUE(MSG_BLTOUCH_DEPLOY                   , _UxGT("BLTouch vysunout"));
  FSTRINGVALUE(MSG_BLTOUCH_SW_MODE                  , _UxGT("SW v�sun BLTouch"));
  FSTRINGVALUE(MSG_BLTOUCH_5V_MODE                  , _UxGT("BLTouch 5V re�im"));
  FSTRINGVALUE(MSG_BLTOUCH_OD_MODE                  , _UxGT("BLTouch OD re�im"));
  FSTRINGVALUE(MSG_BLTOUCH_STOW                     , _UxGT("BLTouch zasunout"));
  FSTRINGVALUE(MSG_MANUAL_DEPLOY                    , _UxGT("Vysunout Z-sondu"));
  FSTRINGVALUE(MSG_MANUAL_STOW                      , _UxGT("Zasunout Z-sondu"));
  FSTRINGVALUE(MSG_HOME_FIRST                       , _UxGT("Domu %s%s%s prvn�"));
  FSTRINGVALUE(MSG_ZPROBE_ZOFFSET                   , _UxGT("Z ofset"));
  FSTRINGVALUE(MSG_BABYSTEP_X                       , _UxGT("Babystep X"));
  FSTRINGVALUE(MSG_BABYSTEP_Y                       , _UxGT("Babystep Y"));
  FSTRINGVALUE(MSG_BABYSTEP_Z                       , _UxGT("Babystep Z"));
  FSTRINGVALUE(MSG_BABYSTEP_TOTAL                   , _UxGT("Celkem"));
  FSTRINGVALUE(MSG_ENDSTOP_ABORT                    , _UxGT("Endstop abort"));
  FSTRINGVALUE(MSG_HEATING_FAILED_LCD               , _UxGT("Chyba zahr�v�n�"));
  FSTRINGVALUE(MSG_HEATING_FAILED_LCD_BED           , _UxGT("Chyba zahr.podl."));
  FSTRINGVALUE(MSG_ERR_REDUNDANT_TEMP               , _UxGT("REDUND. TEPLOTA"));
  FSTRINGVALUE(MSG_THERMAL_RUNAWAY                  , _UxGT("TEPLOTN� �NIK"));
  FSTRINGVALUE(MSG_THERMAL_RUNAWAY_BED              , _UxGT("TEPL. �NIK PODL."));
  FSTRINGVALUE(MSG_ERR_MAXTEMP                      , _UxGT("VYSOK� TEPLOTA"));
  FSTRINGVALUE(MSG_ERR_MINTEMP                      , _UxGT("N�ZKA TEPLOTA"));
  FSTRINGVALUE(MSG_ERR_MAXTEMP_BED                  , _UxGT("VYS. TEPL. PODL."));
  FSTRINGVALUE(MSG_ERR_MINTEMP_BED                  , _UxGT("N�Z. TEPL. PODL."));
  FSTRINGVALUE(MSG_ERR_MAXTEMP_CHAMBER              , _UxGT("Err: MAXTEMP KOMORA"));
  FSTRINGVALUE(MSG_ERR_MINTEMP_CHAMBER              , _UxGT("Err: MINTEMP KOMORA"));
  FSTRINGVALUE(MSG_ERR_Z_HOMING                     , _UxGT("Domu XY prvn�"));
  FSTRINGVALUE(MSG_HALTED                           , _UxGT("TISK. ZASTAVENA"));
  FSTRINGVALUE(MSG_PLEASE_RESET                     , _UxGT("Provedte reset"));
  FSTRINGVALUE(MSG_SHORT_DAY                        , _UxGT("d"));
  FSTRINGVALUE(MSG_SHORT_HOUR                       , _UxGT("h"));
  FSTRINGVALUE(MSG_SHORT_MINUTE                     , _UxGT("m"));
  FSTRINGVALUE(MSG_HEATING                          , _UxGT("Zahr�v�n�..."));
  FSTRINGVALUE(MSG_COOLING                          , _UxGT("Chlazen�..."));
  #if LCD_WIDTH >= 20
    FSTRINGVALUE(MSG_BED_HEATING                    , _UxGT("Zahr�v�n� podlo�ky"));
  #else
    FSTRINGVALUE(MSG_BED_HEATING                    , _UxGT("Zahr�v�n� podl."));
  #endif
  #if LCD_WIDTH >= 20
    FSTRINGVALUE(MSG_BED_COOLING                    , _UxGT("Chlazen� podlo�ky"));
  #else
    FSTRINGVALUE(MSG_BED_COOLING                    , _UxGT("Chlazen� podl."));
  #endif
  FSTRINGVALUE(MSG_DELTA_CALIBRATE                  , _UxGT("Delta Kalibrace"));
  FSTRINGVALUE(MSG_DELTA_CALIBRATE_X                , _UxGT("Kalibrovat X"));
  FSTRINGVALUE(MSG_DELTA_CALIBRATE_Y                , _UxGT("Kalibrovat Y"));
  FSTRINGVALUE(MSG_DELTA_CALIBRATE_Z                , _UxGT("Kalibrovat Z"));
  FSTRINGVALUE(MSG_DELTA_CALIBRATE_CENTER           , _UxGT("Kalibrovat Stred"));
  FSTRINGVALUE(MSG_DELTA_SETTINGS                   , _UxGT("Delta nastaven�"));
  FSTRINGVALUE(MSG_DELTA_AUTO_CALIBRATE             , _UxGT("Autokalibrace"));
  FSTRINGVALUE(MSG_DELTA_HEIGHT_CALIBRATE           , _UxGT("Nast.v��ku delty"));
  FSTRINGVALUE(MSG_DELTA_Z_OFFSET_CALIBRATE         , _UxGT("Nast. Z-ofset"));
  FSTRINGVALUE(MSG_DELTA_DIAG_ROD                   , _UxGT("Diag rameno"));
  FSTRINGVALUE(MSG_DELTA_HEIGHT                     , _UxGT("V��ka"));
  FSTRINGVALUE(MSG_DELTA_RADIUS                     , _UxGT("Polomer"));
  FSTRINGVALUE(MSG_INFO_MENU                        , _UxGT("O tisk�rne"));
  FSTRINGVALUE(MSG_INFO_PRINTER_MENU                , _UxGT("Info o tisk�rne"));
  FSTRINGVALUE(MSG_3POINT_LEVELING                  , _UxGT("3-bodov� rovn�n�"));
  FSTRINGVALUE(MSG_LINEAR_LEVELING                  , _UxGT("Line�rni rovn�n�"));
  FSTRINGVALUE(MSG_BILINEAR_LEVELING                , _UxGT("Biline�rn� rovn�n�"));
  FSTRINGVALUE(MSG_UBL_LEVELING                     , _UxGT("Unified Bed Leveling"));
  FSTRINGVALUE(MSG_MESH_LEVELING                    , _UxGT("Mr�kov� rovn�n�"));
  FSTRINGVALUE(MSG_INFO_STATS_MENU                  , _UxGT("Statistika"));
  FSTRINGVALUE(MSG_INFO_BOARD_MENU                  , _UxGT("Info o desce"));
  FSTRINGVALUE(MSG_INFO_THERMISTOR_MENU             , _UxGT("Termistory"));
  FSTRINGVALUE(MSG_INFO_EXTRUDERS                   , _UxGT("Extrud�ry"));
  FSTRINGVALUE(MSG_INFO_BAUDRATE                    , _UxGT("Rychlost"));
  FSTRINGVALUE(MSG_INFO_PROTOCOL                    , _UxGT("Protokol"));
  FSTRINGVALUE(MSG_CASE_LIGHT                       , _UxGT("Osvetlen�"));
  FSTRINGVALUE(MSG_CASE_LIGHT_BRIGHTNESS            , _UxGT("Jas svetla"));

  #if LCD_WIDTH >= 20
    FSTRINGVALUE(MSG_INFO_PRINT_COUNT               , _UxGT("Pocet tisku"));
    FSTRINGVALUE(MSG_INFO_COMPLETED_PRINTS          , _UxGT("Dokonceno"));
    FSTRINGVALUE(MSG_INFO_PRINT_TIME                , _UxGT("Celkov� cas"));
    FSTRINGVALUE(MSG_INFO_PRINT_LONGEST             , _UxGT("Nejdel�� tisk"));
    FSTRINGVALUE(MSG_INFO_PRINT_FILAMENT            , _UxGT("Celkem vytlaceno"));
  #else
    FSTRINGVALUE(MSG_INFO_PRINT_COUNT               , _UxGT("Tisky"));
    FSTRINGVALUE(MSG_INFO_COMPLETED_PRINTS          , _UxGT("Hotovo"));
    FSTRINGVALUE(MSG_INFO_PRINT_TIME                , _UxGT("Cas"));
    FSTRINGVALUE(MSG_INFO_PRINT_LONGEST             , _UxGT("Nejdel��"));
    FSTRINGVALUE(MSG_INFO_PRINT_FILAMENT            , _UxGT("Vytlaceno"));
  #endif

  FSTRINGVALUE(MSG_INFO_MIN_TEMP                    , _UxGT("Teplota min"));
  FSTRINGVALUE(MSG_INFO_MAX_TEMP                    , _UxGT("Teplota max"));
  FSTRINGVALUE(MSG_INFO_PSU                         , _UxGT("Nap. zdroj"));
  FSTRINGVALUE(MSG_DRIVE_STRENGTH                   , _UxGT("Buzen� motoru"));
  FSTRINGVALUE(MSG_DAC_PERCENT                      , _UxGT("Motor %"));
  FSTRINGVALUE(MSG_DAC_EEPROM_WRITE                 , _UxGT("Ulo�it do EEPROM"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEADER           , _UxGT("V�MENA FILAMENTU"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEADER_PAUSE     , _UxGT("TISK POZASTAVEN"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEADER_LOAD      , _UxGT("ZAVEDEN� FILAMENTU"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEADER_UNLOAD    , _UxGT("VYSUNUT� FILAMENTU"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_OPTION_HEADER    , _UxGT("MO�NOSTI OBNOVEN�:"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_OPTION_PURGE     , _UxGT("Vytlacit v�c"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_OPTION_RESUME    , _UxGT("Obnovit tisk"));
  FSTRINGVALUE(MSG_FILAMENT_CHANGE_NOZZLE           , _UxGT("  Tryska: "));
  FSTRINGVALUE(MSG_RUNOUT_SENSOR                    , _UxGT("Senzor filamentu"));
  FSTRINGVALUE(MSG_LCD_HOMING_FAILED                , _UxGT("Parkov�n� selhalo"));
  FSTRINGVALUE(MSG_LCD_PROBING_FAILED               , _UxGT("Kalibrace selhala"));
  FSTRINGVALUE(MSG_M600_TOO_COLD                    , _UxGT("M600: Moc studen�"));

  FSTRINGVALUE(MSG_MMU2_CHOOSE_FILAMENT_HEADER      , _UxGT("VYBERTE FILAMENT"));
  FSTRINGVALUE(MSG_MMU2_MENU                        , _UxGT("MMU"));
  FSTRINGVALUE(MSG_MMU2_WRONG_FIRMWARE              , _UxGT("Aktual. MMU firmware!"));
  FSTRINGVALUE(MSG_MMU2_NOT_RESPONDING              , _UxGT("MMU potr. pozornost."));
  FSTRINGVALUE(MSG_MMU2_RESUME                      , _UxGT("Obnovit tisk"));
  FSTRINGVALUE(MSG_MMU2_RESUMING                    , _UxGT("Obnovov�n�..."));
  FSTRINGVALUE(MSG_MMU2_LOAD_FILAMENT               , _UxGT("Zav�st filament"));
  FSTRINGVALUE(MSG_MMU2_LOAD_ALL                    , _UxGT("Zav�st v�echny"));
  FSTRINGVALUE(MSG_MMU2_LOAD_TO_NOZZLE              , _UxGT("Zav�st do trysky"));
  FSTRINGVALUE(MSG_MMU2_EJECT_FILAMENT              , _UxGT("Vysunout filament"));
  FSTRINGVALUE(MSG_MMU2_EJECT_FILAMENT_N            , _UxGT("Vysun. filament E"));
  FSTRINGVALUE(MSG_MMU2_UNLOAD_FILAMENT             , _UxGT("Vyt�hnout filament"));
  FSTRINGVALUE(MSG_MMU2_LOADING_FILAMENT            , _UxGT("Zav�den� fil. %i..."));
  FSTRINGVALUE(MSG_MMU2_EJECTING_FILAMENT           , _UxGT("Vytahov�n� fil. ..."));
  FSTRINGVALUE(MSG_MMU2_UNLOADING_FILAMENT          , _UxGT("Vysouv�n� fil...."));
  FSTRINGVALUE(MSG_MMU2_ALL                         , _UxGT("V�echny"));
  FSTRINGVALUE(MSG_MMU2_FILAMENT_N                  , _UxGT("Filament E"));
  FSTRINGVALUE(MSG_MMU2_RESET                       , _UxGT("Resetovat MMU"));
  FSTRINGVALUE(MSG_MMU2_RESETTING                   , _UxGT("Resetov�n� MMU..."));
  FSTRINGVALUE(MSG_MMU2_EJECT_RECOVER               , _UxGT("Vyt�hnete, kliknete"));

  FSTRINGVALUE(MSG_MIX                              , _UxGT("Mix"));
  FSTRINGVALUE(MSG_MIX_COMPONENT_N                  , _UxGT("Komponenta E"));
  FSTRINGVALUE(MSG_MIXER                            , _UxGT("Mix�r"));
  FSTRINGVALUE(MSG_GRADIENT                         , _UxGT("Prechod"));
  FSTRINGVALUE(MSG_FULL_GRADIENT                    , _UxGT("Cel� prechod"));
  FSTRINGVALUE(MSG_TOGGLE_MIX                       , _UxGT("Prepnout mix"));
  FSTRINGVALUE(MSG_CYCLE_MIX                        , _UxGT("Str�dat mix"));
  FSTRINGVALUE(MSG_GRADIENT_MIX                     , _UxGT("Prechod mix"));
  FSTRINGVALUE(MSG_REVERSE_GRADIENT                 , _UxGT("Opacn� prechod"));
  #if LCD_WIDTH >= 20
    FSTRINGVALUE(MSG_ACTIVE_VTOOL                   , _UxGT("Aktivn� V-n�stroj"));
    FSTRINGVALUE(MSG_START_VTOOL                    , _UxGT("Spustit V-n�stroj"));
    FSTRINGVALUE(MSG_END_VTOOL                      , _UxGT("Ukoncit V-n�stroj"));
    FSTRINGVALUE(MSG_GRADIENT_ALIAS                 , _UxGT("Alias V-n�stroje"));
    FSTRINGVALUE(MSG_RESET_VTOOLS                   , _UxGT("Resetovat V-n�stroj"));
    FSTRINGVALUE(MSG_COMMIT_VTOOL                   , _UxGT("Ulo�it V-n�stroj mix"));
    FSTRINGVALUE(MSG_VTOOLS_RESET                   , _UxGT("V-n�stroj resetovat"));
  #else
    FSTRINGVALUE(MSG_ACTIVE_VTOOL                   , _UxGT("Aktivn� V-n�str."));
    FSTRINGVALUE(MSG_START_VTOOL                    , _UxGT("Spustit V-n�str."));
    FSTRINGVALUE(MSG_END_VTOOL                      , _UxGT("Ukoncit V-n�str."));
    FSTRINGVALUE(MSG_GRADIENT_ALIAS                 , _UxGT("Alias V-n�str."));
    FSTRINGVALUE(MSG_RESET_VTOOLS                   , _UxGT("Reset. V-n�str."));
    FSTRINGVALUE(MSG_COMMIT_VTOOL                   , _UxGT("Ulo�it V-n�s. mix"));
    FSTRINGVALUE(MSG_VTOOLS_RESET                   , _UxGT("V-n�str. reset."));
  #endif
  FSTRINGVALUE(MSG_START_Z                          , _UxGT("Poc�tecn� Z:"));
  FSTRINGVALUE(MSG_END_Z                            , _UxGT("  Koncov� Z:"));
  FSTRINGVALUE(MSG_BRICKOUT                         , _UxGT("Brickout"));
  FSTRINGVALUE(MSG_INVADERS                         , _UxGT("Invaders"));
  FSTRINGVALUE(MSG_SNAKE                            , _UxGT("Sn4k3"));
  FSTRINGVALUE(MSG_MAZE                             , _UxGT("Bludi�te"));

  FSTRINGVALUE(MSG_EXPECTED_PRINTER                 , _UxGT("Nespr�vn� tisk�rna"));

  #if LCD_HEIGHT >= 4
    // Up to 3 lines allowed
    FSTRINGVALUE(MSG_ADVANCED_PAUSE_WAITING         , _UxGT(MSG_2_LINE("Stiknete tlac�tko", "pro obnoven� tisku")));
    FSTRINGVALUE(MSG_PAUSE_PRINT_PARKING            , _UxGT(MSG_1_LINE("Parkov�n�...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_INIT           , _UxGT(MSG_3_LINE("Cekejte pros�m", "na zah�jen�", "v�meny filamentu")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_INSERT         , _UxGT(MSG_3_LINE("Vlo�te filament", "a stisknete", "tlac�tko...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEAT           , _UxGT(MSG_2_LINE("Kliknete pro", "nahr�t� trysky")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEATING        , _UxGT(MSG_2_LINE("Cekejte pros�m", "na nahr�t� tr.")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_UNLOAD         , _UxGT(MSG_3_LINE("Cekejte pros�m", "na vysunuti", "filamentu")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_LOAD           , _UxGT(MSG_3_LINE("Cekejte pros�m", "na zaveden�", "filamentu")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_PURGE          , _UxGT(MSG_2_LINE("Vyckejte na", "vytlacen�")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_CONT_PURGE     , _UxGT(MSG_3_LINE("Kliknete pro", "ukoncen�", "vytlacov�n�")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_RESUME         , _UxGT(MSG_3_LINE("Cekejte pros�m", "na pokracov�n�", "tisku")));
  #else // LCD_HEIGHT < 4
    // Up to 2 lines allowed
    FSTRINGVALUE(MSG_ADVANCED_PAUSE_WAITING         , _UxGT(MSG_2_LINE("Stiknete tlac.", "pro obnoven�")));
    FSTRINGVALUE(MSG_PAUSE_PRINT_PARKING            , _UxGT(MSG_1_LINE("Parkov�n�...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_INIT           , _UxGT(MSG_1_LINE("Cekejte...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_INSERT         , _UxGT(MSG_1_LINE("Vlo�te, kliknete")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEAT           , _UxGT(MSG_2_LINE("Kliknete pro", "nahr�t�")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_HEATING        , _UxGT(MSG_1_LINE("Nahr�v�n�...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_UNLOAD         , _UxGT(MSG_1_LINE("Vysouv�n�...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_LOAD           , _UxGT(MSG_1_LINE("Zav�den�...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_PURGE          , _UxGT(MSG_1_LINE("Vytlacov�n�...")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_CONT_PURGE     , _UxGT(MSG_2_LINE("Kliknete pro", "ukoncen�")));
    FSTRINGVALUE(MSG_FILAMENT_CHANGE_RESUME         , _UxGT(MSG_1_LINE("Pokracov�n�...")));
  #endif // LCD_HEIGHT < 4

  FSTRINGVALUE(MSG_TMC_DRIVERS                      , _UxGT("TMC budice"));
  FSTRINGVALUE(MSG_TMC_CURRENT                      , _UxGT("Proud budicu"));
  FSTRINGVALUE(MSG_TMC_HYBRID_THRS                  , _UxGT("Hybridn� pr�h"));
  FSTRINGVALUE(MSG_TMC_HOMING_THRS                  , _UxGT("Domu bez senzoru"));
  FSTRINGVALUE(MSG_TMC_STEPPING_MODE                , _UxGT("Re�im kroku"));
  FSTRINGVALUE(MSG_TMC_STEALTH_ENABLED              , _UxGT("StealthChop povolen"));
  FSTRINGVALUE(MSG_SERVICE_RESET                    , _UxGT("Reset"));
  FSTRINGVALUE(MSG_SERVICE_IN                       , _UxGT(" za:"));
  FSTRINGVALUE(MSG_BACKLASH                         , _UxGT("Vule"));
  FSTRINGVALUE(MSG_BACKLASH_A                       , LCD_STR_A);
  FSTRINGVALUE(MSG_BACKLASH_B                       , LCD_STR_B);
  FSTRINGVALUE(MSG_BACKLASH_C                       , LCD_STR_C);
  FSTRINGVALUE(MSG_BACKLASH_CORRECTION              , _UxGT("Korekce"));
  FSTRINGVALUE(MSG_BACKLASH_SMOOTHING               , _UxGT("Vyhlazen�"));
}
