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
 * gcode.h
 *
 * Copyright (c) 2020 Alberto Cotronei @MagoKimbra
 */

#if ENABLED(DELTA_AUTO_CALIBRATION_2)

#define CODE_G33

constexpr uint8_t _7P_STEP = 1,              // 7-point step - to change number of calibration points
                  _4P_STEP = _7P_STEP * 2,   // 4-point step
                  NPP      = _7P_STEP * 6,   // number of calibration points on the radius
                  CEN      = 0,
                  __A      = 1,
                  _AB      = __A + _7P_STEP,
                  __B      = _AB + _7P_STEP,
                  _BC      = __B + _7P_STEP,
                  __C      = _BC + _7P_STEP,
                  _CA      = __C + _7P_STEP;

#define LOOP_CAL_PT(VAR,S,N)      for(uint8_t VAR=S; VAR<=NPP; VAR+=N)
#define F_LOOP_CAL_PT(VAR,S,N)    for(float VAR=S; VAR<NPP+0.9999; VAR+=N)
#define I_LOOP_CAL_PT(VAR,S,N)    for(float VAR=S; VAR>CEN+0.9999; VAR-=N)
#define LOOP_CAL_ALL(VAR)         LOOP_CAL_PT(VAR, CEN, 1)
#define LOOP_CAL_RAD(VAR)         LOOP_CAL_PT(VAR, __A, _7P_STEP)
#define LOOP_CAL_ACT(VAR,_4P,_OP) LOOP_CAL_PT(VAR, _OP ? _AB : __A, _4P ? _4P_STEP : _7P_STEP)

static void ac_home() {
  endstops.setEnabled(true);
  mechanics.home(false);
  endstops.setNotHoming();
}

static void ac_setup(const bool reset_bed) {

  if (toolManager.extruder.total > 1) toolManager.change(0, true);

  planner.synchronize();
  mechanics.setup_for_endstop_or_probe_move();

  #if HAS_LEVELING
    if (reset_bed) bedlevel.reset(); // After full calibration bed-level data is no longer valid
  #endif

}

static void ac_cleanup() {
  #if ENABLED(DELTA_HOME_TO_SAFE_ZONE)
    mechanics.do_blocking_move_to_z(mechanics.delta_clip_start_height);
  #endif

  STOW_PROBE();
  mechanics.clean_up_after_endstop_or_probe_move();

  if (toolManager.extruder.total > 1) toolManager.change(toolManager.extruder.previous, true);

}

static void Report_signed_float(PGM_P const prefix, const float &f) {
  SERIAL_MSG("  ");
  SERIAL_STR(prefix);
  SERIAL_CHR(':');
  if (f >= 0) SERIAL_CHR('+');
  SERIAL_VAL(f, 2);
}

/**
 * Print the delta settings
 */
static void Report_settings(const bool end_stops, const bool tower_angles) {
  SERIAL_MV(".Height:", mechanics.data.height, 2);
  if (end_stops) {
    Report_signed_float(PSTR("Ex"), mechanics.data.endstop_adj[A_AXIS]);
    Report_signed_float(PSTR("Ey"), mechanics.data.endstop_adj[B_AXIS]);
    Report_signed_float(PSTR("Ez"), mechanics.data.endstop_adj[C_AXIS]);
  }
  if (end_stops && tower_angles) {
    SERIAL_MV("  Radius:", mechanics.data.radius, 2);
    SERIAL_EOL();
    SERIAL_CHR('.');
    SERIAL_SP(13);
  }
  if (tower_angles) {
    Report_signed_float(PSTR("Tx"), mechanics.data.tower_angle_adj[A_AXIS]);
    Report_signed_float(PSTR("Ty"), mechanics.data.tower_angle_adj[B_AXIS]);
    Report_signed_float(PSTR("Tz"), mechanics.data.tower_angle_adj[C_AXIS]);
  }
  if ((!end_stops && tower_angles) || (end_stops && !tower_angles)) { // XOR
    SERIAL_MV("  Radius:", mechanics.data.radius, 2);
  }
  SERIAL_EOL();
}

/**
 * Print the probe results
 */
static void Report_results(const float z_pt[NPP + 1], const bool tower_points, const bool opposite_points) {
  SERIAL_MSG(".    ");
  Report_signed_float(PSTR("c"), z_pt[CEN]);
  if (tower_points) {
    Report_signed_float(PSTR(" x"), z_pt[__A]);
    Report_signed_float(PSTR(" y"), z_pt[__B]);
    Report_signed_float(PSTR(" z"), z_pt[__C]);
  }
  if (tower_points && opposite_points) {
    SERIAL_EOL();
    SERIAL_CHR('.');
    SERIAL_SP(13);
  }
  if (opposite_points) {
    Report_signed_float(PSTR("yz"), z_pt[_BC]);
    Report_signed_float(PSTR("zx"), z_pt[_CA]);
    Report_signed_float(PSTR("xy"), z_pt[_AB]);
  }
  SERIAL_EOL();
}

/**
 * Calculate the standard deviation from the zero plane
 */
static float std_dev_points(float z_pt[NPP + 1], const bool _0p_cal, const bool _1p_cal, const bool _4p_cal, const bool _4p_opp) {
  if (!_0p_cal) {
    float S2 = sq(z_pt[CEN]);
    int16_t N = 1;
    if (!_1p_cal) { // std dev from zero plane
      LOOP_CAL_ACT(rad, _4p_cal, _4p_opp) {
        S2 += sq(z_pt[rad]);
        N++;
      }
      return LROUND(SQRT(S2 / N) * 1000.0f) / 1000.0f + 0.00001f;
    }
  }
  return 0.00001f;
}

/**
 * Probe a point
 */
static float calibration_probe(const xy_pos_t &xy, const bool stow) {
  return probe.check_at_point(xy, stow ? PROBE_PT_STOW : PROBE_PT_RAISE, 0, false);
}

static bool probe_calibration_points(float z_pt[NPP + 1], const uint8_t probe_points, const bool towers_set, const bool stow_after_each) {

  const bool  _0p_calibration      = probe_points == 0,
              _1p_calibration      = probe_points == 1,
              _4p_calibration      = probe_points == 2,
              _4p_opposite_points  = _4p_calibration && !towers_set,
              _7p_calibration      = probe_points >= 3 || probe_points == 0,
              _7p_no_intermediates = probe_points == 3,
              _7p_1_intermediates  = probe_points == 4,
              _7p_2_intermediates  = probe_points == 5,
              _7p_4_intermediates  = probe_points == 6,
              _7p_6_intermediates  = probe_points == 7,
              _7p_8_intermediates  = probe_points == 8,
              _7p_11_intermediates = probe_points == 9,
              _7p_14_intermediates = probe_points == 10,
              _7p_intermed_points  = probe_points >= 4,
              _7p_6_center         = probe_points >= 5 && probe_points <= 7,
              _7p_9_center         = probe_points >= 8;

  LOOP_CAL_ALL(rad) z_pt[rad] = 0.0f;

  if (!_0p_calibration) {

    if (!_7p_no_intermediates && !_7p_4_intermediates && !_7p_11_intermediates) { // probe the center
      const xy_pos_t center{0};
      z_pt[CEN] += calibration_probe(center, stow_after_each);
      if (isnan(z_pt[CEN])) return false;
    }

    if (_7p_calibration) {  // probe extra center points
      const float start  = _7p_9_center ? float(_CA) + _7P_STEP / 3.0f : _7p_6_center ? float(_CA) : float(__C),
                  steps  = _7p_9_center ? _4P_STEP / 3.0f : _7p_6_center ? _7P_STEP : _4P_STEP;
      I_LOOP_CAL_PT(rad, start, steps) {
        const float a = RADIANS(210 + (360 / NPP) *  (rad - 1)),
                    r = mechanics.data.probe_radius * 0.1f;
        const xy_pos_t vec = { cos(a), sin(a) };
        z_pt[CEN] += calibration_probe(vec * r, stow_after_each);
        if (isnan(z_pt[CEN])) return false;
      }
      z_pt[CEN] /= float(_7p_2_intermediates ? 7 : probe_points);
    }

    if (!_1p_calibration) {   // probe the radius
      const uint8_t start  =  _4p_opposite_points  ? _AB : __A;
      const float   steps  =  _7p_14_intermediates ? _7P_STEP / 15.0f : // 15r * 6 + 10c = 100
                              _7p_11_intermediates ? _7P_STEP / 12.0f : // 12r * 6 +  9c = 81
                              _7p_8_intermediates  ? _7P_STEP /  9.0f : //  9r * 6 + 10c = 64
                              _7p_6_intermediates  ? _7P_STEP /  7.0f : //  7r * 6 +  7c = 49
                              _7p_4_intermediates  ? _7P_STEP /  5.0f : //  5r * 6 +  6c = 36
                              _7p_2_intermediates  ? _7P_STEP /  3.0f : //  3r * 6 +  7c = 25
                              _7p_1_intermediates  ? _7P_STEP /  2.0f : //  2r * 6 +  4c = 16
                              _7p_no_intermediates ? _7P_STEP :         //  1r * 6 +  3c = 9
                              _4P_STEP;                                 // .5r * 6 +  1c = 4

      bool zig_zag = true;
      F_LOOP_CAL_PT(rad, start, _7p_9_center ? steps * 3 : steps) {
        const int8_t offset = _7p_9_center ? 2 : 0;
        for (int8_t circle = 0; circle <= offset; circle++) {
          const float a = RADIANS(210 + (360 / NPP) *  (rad - 1)),
                      r = mechanics.data.probe_radius * (1 - 0.1 * (zig_zag ? offset - circle : circle)),
                      interpol = FMOD(rad, 1);
          const xy_pos_t vec = { cos(a), sin(a) };
          const float z_temp = calibration_probe(vec * r, stow_after_each);
          if (isnan(z_temp)) return false;
          // split probe point to neighbouring calibration points
          z_pt[uint8_t(LROUND(rad - interpol + NPP - 1)) % NPP + 1] += z_temp * sq(COS(RADIANS(interpol * 90)));
          z_pt[uint8_t(LROUND(rad - interpol))           % NPP + 1] += z_temp * sq(SIN(RADIANS(interpol * 90)));
        }
        zig_zag = !zig_zag;
      }

      if (_7p_intermed_points) {
        LOOP_CAL_RAD(rad) {
          z_pt[rad] /= _7P_STEP / steps;
        }
      }

      // goto centre
      mechanics.do_blocking_move_to_xy(0.0f, 0.0f);
    }
  }
  return true;
}

/**
 * kinematics routines and auto tune matrix scaling parameters:
 *  - formulae for approximative forward kinematics in the end-stop displacement matrix
 *  - definition of the matrix scaling parameters
 */
static void reverse_kinematics_probe_points(float z_pt[NPP + 1], abc_float_t mm_at_pt_axis[NPP + 1]) {
  xyz_pos_t pos{0};

  LOOP_CAL_ALL(rad) {
    const float a = RADIANS(210 + (360 / NPP) *  (rad - 1)),
                r = (rad == CEN ? 0.0f : mechanics.data.probe_radius);
    pos.set(cos(a) * r, sin(a) * r, z_pt[rad]);
    mechanics.Transform(pos);
    mm_at_pt_axis[rad] = mechanics.delta;
  }
}

static void forward_kinematics_probe_points(abc_float_t mm_at_pt_axis[NPP + 1], float z_pt[NPP + 1]) {
  const float r_quot = mechanics.data.probe_radius / mechanics.data.radius;

  #define ZPP(N,I,A) (((1.0f + r_quot * (N)) / 3.0f) * mm_at_pt_axis[I].A)
  #define Z00(I, A) ZPP( 0, I, A)
  #define Zp1(I, A) ZPP(+1, I, A)
  #define Zm1(I, A) ZPP(-1, I, A)
  #define Zp2(I, A) ZPP(+2, I, A)
  #define Zm2(I, A) ZPP(-2, I, A)

  z_pt[CEN] = Z00(CEN, a) + Z00(CEN, b) + Z00(CEN, c);
  z_pt[__A] = Zp2(__A, a) + Zm1(__A, b) + Zm1(__A, c);
  z_pt[__B] = Zm1(__B, a) + Zp2(__B, b) + Zm1(__B, c);
  z_pt[__C] = Zm1(__C, a) + Zm1(__C, b) + Zp2(__C, c);
  z_pt[_BC] = Zm2(_BC, a) + Zp1(_BC, b) + Zp1(_BC, c);
  z_pt[_CA] = Zp1(_CA, a) + Zm2(_CA, b) + Zp1(_CA, c);
  z_pt[_AB] = Zp1(_AB, a) + Zp1(_AB, b) + Zm2(_AB, c);
}

static void calc_kinematics_diff_probe_points(float z_pt[NPP + 1], abc_float_t delta_e, float delta_r, abc_float_t delta_t) {
  const float z_center = z_pt[CEN];
  abc_float_t diff_mm_at_pt_axis[NPP + 1],
              new_mm_at_pt_axis[NPP + 1];

  reverse_kinematics_probe_points(z_pt, diff_mm_at_pt_axis);

  mechanics.data.radius += delta_r;
  mechanics.data.tower_angle_adj += delta_t;
  mechanics.recalc_delta_settings();
  reverse_kinematics_probe_points(z_pt, new_mm_at_pt_axis);

  LOOP_CAL_ALL(rad) diff_mm_at_pt_axis[rad] -= new_mm_at_pt_axis[rad] + delta_e;
  forward_kinematics_probe_points(diff_mm_at_pt_axis, z_pt);

  LOOP_CAL_RAD(rad) z_pt[rad] -= z_pt[CEN] - z_center;
  z_pt[CEN] = z_center;

  mechanics.data.radius -= delta_r;
  mechanics.data.tower_angle_adj -= delta_t;
  mechanics.recalc_delta_settings();
}

static float auto_tune_h() {
  const float r_quot = mechanics.data.probe_radius / mechanics.data.radius;

  float h_fac = r_quot / (2.0f / 3.0f);
  h_fac = 1.0f / h_fac; // (2/3)/CR

  return h_fac;
}

static float auto_tune_r() {
  constexpr float diff = 0.01f, delta_r = diff;
  float r_fac = 0.0f, z_pt[NPP + 1] = { 0.0f };
  abc_float_t delta_e = { 0.0f }, delta_t = { 0.0f };

  calc_kinematics_diff_probe_points(z_pt, delta_e, delta_r, delta_t);     
  r_fac = -(z_pt[__A] + z_pt[__B] + z_pt[__C] + z_pt[_BC] + z_pt[_CA] + z_pt[_AB]) / 6.0f;
  r_fac = diff / r_fac / 3.0f; // 1/(3*delta_Z)

  return r_fac;
}

static float auto_tune_a() {
  constexpr float diff = 0.01f, delta_r = 0.0f;
  float a_fac = 0.0f, z_pt[NPP + 1] = { 0.0f };
  abc_float_t delta_e = { 0.0f }, delta_t = { 0.0f };

  delta_t.reset();
  LOOP_XYZ(axis) {
    delta_t[axis] = diff;
    calc_kinematics_diff_probe_points(z_pt, delta_e, delta_r, delta_t);
    delta_t[axis] = 0;
    a_fac += z_pt[uint8_t((axis * _4P_STEP) - _7P_STEP + NPP) % NPP + 1] / 6.0f;
    a_fac -= z_pt[uint8_t((axis * _4P_STEP) + 1 + _7P_STEP)] / 6.0f;
  }
  a_fac = diff / a_fac / 3.0f; // 1/(3*delta_Z)

  return a_fac;
}

/**
 * Delta AutoCalibration Algorithm based on Luc Van Daele LVD-AC
 *       Calibrate height, endstops, delta radius and tower angles.
 *
 * Parameters:
 *
 *   Pn Number of probe points:
 *      P0     Normalizes settings without probing.
 *      P1     Calibrates height only with center probe.
 *      P2     Probe center and towers. Calibrate height, endstops and delta radius.
 *      P3     Probe all positions: center, towers and opposite towers. Calibrate all.
 *      P4-P10 Probe all positions at different itermediate locations and average them.
 *
 *   T   Don't calibrate tower angle corrections
 *
 *   Cn.nn  Calibration precision; when omitted calibrates to maximum precision
 *
 *   Fn  Force to run at least n iterations and takes the best result
 *
 *   Vn  Verbose level:
 *      V0  Dry-run mode. Report settings and probe results. No calibration.
 *      V1  Report start and end settings only
 *      V2  Report settings at each iteration
 *      V3  Report settings and probe results
 *
 *   E   Engage the probe for each point
 */
inline void gcode_G33() {

  const uint8_t probe_points = parser.intval('P', DELTA_AUTO_CALIBRATION_2_DEFAULT_POINTS);
  if (!WITHIN(probe_points, 0, 10)) {
    SERIAL_EM("?(P)oints is implausible (0-10).");
    return;
  }

  const bool towers_set = !parser.seen('T');

  const float calibration_precision = parser.floatval('C', 0.0f);
  if (calibration_precision < 0) {
    SERIAL_EM("?(C)alibration precision is implausible (>=0).");
    return;
  }

  const int8_t force_iterations = parser.intval('F', 0);
  if (!WITHIN(force_iterations, 0, 30)) {
    SERIAL_EM("?(F)orce iteration is implausible (0-30).");
    return;
  }

  const int8_t verbose_level = parser.byteval('V', 1);
  if (!WITHIN(verbose_level, 0, 3)) {
    SERIAL_EM("?(V)erbose Level is implausible (0-3).");
    return;
  }

  const bool stow_after_each = parser.seen('E');

  const bool  _0p_calibration     = probe_points == 0,
              _1p_calibration     = probe_points == 1,
              _4p_calibration     = probe_points == 2,
              _4p_opposite_points = _4p_calibration && !towers_set,
              _7p_9_center        = probe_points >= 8,
              _tower_results      = (_4p_calibration && towers_set) || probe_points >= 3,
              _opposite_results   = (_4p_calibration && !towers_set) || probe_points >= 3,
              _endstop_results    = probe_points != 1 && probe_points != 0,
              _angle_results      = probe_points >= 3 && towers_set;

  SFSTRINGVALUE(save_message, "Save with M500 and/or copy to configuration_delta.h");

  int8_t iterations = 0;
  float test_precision,
        zero_std_dev = (verbose_level ? 999.0f : 0.0f), // 0.0f in dry-run mode : forced end
        zero_std_dev_min = zero_std_dev,
        zero_std_dev_old = zero_std_dev,
        h_factor,
        r_factor,
        a_factor,
        r_old = mechanics.data.radius,
        h_old = mechanics.data.height;

  abc_pos_t e_old = mechanics.data.endstop_adj,
            a_old = mechanics.data.tower_angle_adj;

  const float dcr = mechanics.data.probe_radius;

  if (!_1p_calibration && !_0p_calibration) {  // test if the outer radius is reachable
    LOOP_CAL_RAD(axis) {
      const float a = RADIANS(210 + (360 / NPP) * (axis - 1));
      if (!mechanics.position_is_reachable(COS(a) * dcr, SIN(a) * dcr)) {
        SERIAL_EM("?(M666 P)robe radius is implausible.");
        return;
      }
    }
  }

  // Report settings
  SERIAL_STR(GET_TEXT(MSG_DELTA_AUTO_CALIBRATE));
  if (verbose_level == 0) SERIAL_MSG(" (DRY-RUN)");
  SERIAL_EOL();
  LCD_MESSAGEPGM(MSG_DELTA_AUTO_CALIBRATE);

  Report_settings(_endstop_results, _angle_results);

  ac_setup(!_0p_calibration && !_1p_calibration);

  if (!_0p_calibration) ac_home();

  // start iterations
  do {

    float z_at_pt[NPP + 1] = { 0.0f };

    test_precision = zero_std_dev_old != 999.0f ? (zero_std_dev + zero_std_dev_old) / 2.0f : zero_std_dev;

    iterations++;

    // Probe the points
    zero_std_dev_old = zero_std_dev;
    if (!probe_calibration_points(z_at_pt, probe_points, towers_set, stow_after_each)) {
      SERIAL_EM("Correct data.radius with M666 R or end-stops with M666 X Y Z");
      ac_cleanup();
      return;
    }
    zero_std_dev = std_dev_points(z_at_pt, _0p_calibration, _1p_calibration, _4p_calibration, _4p_opposite_points);

    // Solve matrices

    if ((zero_std_dev < test_precision || iterations <= force_iterations) && zero_std_dev > calibration_precision) {

      #if !HAS_BED_PROBE
        test_precision = 0.0f; // forced end
      #endif

      if (zero_std_dev < zero_std_dev_min) {
        // set roll-back point
        e_old = mechanics.data.endstop_adj;
        r_old = mechanics.data.radius;
        h_old = mechanics.data.height;
        a_old = mechanics.data.tower_angle_adj;
      }

      abc_float_t e_delta = { 0.0f }, t_delta = { 0.0f };
      float r_delta = 0.0f;

      /**
       * convergence matrices:
       *  - definition of the matrix scaling parameters
       *  - matrices for 4 and 7 point calibration
       */
      #define ZP(N,I) ((N) * z_at_pt[I] / 4.0f) // 4.0f = divider to normalize to integers
      #define Z12(I)  ZP(12, I)
      #define Z4(I)   ZP(4, I)
      #define Z2(I)   ZP(2, I)
      #define Z1(I)   ZP(1, I)
      #define Z0(I)   ZP(0, I)

      const float cr_old = mechanics.data.probe_radius;
      if (_7p_9_center) mechanics.data.probe_radius *= 0.9f;
      h_factor = auto_tune_h();
      r_factor = auto_tune_r();
      a_factor = auto_tune_a();
      mechanics.data.probe_radius = cr_old;

      switch (probe_points) {
        case 0:
          test_precision = 0.0f;  // forced end
          break;

        case 1:
          test_precision = 0.0f;  // forced end
          LOOP_XYZ(axis) e_delta[axis] = +Z4(CEN);
          break;

        case 2:
          if (towers_set) { // see 4 point calibration (towers) matrix
            e_delta.set((+Z4(__A) -Z2(__B) -Z2(__C)) * h_factor  +Z4(CEN),
                        (-Z2(__A) +Z4(__B) -Z2(__C)) * h_factor  +Z4(CEN),
                        (-Z2(__A) -Z2(__B) +Z4(__C)) * h_factor  +Z4(CEN));
            r_delta   = (+Z4(__A) +Z4(__B) +Z4(__C) -Z12(CEN)) * r_factor;
          }
          else { // see 4 point calibration (opposites) matrix
            e_delta.set((-Z4(_BC) +Z2(_CA) +Z2(_AB)) * h_factor  +Z4(CEN),
                        (+Z2(_BC) -Z4(_CA) +Z2(_AB)) * h_factor  +Z4(CEN),
                        (+Z2(_BC) +Z2(_CA) -Z4(_AB)) * h_factor  +Z4(CEN));
            r_delta   = (+Z4(_BC) +Z4(_CA) +Z4(_AB) -Z12(CEN)) * r_factor;
          }
          break;

        default: // see 7 point calibration (towers & opposites) matrix
          e_delta.set((+Z2(__A) -Z1(__B) -Z1(__C) -Z2(_BC) +Z1(_CA) +Z1(_AB)) * h_factor  +Z4(CEN),
                      (-Z1(__A) +Z2(__B) -Z1(__C) +Z1(_BC) -Z2(_CA) +Z1(_AB)) * h_factor  +Z4(CEN),
                      (-Z1(__A) -Z1(__B) +Z2(__C) +Z1(_BC) +Z1(_CA) -Z2(_AB)) * h_factor  +Z4(CEN));
          r_delta   = (+Z2(__A) +Z2(__B) +Z2(__C) +Z2(_BC) +Z2(_CA) +Z2(_AB) -Z12(CEN)) * r_factor;

          if (towers_set) { // see 7 point tower angle calibration (towers & opposites) matrix
            t_delta.set((+Z0(__A) -Z4(__B) +Z4(__C) +Z0(_BC) -Z4(_CA) +Z4(_AB) +Z0(CEN)) * a_factor,
                        (+Z4(__A) +Z0(__B) -Z4(__C) +Z4(_BC) +Z0(_CA) -Z4(_AB) +Z0(CEN)) * a_factor,
                        (-Z4(__A) +Z4(__B) +Z0(__C) -Z4(_BC) +Z4(_CA) +Z0(_AB) +Z0(CEN)) * a_factor);
          }
          break;
      }
      mechanics.data.endstop_adj += e_delta;
      mechanics.data.radius += r_delta;
      mechanics.data.tower_angle_adj += t_delta;
    }
    else if (zero_std_dev >= test_precision) {
      // roll back
      mechanics.data.endstop_adj = e_old;
      mechanics.data.radius = r_old;
      mechanics.data.height = h_old;
      mechanics.data.tower_angle_adj = a_old;
    }

    if (verbose_level != 0) {                                    // !dry run

      // Normalise angles to least squares
      if (_angle_results) {
        float a_sum = 0.0f;
        LOOP_XYZ(axis) a_sum += mechanics.data.tower_angle_adj[axis];
        LOOP_XYZ(axis) mechanics.data.tower_angle_adj[axis] -= a_sum / 3.0f;
      }

      // Adjust data.height and endstops by the max amount
      const float z_temp = MAX(mechanics.data.endstop_adj.a, mechanics.data.endstop_adj.b, mechanics.data.endstop_adj.c);
      mechanics.data.height -= z_temp;
      LOOP_XYZ(axis) mechanics.data.endstop_adj[axis] -= z_temp;
    }
    mechanics.recalc_delta_settings();
    NOMORE(zero_std_dev_min, zero_std_dev);

    // Report results
    if (verbose_level > 2)
      Report_results(z_at_pt, _tower_results, _opposite_results);

    if (verbose_level != 0) {                                    // !dry run
      if ((zero_std_dev >= test_precision && iterations > force_iterations) || zero_std_dev <= calibration_precision) {  // end iterations
        SERIAL_MSG("Calibration OK");
        SERIAL_SP(32);
        #if HAS_BED_PROBE
          if (zero_std_dev >= test_precision && !_1p_calibration && !_0p_calibration)
            SERIAL_MSG("rolling back.");
          else
        #endif
        {
          SERIAL_MV("std dev:", zero_std_dev_min, 3);
        }

        SERIAL_EOL();
        char mess[21];
        strcpy_P(mess, PSTR("Calibration sd:"));
        if (zero_std_dev_min < 1)
          sprintf_P(&mess[15], PSTR("0.%03i"), (int)LROUND(zero_std_dev_min * 1000.0f));
        else
          sprintf_P(&mess[15], PSTR("%03i.x"), (int)LROUND(zero_std_dev_min));
        lcdui.set_status(mess);
        Report_settings(_endstop_results, _angle_results);
        SERIAL_STR(save_message);
        SERIAL_EOL();
      }
      else {                                                     // !end iterations
        char mess[15];
        if (iterations < 31)
          sprintf_P(mess, PSTR("Iteration : %02i"), (int)iterations);
        else
          strcpy_P(mess, PSTR("No convergence"));
        SERIAL_TXT(mess);
        SERIAL_SP(32);
        SERIAL_EMV("std dev:", zero_std_dev, 3);
        lcdui.set_status(mess);
        if (verbose_level > 1)
          Report_settings(_endstop_results, _angle_results);
      }
    }
    else { // dry run
      PGM_P enddryrun = PSTR("End DRY-RUN");
      SERIAL_STR(enddryrun);
      SERIAL_SP(35);
      SERIAL_EMV("std dev:", zero_std_dev, 3);

      char mess[21];
      strcpy_P(mess, enddryrun);
      strcpy_P(&mess[11], PSTR(" sd:"));
      if (zero_std_dev < 1)
        sprintf_P(&mess[15], PSTR("0.%03i"), (int)LROUND(zero_std_dev * 1000.0f));
      else
        sprintf_P(&mess[15], PSTR("%03i.x"), (int)LROUND(zero_std_dev));
      lcdui.set_status(mess);
    }

    ac_home();

  } while (((zero_std_dev < test_precision && iterations < 31) || iterations <= force_iterations) && zero_std_dev > calibration_precision);

  ac_cleanup();
}

#endif // ENABLED(DELTA_AUTO_CALIBRATION_2)
