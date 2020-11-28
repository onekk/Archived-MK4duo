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
 * planner.cpp
 *
 * Buffer movement commands and manage the acceleration profile plan
 *
 * Derived from Grbl
 * Copyright (c) 2009-2011 Simen Svale Skogsrud
 *
 * The ring buffer implementation gleaned from the wiring_serial library by David A. Mellis.
 *
 *
 * Reasoning behind the mathematics in this module (in the key of 'Mathematica'):
 *
 * s == speed, a == acceleration, t == time, d == distance
 *
 * Basic definitions:
 *   Speed[s_, a_, t_] := s + (a*t)
 *   Travel[s_, a_, t_] := Integrate[Speed[s, a, t], t]
 *
 * Distance to reach a specific speed with a constant acceleration:
 *   Solve[{Speed[s, a, t] == m, Travel[s, a, t] == d}, d, t]
 *   d -> (m^2 - s^2)/(2 a) --> estimate_acceleration_distance()
 *
 * Speed after a given distance of travel with constant acceleration:
 *   Solve[{Speed[s, a, t] == m, Travel[s, a, t] == d}, m, t]
 *   m -> Sqrt[2 a d + s^2]
 *
 * DestinationSpeed[s_, a_, d_] := Sqrt[2 a d + s^2]
 *
 * When to start braking (di) to reach a specified destination speed (s2) after accelerating
 * from initial speed s1 without ever stopping at a plateau:
 *   Solve[{DestinationSpeed[s1, a, di] == DestinationSpeed[s2, a, d - di]}, di]
 *   di -> (2 a d - s1^2 + s2^2)/(4 a) --> intersection_distance()
 *
 * IntersectionDistance[s1_, s2_, a_, d_] := (2 a d - s1^2 + s2^2)/(4 a)
 *
 * --
 *
 * The fast inverse function needed for Bézier interpolation for AVR
 * was designed, written and tested by Eduardo José Tagle on April/2018
 */

#include "../../../MK4duo.h"

// Delay for delivery of first block to the stepper ISR, if the queue contains 2 or
// less movements. The delay is measured in milliseconds, and must be less than 250ms
#define BLOCK_DELAY_FOR_1ST_MOVE 100

Planner planner;

/** Public Parameters */
plan_flag_t       Planner::flag;

block_t           Planner::block_buffer[BLOCK_BUFFER_SIZE];

volatile uint8_t  Planner::block_buffer_head        = 0,
                  Planner::block_buffer_nonbusy     = 0,
                  Planner::block_buffer_planned     = 0,
                  Planner::block_buffer_tail        = 0;

uint8_t           Planner::delay_before_delivering  = 0;

#if HAS_POSITION_FLOAT
  xyze_pos_t Planner::position_float{0.0f};
#endif

#if IS_KINEMATIC
  xyze_pos_t Planner::position_cart{0.0f};
#endif

#if HAS_TEMP_HOTEND && ENABLED(AUTOTEMP)
  float Planner::autotemp_max     = 250,
        Planner::autotemp_min     = 210,
        Planner::autotemp_factor  = 0.1;
#endif

/** Private Parameters */
xyze_long_t   Planner::position = { 0, 0 ,0 ,0 };

xyze_float_t  Planner::previous_speed = { 0.0, 0.0, 0.0, 0.0 };

float Planner::previous_nominal_speed_sqr = 0.0;

uint32_t Planner::cutoff_long = 0;

#if ENABLED(DISABLE_INACTIVE_EXTRUDER)
  uint8_t Planner::g_uc_extruder_last_move[MAX_EXTRUDER] = { 0 };
#endif

#if HAS_SPI_LCD
  volatile uint32_t Planner::block_buffer_runtime_us = 0;
#endif

/** Public Function */
void Planner::init() {
  position.reset();
  #if HAS_POSITION_FLOAT
    position_float.reset();
  #endif
  #if IS_KINEMATIC
    position_cart.reset();
  #endif
  previous_speed.reset();
  previous_nominal_speed_sqr = 0.0f;
  #if ABL_PLANAR
    bedlevel.matrix.set_to_identity();
  #endif
  flag.all = 0x00;
  clear_block_buffer();
  delay_before_delivering = 0;
}

#if ENABLED(BEZIER_JERK_CONTROL)

  #if ENABLED(__AVR__)

    // This routine, for AVR, returns 0x1000000 / d, but trying to get the inverse as
    //  fast as possible. A fast converging iterative Newton-Raphson method is able to
    //  reach full precision in just 1 iteration, and takes 211 cycles (worst case, mean
    //  case is less, up to 30 cycles for small divisors), instead of the 500 cycles a
    //  normal division would take.
    //
    // Inspired by the following page,
    //  https://stackoverflow.com/questions/27801397/newton-raphson-division-with-big-integers
    //
    // Suppose we want to calculate
    //  floor(2 ^ k / B)    where B is a positive integer
    // Then
    //  B must be <= 2^k, otherwise, the quotient is 0.
    //
    // The Newton - Raphson iteration for x = B / 2 ^ k yields:
    //  q[n + 1] = q[n] * (2 - q[n] * B / 2 ^ k)
    //
    // We can rearrange it as:
    //  q[n + 1] = q[n] * (2 ^ (k + 1) - q[n] * B) >> k
    //
    //  Each iteration of this kind requires only integer multiplications
    // and bit shifts.
    //  Does it converge to floor(2 ^ k / B) ?:  Not necessarily, but, in
    // the worst case, it eventually alternates between floor(2 ^ k / B)
    // and ceiling(2 ^ k / B)).
    //  So we can use some not-so-clever test to see if we are in this
    // case, and extract floor(2 ^ k / B).
    //  Lastly, a simple but important optimization for this approach is to
    // truncate multiplications (i.e.calculate only the higher bits of the
    // product) in the early iterations of the Newton - Raphson method.The
    // reason to do so, is that the results of the early iterations are far
    // from the quotient, and it doesn't matter to perform them inaccurately.
    //  Finally, we should pick a good starting value for x. Knowing how many
    // digits the divisor has, we can estimate it:
    //
    // 2^k / x = 2 ^ log2(2^k / x)
    // 2^k / x = 2 ^(log2(2^k)-log2(x))
    // 2^k / x = 2 ^(k*log2(2)-log2(x))
    // 2^k / x = 2 ^ (k-log2(x))
    // 2^k / x >= 2 ^ (k-floor(log2(x)))
    // floor(log2(x)) simply is the index of the most significant bit set.
    //
    //  If we could improve this estimation even further, then the number of
    // iterations can be dropped quite a bit, thus saving valuable execution time.
    //  The paper "Software Integer Division" by Thomas L.Rodeheffer, Microsoft
    // Research, Silicon Valley,August 26, 2008, that is available at
    // https://www.microsoft.com/en-us/research/wp-content/uploads/2008/08/tr-2008-141.pdf
    // suggests , for its integer division algorithm, that using a table to supply the
    // first 8 bits of precision, and due to the quadratic convergence nature of the
    // Newton-Raphon iteration, then just 2 iterations should be enough to get
    // maximum precision of the division.
    //  If we precompute values of inverses for small denominator values, then
    // just one Newton-Raphson iteration is enough to reach full precision
    //  We will use the top 9 bits of the denominator as index.
    //
    //  The AVR assembly function is implementing the following C code, included
    // here as reference:
    //
    // uint32_t get_period_inverse(uint32_t d) {
    //  static const uint8_t inv_tab[256] = {
    //    255,253,252,250,248,246,244,242,240,238,236,234,233,231,229,227,
    //    225,224,222,220,218,217,215,213,212,210,208,207,205,203,202,200,
    //    199,197,195,194,192,191,189,188,186,185,183,182,180,179,178,176,
    //    175,173,172,170,169,168,166,165,164,162,161,160,158,157,156,154,
    //    153,152,151,149,148,147,146,144,143,142,141,139,138,137,136,135,
    //    134,132,131,130,129,128,127,126,125,123,122,121,120,119,118,117,
    //    116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,
    //    100,99,98,97,96,95,94,93,92,91,90,89,88,88,87,86,
    //    85,84,83,82,81,80,80,79,78,77,76,75,74,74,73,72,
    //    71,70,70,69,68,67,66,66,65,64,63,62,62,61,60,59,
    //    59,58,57,56,56,55,54,53,53,52,51,50,50,49,48,48,
    //    47,46,46,45,44,43,43,42,41,41,40,39,39,38,37,37,
    //    36,35,35,34,33,33,32,32,31,30,30,29,28,28,27,27,
    //    26,25,25,24,24,23,22,22,21,21,20,19,19,18,18,17,
    //    17,16,15,15,14,14,13,13,12,12,11,10,10,9,9,8,
    //    8,7,7,6,6,5,5,4,4,3,3,2,2,1,0,0
    //  };
    //
    //  // For small denominators, it is cheaper to directly store the result,
    //  //  because those denominators would require 2 Newton-Raphson iterations
    //  //  to converge to the required result precision. For bigger ones, just
    //  //  ONE Newton-Raphson iteration is enough to get maximum precision!
    //  static const uint32_t small_inv_tab[111] PROGMEM = {
    //    16777216,16777216,8388608,5592405,4194304,3355443,2796202,2396745,2097152,1864135,1677721,1525201,1398101,1290555,1198372,1118481,
    //    1048576,986895,932067,883011,838860,798915,762600,729444,699050,671088,645277,621378,599186,578524,559240,541200,
    //    524288,508400,493447,479349,466033,453438,441505,430185,419430,409200,399457,390167,381300,372827,364722,356962,
    //    349525,342392,335544,328965,322638,316551,310689,305040,299593,294337,289262,284359,279620,275036,270600,266305,
    //    262144,258111,254200,250406,246723,243148,239674,236298,233016,229824,226719,223696,220752,217885,215092,212369,
    //    209715,207126,204600,202135,199728,197379,195083,192841,190650,188508,186413,184365,182361,180400,178481,176602,
    //    174762,172960,171196,169466,167772,166111,164482,162885,161319,159783,158275,156796,155344,153919,152520
    //  };
    //
    //  // For small divisors, it is best to directly retrieve the results
    //  if (d <= 110)
    //    return pgm_read_dword(&small_inv_tab[d]);
    //
    //  // Compute initial estimation of 0x1000000/x -
    //  // Get most significant bit set on divider
    //  uint8_t idx = 0;
    //  uint32_t nr = d;
    //  if (!(nr & 0xFF0000)) {
    //    nr <<= 8;
    //    idx += 8;
    //    if (!(nr & 0xFF0000)) {
    //      nr <<= 8;
    //      idx += 8;
    //    }
    //  }
    //  if (!(nr & 0xF00000)) {
    //    nr <<= 4;
    //    idx += 4;
    //  }
    //  if (!(nr & 0xC00000)) {
    //    nr <<= 2;
    //    idx += 2;
    //  }
    //  if (!(nr & 0x800000)) {
    //    nr <<= 1;
    //    idx += 1;
    //  }
    //
    //  // Isolate top 9 bits of the denominator, to be used as index into the initial estimation table
    //  uint32_t tidx = nr >> 15;         // top 9 bits. bit8 is always set
    //  uint32_t ie = inv_tab[tidx & 0xFF] + 256; // Get the table value. bit9 is always set
    //  uint32_t x = idx <= 8 ? (ie >> (8 - idx)) : (ie << (idx - 8)); // Position the estimation at the proper place
    //
    //  // Now, refine estimation by newton-raphson. 1 iteration is enough
    //  x = uint32_t((x * uint64_t((1 << 25) - x * d)) >> 24);
    //
    //  // Estimate remainder
    //  uint32_t r = (1 << 24) - x * d;
    //
    //  // Check if we must adjust result
    //  if (r >= d) x++;
    //
    //  // x holds the proper estimation
    //  return uint32_t(x);
    // }
    //
    static uint32_t get_period_inverse(uint32_t d) {

      static const uint8_t inv_tab[256] PROGMEM = {
        255,253,252,250,248,246,244,242,240,238,236,234,233,231,229,227,
        225,224,222,220,218,217,215,213,212,210,208,207,205,203,202,200,
        199,197,195,194,192,191,189,188,186,185,183,182,180,179,178,176,
        175,173,172,170,169,168,166,165,164,162,161,160,158,157,156,154,
        153,152,151,149,148,147,146,144,143,142,141,139,138,137,136,135,
        134,132,131,130,129,128,127,126,125,123,122,121,120,119,118,117,
        116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,
        100,99,98,97,96,95,94,93,92,91,90,89,88,88,87,86,
        85,84,83,82,81,80,80,79,78,77,76,75,74,74,73,72,
        71,70,70,69,68,67,66,66,65,64,63,62,62,61,60,59,
        59,58,57,56,56,55,54,53,53,52,51,50,50,49,48,48,
        47,46,46,45,44,43,43,42,41,41,40,39,39,38,37,37,
        36,35,35,34,33,33,32,32,31,30,30,29,28,28,27,27,
        26,25,25,24,24,23,22,22,21,21,20,19,19,18,18,17,
        17,16,15,15,14,14,13,13,12,12,11,10,10,9,9,8,
        8,7,7,6,6,5,5,4,4,3,3,2,2,1,0,0
      };

      // For small denominators, it is cheaper to directly store the result.
      //  For bigger ones, just ONE Newton-Raphson iteration is enough to get
      //  maximum precision we need
      static const uint32_t small_inv_tab[111] PROGMEM = {
        16777216,16777216,8388608,5592405,4194304,3355443,2796202,2396745,2097152,1864135,1677721,1525201,1398101,1290555,1198372,1118481,
        1048576,986895,932067,883011,838860,798915,762600,729444,699050,671088,645277,621378,599186,578524,559240,541200,
        524288,508400,493447,479349,466033,453438,441505,430185,419430,409200,399457,390167,381300,372827,364722,356962,
        349525,342392,335544,328965,322638,316551,310689,305040,299593,294337,289262,284359,279620,275036,270600,266305,
        262144,258111,254200,250406,246723,243148,239674,236298,233016,229824,226719,223696,220752,217885,215092,212369,
        209715,207126,204600,202135,199728,197379,195083,192841,190650,188508,186413,184365,182361,180400,178481,176602,
        174762,172960,171196,169466,167772,166111,164482,162885,161319,159783,158275,156796,155344,153919,152520
      };

      // For small divisors, it is best to directly retrieve the results
      if (d <= 110) return pgm_read_dword(&small_inv_tab[d]);

      uint8_t r8 = d & 0xFF,
              r9 = (d >> 8) & 0xFF,
              r10 = (d >> 16) & 0xFF,
              r2, r3, r4, r5, r6, r7, r11, r12, r13, r14, r15, r16, r17, r18;

      const uint8_t* ptab = inv_tab;

      __asm__ __volatile__(
        // %8:%7:%6 = interval
        // r31:r30: MUST be those registers, and they must point to the inv_tab

        A("clr %13")                       // %13 = 0

        // Now we must compute
        // result = 0xFFFFFF / d
        // %8:%7:%6 = interval
        // %16:%15:%14 = nr
        // %13 = 0

        // A plain division of 24x24 bits should take 388 cycles to complete. We will
        // use Newton-Raphson for the calculation, and will strive to get way less cycles
        // for the same result - Using C division, it takes 500cycles to complete .

        A("clr %3")                       // idx = 0
        A("mov %14,%6")
        A("mov %15,%7")
        A("mov %16,%8")                   // nr = interval
        A("tst %16")                      // nr & 0xFF0000 == 0 ?
        A("brne 2f")                      // No, skip this
        A("mov %16,%15")
        A("mov %15,%14")                  // nr <<= 8, %14 not needed
        A("subi %3,-8")                   // idx += 8
        A("tst %16")                      // nr & 0xFF0000 == 0 ?
        A("brne 2f")                      // No, skip this
        A("mov %16,%15")                  // nr <<= 8, %14 not needed
        A("clr %15")                      // We clear %14
        A("subi %3,-8")                   // idx += 8

        // here %16 != 0 and %16:%15 contains at least 9 MSBits, or both %16:%15 are 0
        L("2")
        A("cpi %16,0x10")                 // (nr & 0xF00000) == 0 ?
        A("brcc 3f")                      // No, skip this
        A("swap %15")                     // Swap nibbles
        A("swap %16")                     // Swap nibbles. Low nibble is 0
        A("mov %14, %15")
        A("andi %14,0x0F")                // Isolate low nibble
        A("andi %15,0xF0")                // Keep proper nibble in %15
        A("or %16, %14")                  // %16:%15 <<= 4
        A("subi %3,-4")                   // idx += 4

        L("3")
        A("cpi %16,0x40")                 // (nr & 0xC00000) == 0 ?
        A("brcc 4f")                      // No, skip this
        A("add %15,%15")
        A("adc %16,%16")
        A("add %15,%15")
        A("adc %16,%16")                  // %16:%15 <<= 2
        A("subi %3,-2")                   // idx += 2

        L("4")
        A("cpi %16,0x80")                 // (nr & 0x800000) == 0 ?
        A("brcc 5f")                      // No, skip this
        A("add %15,%15")
        A("adc %16,%16")                  // %16:%15 <<= 1
        A("inc %3")                       // idx += 1

        // Now %16:%15 contains its MSBit set to 1, or %16:%15 is == 0. We are now absolutely sure
        // we have at least 9 MSBits available to enter the initial estimation table
        L("5")
        A("add %15,%15")
        A("adc %16,%16")                  // %16:%15 = tidx = (nr <<= 1), we lose the top MSBit (always set to 1, %16 is the index into the inverse table)
        A("add r30,%16")                  // Only use top 8 bits
        A("adc r31,%13")                  // r31:r30 = inv_tab + (tidx)
        A("lpm %14, Z")                   // %14 = inv_tab[tidx]
        A("ldi %15, 1")                   // %15 = 1  %15:%14 = inv_tab[tidx] + 256

        // We must scale the approximation to the proper place
        A("clr %16")                      // %16 will always be 0 here
        A("subi %3,8")                    // idx == 8 ?
        A("breq 6f")                      // yes, no need to scale
        A("brcs 7f")                      // If C=1, means idx < 8, result was negative!

        // idx > 8, now %3 = idx - 8. We must perform a left shift. idx range:[1-8]
        A("sbrs %3,0")                    // shift by 1bit position?
        A("rjmp 8f")                      // No
        A("add %14,%14")
        A("adc %15,%15")                  // %15:16 <<= 1
        L("8")
        A("sbrs %3,1")                    // shift by 2bit position?
        A("rjmp 9f")                      // No
        A("add %14,%14")
        A("adc %15,%15")
        A("add %14,%14")
        A("adc %15,%15")                  // %15:16 <<= 1
        L("9")
        A("sbrs %3,2")                    // shift by 4bits position?
        A("rjmp 16f")                     // No
        A("swap %15")                     // Swap nibbles. lo nibble of %15 will always be 0
        A("swap %14")                     // Swap nibbles
        A("mov %12,%14")
        A("andi %12,0x0F")                // isolate low nibble
        A("andi %14,0xF0")                // and clear it
        A("or %15,%12")                   // %15:%16 <<= 4
        L("16")
        A("sbrs %3,3")                    // shift by 8bits position?
        A("rjmp 6f")                      // No, we are done
        A("mov %16,%15")
        A("mov %15,%14")
        A("clr %14")
        A("jmp 6f")

        // idx < 8, now %3 = idx - 8. Get the count of bits
        L("7")
        A("neg %3")                       // %3 = -idx = count of bits to move right. idx range:[1...8]
        A("sbrs %3,0")                    // shift by 1 bit position ?
        A("rjmp 10f")                     // No, skip it
        A("asr %15")                      // (bit7 is always 0 here)
        A("ror %14")
        L("10")
        A("sbrs %3,1")                    // shift by 2 bit position ?
        A("rjmp 11f")                     // No, skip it
        A("asr %15")                      // (bit7 is always 0 here)
        A("ror %14")
        A("asr %15")                      // (bit7 is always 0 here)
        A("ror %14")
        L("11")
        A("sbrs %3,2")                    // shift by 4 bit position ?
        A("rjmp 12f")                     // No, skip it
        A("swap %15")                     // Swap nibbles
        A("andi %14, 0xF0")               // Lose the lowest nibble
        A("swap %14")                     // Swap nibbles. Upper nibble is 0
        A("or %14,%15")                   // Pass nibble from upper byte
        A("andi %15, 0x0F")               // And get rid of that nibble
        L("12")
        A("sbrs %3,3")                    // shift by 8 bit position ?
        A("rjmp 6f")                      // No, skip it
        A("mov %14,%15")
        A("clr %15")
        L("6")                            // %16:%15:%14 = initial estimation of 0x1000000 / d

        // Now, we must refine the estimation present on %16:%15:%14 using 1 iteration
        // of Newton-Raphson. As it has a quadratic convergence, 1 iteration is enough
        // to get more than 18bits of precision (the initial table lookup gives 9 bits of
        // precision to start from). 18bits of precision is all what is needed here for result

        // %8:%7:%6 = d = interval
        // %16:%15:%14 = x = initial estimation of 0x1000000 / d
        // %13 = 0
        // %3:%2:%1:%0 = working accumulator

        // Compute 1<<25 - x*d. Result should never exceed 25 bits and should always be positive
        A("clr %0")
        A("clr %1")
        A("clr %2")
        A("ldi %3,2")                     // %3:%2:%1:%0 = 0x2000000
        A("mul %6,%14")                   // r1:r0 = LO(d) * LO(x)
        A("sub %0,r0")
        A("sbc %1,r1")
        A("sbc %2,%13")
        A("sbc %3,%13")                   // %3:%2:%1:%0 -= LO(d) * LO(x)
        A("mul %7,%14")                   // r1:r0 = MI(d) * LO(x)
        A("sub %1,r0")
        A("sbc %2,r1" )
        A("sbc %3,%13")                   // %3:%2:%1:%0 -= MI(d) * LO(x) << 8
        A("mul %8,%14")                   // r1:r0 = HI(d) * LO(x)
        A("sub %2,r0")
        A("sbc %3,r1")                    // %3:%2:%1:%0 -= MIL(d) * LO(x) << 16
        A("mul %6,%15")                   // r1:r0 = LO(d) * MI(x)
        A("sub %1,r0")
        A("sbc %2,r1")
        A("sbc %3,%13")                   // %3:%2:%1:%0 -= LO(d) * MI(x) << 8
        A("mul %7,%15")                   // r1:r0 = MI(d) * MI(x)
        A("sub %2,r0")
        A("sbc %3,r1")                    // %3:%2:%1:%0 -= MI(d) * MI(x) << 16
        A("mul %8,%15")                   // r1:r0 = HI(d) * MI(x)
        A("sub %3,r0")                    // %3:%2:%1:%0 -= MIL(d) * MI(x) << 24
        A("mul %6,%16")                   // r1:r0 = LO(d) * HI(x)
        A("sub %2,r0")
        A("sbc %3,r1")                    // %3:%2:%1:%0 -= LO(d) * HI(x) << 16
        A("mul %7,%16")                   // r1:r0 = MI(d) * HI(x)
        A("sub %3,r0")                    // %3:%2:%1:%0 -= MI(d) * HI(x) << 24
        // %3:%2:%1:%0 = (1<<25) - x*d     [169]

        // We need to multiply that result by x, and we are only interested in the top 24bits of that multiply

        // %16:%15:%14 = x = initial estimation of 0x1000000 / d
        // %3:%2:%1:%0 = (1<<25) - x*d = acc
        // %13 = 0

        // result = %11:%10:%9:%5:%4
        A("mul %14,%0")                   // r1:r0 = LO(x) * LO(acc)
        A("mov %4,r1")
        A("clr %5")
        A("clr %9")
        A("clr %10")
        A("clr %11")                      // %11:%10:%9:%5:%4 = LO(x) * LO(acc) >> 8
        A("mul %15,%0")                   // r1:r0 = MI(x) * LO(acc)
        A("add %4,r0")
        A("adc %5,r1")
        A("adc %9,%13")
        A("adc %10,%13")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 += MI(x) * LO(acc)
        A("mul %16,%0")                   // r1:r0 = HI(x) * LO(acc)
        A("add %5,r0")
        A("adc %9,r1")
        A("adc %10,%13")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 += MI(x) * LO(acc) << 8

        A("mul %14,%1")                   // r1:r0 = LO(x) * MIL(acc)
        A("add %4,r0")
        A("adc %5,r1")
        A("adc %9,%13")
        A("adc %10,%13")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 = LO(x) * MIL(acc)
        A("mul %15,%1")                   // r1:r0 = MI(x) * MIL(acc)
        A("add %5,r0")
        A("adc %9,r1")
        A("adc %10,%13")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 += MI(x) * MIL(acc) << 8
        A("mul %16,%1")                   // r1:r0 = HI(x) * MIL(acc)
        A("add %9,r0")
        A("adc %10,r1")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 += MI(x) * MIL(acc) << 16

        A("mul %14,%2")                   // r1:r0 = LO(x) * MIH(acc)
        A("add %5,r0")
        A("adc %9,r1")
        A("adc %10,%13")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 = LO(x) * MIH(acc) << 8
        A("mul %15,%2")                   // r1:r0 = MI(x) * MIH(acc)
        A("add %9,r0")
        A("adc %10,r1")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 += MI(x) * MIH(acc) << 16
        A("mul %16,%2")                   // r1:r0 = HI(x) * MIH(acc)
        A("add %10,r0")
        A("adc %11,r1")                   // %11:%10:%9:%5:%4 += MI(x) * MIH(acc) << 24

        A("mul %14,%3")                   // r1:r0 = LO(x) * HI(acc)
        A("add %9,r0")
        A("adc %10,r1")
        A("adc %11,%13")                  // %11:%10:%9:%5:%4 = LO(x) * HI(acc) << 16
        A("mul %15,%3")                   // r1:r0 = MI(x) * HI(acc)
        A("add %10,r0")
        A("adc %11,r1")                   // %11:%10:%9:%5:%4 += MI(x) * HI(acc) << 24
        A("mul %16,%3")                   // r1:r0 = HI(x) * HI(acc)
        A("add %11,r0")                   // %11:%10:%9:%5:%4 += MI(x) * HI(acc) << 32

        // At this point, %11:%10:%9 contains the new estimation of x.

        // Finally, we must correct the result. Estimate remainder as
        // (1<<24) - x*d
        // %11:%10:%9 = x
        // %8:%7:%6 = d = interval" "\n\t"
        A("ldi %3,1")
        A("clr %2")
        A("clr %1")
        A("clr %0")                       // %3:%2:%1:%0 = 0x1000000
        A("mul %6,%9")                    // r1:r0 = LO(d) * LO(x)
        A("sub %0,r0")
        A("sbc %1,r1")
        A("sbc %2,%13")
        A("sbc %3,%13")                   // %3:%2:%1:%0 -= LO(d) * LO(x)
        A("mul %7,%9")                    // r1:r0 = MI(d) * LO(x)
        A("sub %1,r0")
        A("sbc %2,r1")
        A("sbc %3,%13")                   // %3:%2:%1:%0 -= MI(d) * LO(x) << 8
        A("mul %8,%9")                    // r1:r0 = HI(d) * LO(x)
        A("sub %2,r0")
        A("sbc %3,r1")                    // %3:%2:%1:%0 -= MIL(d) * LO(x) << 16
        A("mul %6,%10")                   // r1:r0 = LO(d) * MI(x)
        A("sub %1,r0")
        A("sbc %2,r1")
        A("sbc %3,%13")                   // %3:%2:%1:%0 -= LO(d) * MI(x) << 8
        A("mul %7,%10")                   // r1:r0 = MI(d) * MI(x)
        A("sub %2,r0")
        A("sbc %3,r1")                    // %3:%2:%1:%0 -= MI(d) * MI(x) << 16
        A("mul %8,%10")                   // r1:r0 = HI(d) * MI(x)
        A("sub %3,r0")                    // %3:%2:%1:%0 -= MIL(d) * MI(x) << 24
        A("mul %6,%11")                   // r1:r0 = LO(d) * HI(x)
        A("sub %2,r0")
        A("sbc %3,r1")                    // %3:%2:%1:%0 -= LO(d) * HI(x) << 16
        A("mul %7,%11")                   // r1:r0 = MI(d) * HI(x)
        A("sub %3,r0")                    // %3:%2:%1:%0 -= MI(d) * HI(x) << 24
        // %3:%2:%1:%0 = r = (1<<24) - x*d
        // %8:%7:%6 = d = interval

        // Perform the final correction
        A("sub %0,%6")
        A("sbc %1,%7")
        A("sbc %2,%8")                    // r -= d
        A("brcs 14f")                     // if ( r >= d)

        // %11:%10:%9 = x
        A("ldi %3,1")
        A("add %9,%3")
        A("adc %10,%13")
        A("adc %11,%13")                  // x++
        L("14")

        // Estimation is done. %11:%10:%9 = x
        A("clr __zero_reg__")              // Make C runtime happy
        // [211 cycles total]
        : "=r" (r2),
          "=r" (r3),
          "=r" (r4),
          "=d" (r5),
          "=r" (r6),
          "=r" (r7),
          "+r" (r8),
          "+r" (r9),
          "+r" (r10),
          "=d" (r11),
          "=r" (r12),
          "=r" (r13),
          "=d" (r14),
          "=d" (r15),
          "=d" (r16),
          "=d" (r17),
          "=d" (r18),
          "+z" (ptab)
        :
        : "r0", "r1", "cc"
      );

      // Return the result
      return r11 | (uint16_t(r12) << 8) | (uint32_t(r13) << 16);
    }

  #else // ! __AVR__

    // All the other 32 CPUs can easily perform the inverse using hardware division,
    // so we don´t need to reduce precision or to use assembly language at all.

    // This routine, for all the other archs, returns 0x100000000 / d ~= 0xFFFFFFFF / d
    static FORCE_INLINE uint32_t get_period_inverse(const uint32_t d) {
      return d ? 0xFFFFFFFF / d : 0xFFFFFFFF;
    }

  #endif // ! __AVR__

#endif // ENABLED(BEZIER_JERK_CONTROL)

#if HAS_TEMP_HOTEND && ENABLED(AUTOTEMP)

  void Planner::getHighESpeed() {
    static float oldt = 0;

    if (!flag.autotemp_enabled) return;
    if (hotends[0]->deg_target() + 2 < autotemp_min) return; // probably temperature set to zero.

    float high = 0.0;
    for (uint8_t b = block_buffer_tail; b != block_buffer_head; b = next_block_index(b)) {
      block_t* block = &block_buffer[b];
      if (block->steps.x || block->steps.y || block->steps.z) {
        float se = (float)block->steps.e / block->step_event_count * SQRT(block->nominal_speed_sqr); // mm/sec;
        NOLESS(high, se);
      }
    }

    float t = autotemp_min + high * autotemp_factor;
    LIMIT(t, autotemp_min, autotemp_max);
    if (t < oldt) t = t * (1 - (AUTOTEMP_OLDWEIGHT)) + oldt * (AUTOTEMP_OLDWEIGHT);
    oldt = t;
    hotends[0]->set_target_temp(t);
  }

#endif // HAS_TEMP_HOTEND && ENABLED(AUTOTEMP)

/**
 * Manage Axis, paste pressure, etc.
 */
void Planner::check_axes_activity() {

  xyze_bool_t axis_active = { false };

  #if ENABLED(BARICUDA)
    #if HAS_HEATER_HE1
      uint8_t tail_valve_pressure;
    #endif
    #if HAS_HEATER_HE2
      uint8_t tail_e_to_p_pressure;
    #endif
  #endif

  if (has_blocks_queued()) {

    block_t* block;

    #if ENABLED(BARICUDA)
      block = &block_buffer[block_buffer_tail];
      #if HAS_HEATER_HE1
        tail_valve_pressure = block->valve_pressure;
      #endif
      #if HAS_HEATER_HE2
        tail_e_to_p_pressure = block->e_to_p_pressure;
      #endif
    #endif

    for (uint8_t b = block_buffer_tail; b != block_buffer_head; b = next_block_index(b)) {
      block = &block_buffer[b];
      LOOP_XYZE(i) if (block->steps[i]) axis_active[i] = true;
    }
  }
  else {
    #if ENABLED(BARICUDA)
      #if HAS_HEATER_HE1
        tail_valve_pressure = printer.baricuda_valve_pressure;;
      #endif
      #if HAS_HEATER_HE2
        tail_e_to_p_pressure = printer.baricuda_e_to_p_pressure;
      #endif
    #endif
  }

  #if DISABLE_X
    if (!axis_active.x) stepper.disable_X();
  #endif
  #if DISABLE_Y
    if (!axis_active.y) stepper.disable_Y();
  #endif
  #if DISABLE_Z
    if (!axis_active.z) stepper.disable_Z();
  #endif
  #if DISABLE_E
    if (!axis_active.e) stepper.disable_E();
  #endif

  #if HAS_TEMP_HOTEND && ENABLED(AUTOTEMP)
    getHighESpeed();
  #endif

  #if ENABLED(BARICUDA)
    #if HAS_HEATER_HE1
      analogWrite(HEATER_HE1_PIN, tail_valve_pressure);
    #endif
    #if HAS_HEATER_HE2
      analogWrite(HEATER_HE2_PIN, tail_e_to_p_pressure);
    #endif
  #endif
}

#if ENABLED(FWRETRACT)

  /**
   * rz, e - Cartesian positions in mm
   */
  void Planner::apply_retract(float &rz, float &e) {
    rz += fwretract.current_hop;
    e -= fwretract.current_retract[toolManager.extruder.active];
  }

  void Planner::unapply_retract(float &rz, float &e) {
    rz -= fwretract.current_hop;
    e += fwretract.current_retract[toolManager.extruder.active];
  }

#endif

#if HAS_POSITION_MODIFIERS

  void Planner::apply_modifiers(xyze_pos_t &pos, const bool leveling/*=HAS_PLANNER_LEVELING*/) {
    #if HAS_LEVELING
      if (leveling) bedlevel.apply_leveling(pos);
    #else
      UNUSED(leveling);
    #endif
    #if ENABLED(FWRETRACT)
      apply_retract(pos);
    #endif
  }

  void Planner::unapply_modifiers(xyze_pos_t &pos, const bool leveling/*=HAS_PLANNER_LEVELING*/) {
    #if ENABLED(FWRETRACT)
      unapply_retract(pos);
    #endif
    #if HAS_LEVELING
      if (leveling) bedlevel.unapply_leveling(pos);
    #else
      UNUSED(leveling);
    #endif
  }

#endif // HAS_POSITION_MODIFIERS

void Planner::quick_stop() {

  // Remove all the queued blocks. Note that this function is NOT
  // called from the Stepper ISR, so we must consider tail as readonly!
  // that is why we set head to tail - But there is a race condition that
  // must be handled: The tail could change between the read and the assignment
  // so this must be enclosed in a critical section
  const bool isr_enabled = STEPPER_ISR_ENABLED();
  if (isr_enabled) DISABLE_STEPPER_INTERRUPT();

  // Drop all queue entries
  block_buffer_nonbusy = block_buffer_planned = block_buffer_head = block_buffer_tail;

  // And restart the block delay for the first movement - As the queue was
  // forced to empty, there is no risk the ISR could touch this variable.
  delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;

  #if HAS_SPI_LCD
    // Clear the accumulated runtime
    clear_block_buffer_runtime();
  #endif

  // Make sure to drop any attempt of queuing moves for at least 1 seconds
  flag.clean_buffer = true;

  // Reenable Stepper ISR
  if (isr_enabled) ENABLE_STEPPER_INTERRUPT();

  // And stop the stepper ISR
  stepper.quick_stop();

}

void Planner::endstop_triggered(const AxisEnum axis) {
  // Record stepper position and discard the current block
  stepper.endstop_triggered(axis);
}

float Planner::triggered_position_mm(const AxisEnum axis) {
  return stepper.triggered_position(axis) * mechanics.steps_to_mm[axis];
}

/**
 * Get an axis position according to stepper position(s)
 * For CORE machines apply translation from ABC to XYZ.
 */
float Planner::get_axis_position_mm(const AxisEnum axis) {

  float axis_steps;

  #if IS_CORE
    // Requesting one of the "core" axes?
    if (axis == CORE_AXIS_1 || axis == CORE_AXIS_2) {

      // Protect the access to the position.
      const bool isr_enabled = stepper.suspend();

      const int32_t p1 = stepper.position(CORE_AXIS_1),
                    p2 = stepper.position(CORE_AXIS_2);

      if (isr_enabled) stepper.wake_up();

      axis_steps = (axis == CORE_AXIS_2 ? CORESIGN(p1 - p2) : p1 + p2) * 0.5f;

    }
    else
      axis_steps = stepper.position(axis);
  #else
    axis_steps = stepper.position(axis);
  #endif

  if (axis == E_AXIS)
    return axis_steps * extruders[toolManager.extruder.active]->steps_to_mm;
  else
    return axis_steps * mechanics.steps_to_mm[axis];

}

void Planner::synchronize() {
  while (has_blocks_queued() || flag.clean_buffer) {
    printer.idle();
    PRINTER_KEEPALIVE(InProcess);
  }
}

void Planner::finish_and_disable() {
  synchronize();
  stepper.disable_all();
}

/**
 * Planner::buffer_steps
 *
 * Add a new linear movement to the buffer (in terms of steps).
 *
 *  target        - target position in steps units
 *  target_float  - target position in direct (mm, degrees) units. optional
 *  fr_mm_s       - (target) speed of the move
 *  extruder      - target extruder
 *  millimeters   - the length of the movement, if known
 *
 * Returns true if movement was properly queued, false otherwise
 */
bool Planner::buffer_steps(const xyze_long_t &target
  #if HAS_POSITION_FLOAT
    , const xyze_float_t &target_float
  #endif
  #if HAS_DIST_MM_ARG
    , const xyze_float_t &cart_dist_mm
  #endif
  , feedrate_t fr_mm_s, const uint8_t extruder, const float &millimeters/*=0.0*/
) {

  // If we are cleaning, do not accept queuing of movements
  if (flag.clean_buffer) return false;

  // Wait for the next available block
  uint8_t next_buffer_head;
  block_t * const block = get_next_free_block(next_buffer_head);

  // Fill the block with the specified movement
  if (!fill_block(block, false, target
    #if HAS_POSITION_FLOAT
      , target_float
    #endif
    #if HAS_DIST_MM_ARG
      , cart_dist_mm
    #endif
    , fr_mm_s, extruder, millimeters
  )) {
    // Movement was not queued, probably because it was too short.
    // Simply accept that as movement queued and done
    return true;
  }

  // If this is the first added movement, reload the delay, otherwise, cancel it.
  if (block_buffer_head == block_buffer_tail) {
    //  If it was the first queued block, restart the 1st block delivery delay, to
    // give the planner an opportunity to queue more movements and plan them
    //  As there are no queued movements, the Stepper ISR will not touch this
    // variable, so there is no risk setting this here (but it MUST be done
    // before the following line!!)
    delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;
  }

  // Move buffer head
  block_buffer_head = next_buffer_head;

  // Recalculate and optimize trapezoidal speed profiles
  recalculate();

  // Movement successfully queued!
  return true;
}

/**
 * Planner::fill_block
 *
 * Fills a new linear movement in the block (in terms of steps).
 *
 *  target      - target position in steps units
 *  fr_mm_s     - (target) speed of the move
 *  extruder    - target extruder
 *
 * Returns true is movement is acceptable, false otherwise
 */
bool Planner::fill_block(block_t * const block, bool split_move,
  const abce_long_t &target
  #if HAS_POSITION_FLOAT
    , const xyze_float_t &target_float
  #endif
  #if HAS_DIST_MM_ARG
    , const xyze_float_t &cart_dist_mm
  #endif
  , feedrate_t fr_mm_s, const uint8_t extruder, const float &millimeters/*=0.0*/
) {

  const int32_t dx = target.x - position.x,
                dy = target.y - position.y,
                dz = target.z - position.z;
  int32_t       de = target.e - position.e;

  /* <-- add a slash to enable
    SERIAL_MV("  buffer_steps FR:", fr_mm_s);
    SERIAL_MV(" A:", target.a);
    SERIAL_MV(" (", dx);
    SERIAL_MV(" steps) B:", target.b);
    SERIAL_MV(" (", dy);
    SERIAL_MV(" steps) C:", target.c);
    SERIAL_MV(" (", dz);
    SERIAL_MV(" steps) E:", target.e);
    SERIAL_MV(" (", de);
    SERIAL_EM(" steps)");
  //*/

  #if ENABLED(PREVENT_COLD_EXTRUSION) || ENABLED(PREVENT_LENGTHY_EXTRUDE)
    if (de && printer.mode == PRINTER_MODE_FFF) {
      #if ENABLED(PREVENT_COLD_EXTRUSION)
        if (tempManager.tooColdToExtrude(extruder)) {
          position.e = target.e; // Behave as if the move really took place, but ignore E part
          #if HAS_POSITION_FLOAT
            position_float.e = target_float.e;
          #endif
          de = 0; // no difference
          SERIAL_LM(ER, STR_ERR_COLD_EXTRUDE_STOP);
        }
      #endif
      #if ENABLED(PREVENT_LENGTHY_EXTRUDE)
        if (ABS(de * extruders[extruder]->e_factor) > (int32_t)extruders[extruder]->data.axis_steps_per_mm * (EXTRUDE_MAXLENGTH)) {
          position.e = target.e; // Behave as if the move really took place, but ignore E part
          #if HAS_POSITION_FLOAT
            position_float.e = target_float.e;
          #endif
          de = 0; // no difference
          SERIAL_LM(ER, STR_ERR_LONG_EXTRUDE_STOP);
        }
      #endif // PREVENT_LENGTHY_EXTRUDE
    }
  #endif // PREVENT_COLD_EXTRUSION || PREVENT_LENGTHY_EXTRUDE

  #if CORE_IS_XY
    long da = dx + CORE_FACTOR * dy;
    long db = dx - CORE_FACTOR * dy;
  #elif CORE_IS_XZ
    long da = dx + CORE_FACTOR * dz;
    long dc = dx - CORE_FACTOR * dz;
  #elif CORE_IS_YZ
    long db = dy + CORE_FACTOR * dz;
    long dc = dy - CORE_FACTOR * dz;
  #endif

  // Compute direction bit for this block
  uint8_t dirb = 0;
  #if CORE_IS_XY
    if (dx < 0) SBI(dirb, X_HEAD);            // Save the real Nozzle (head) direction in X Axis
    if (dy < 0) SBI(dirb, Y_HEAD);            // ...and Y
    if (dz < 0) SBI(dirb, Z_AXIS);
    if (da < 0) SBI(dirb, A_AXIS);            // Motor A direction
    if (CORESIGN(db) < 0) SBI(dirb, B_AXIS);  // Motor B direction
  #elif CORE_IS_XZ
    if (dx < 0) SBI(dirb, X_HEAD);            // Save the real Nozzle (head) direction in X Axis
    if (dy < 0) SBI(dirb, Y_AXIS);
    if (dz < 0) SBI(dirb, Z_HEAD);            // ...and Z
    if (da < 0) SBI(dirb, A_AXIS);            // Motor A direction
    if (CORESIGN(dc) < 0) SBI(dirb, C_AXIS);  // Motor C direction
  #elif CORE_IS_YZ
    if (dx < 0) SBI(dirb, X_AXIS);
    if (dy < 0) SBI(dirb, Y_HEAD);            // Save the real Nozzle (head) direction in Y Axis
    if (dz < 0) SBI(dirb, Z_HEAD);            // ...and Z
    if (db < 0) SBI(dirb, B_AXIS);            // Motor B direction
    if (CORESIGN(dc) < 0) SBI(dirb, C_AXIS);  // Motor C direction
  #else
    if (dx < 0) SBI(dirb, X_AXIS);
    if (dy < 0) SBI(dirb, Y_AXIS);
    if (dz < 0) SBI(dirb, Z_AXIS);
  #endif
  if (de < 0) SBI(dirb, E_AXIS);

  const float esteps_float = de * extruders[extruder]->e_factor;
  const uint32_t esteps = ABS(esteps_float) + 0.5;

  // Clear all flags, including the "busy" bit
  block->flag = 0x00;

  // Set direction bits
  block->direction_bits = dirb;

  // Number of steps for each axis
  // See http://www.corexy.com/theory.html
  #if CORE_IS_XY
    // corexy planning
    block->steps.set(ABS(da), ABS(db), ABS(dz));
  #elif CORE_IS_XZ
    // corexz planning
    block->steps.set(ABS(da), ABS(dy), ABS(dc));
  #elif CORE_IS_YZ
    // coreyz planning
    block->steps.set(ABS(dx), ABS(db), ABS(dc));
  #else
    // default non-h-bot planning
    block->steps.set(ABS(dx), ABS(dy), ABS(dz));
  #endif

  /**
   * This part of the code calculates the total length of the movement.
   * For cartesian bots, the X_AXIS is the real X movement and same for Y_AXIS.
   * But for corexy bots, that is not true. The "X_AXIS" and "Y_AXIS" motors (that should be named to A_AXIS
   * and B_AXIS) cannot be used for X and Y length, because A=X+Y and B=X-Y.
   * So we need to create other 2 "AXIS", named X_HEAD and Y_HEAD, meaning the real displacement of the Head.
   * Having the real displacement of the head, we can calculate the total movement length and apply the desired speed.
   */
  struct DistanceMM : abce_float_t {
    #if IS_CORE
      xyz_pos_t head;
    #endif
  } steps_dist_mm;
  #if IS_CORE
    #if CORE_IS_XY
      steps_dist_mm.head.x  = dx * mechanics.steps_to_mm.a;
      steps_dist_mm.head.y  = dy * mechanics.steps_to_mm.b;
      steps_dist_mm.z       = dz * mechanics.steps_to_mm.z;
      steps_dist_mm.a       = da * mechanics.steps_to_mm.a;
      steps_dist_mm.b       = CORESIGN(db) * mechanics.steps_to_mm.b;
    #elif CORE_IS_XZ
      steps_dist_mm.head.x  = dx * mechanics.steps_to_mm.a;
      steps_dist_mm.y       = dy * mechanics.steps_to_mm.y;
      steps_dist_mm.head.z  = dz * mechanics.steps_to_mm.c;
      steps_dist_mm.a       = da * mechanics.steps_to_mm.a;
      steps_dist_mm.c       = CORESIGN(dc) * mechanics.steps_to_mm.c;
    #elif CORE_IS_YZ
      steps_dist_mm.x       = dx * mechanics.steps_to_mm.x;
      steps_dist_mm.head.y  = dy * mechanics.steps_to_mm.b;
      steps_dist_mm.head.z  = dz * mechanics.steps_to_mm.c;
      steps_dist_mm.b       = db * mechanics.steps_to_mm.b;
      steps_dist_mm.c       = CORESIGN(dc) * mechanics.steps_to_mm.c;
    #endif
  #else
    steps_dist_mm.x         = dx * mechanics.steps_to_mm.x;
    steps_dist_mm.y         = dy * mechanics.steps_to_mm.y;
    steps_dist_mm.z         = dz * mechanics.steps_to_mm.z;
  #endif

  steps_dist_mm.e = esteps_float * extruders[extruder]->steps_to_mm;

  if (block->steps.x < MIN_STEPS_PER_SEGMENT && block->steps.y < MIN_STEPS_PER_SEGMENT && block->steps.z < MIN_STEPS_PER_SEGMENT) {
    block->millimeters = ABS(steps_dist_mm.e);
  }
  else {
    if (millimeters)
      block->millimeters = millimeters;
    else
      block->millimeters = SQRT(
        #if CORE_IS_XY
          sq(steps_dist_mm.head.x) + sq(steps_dist_mm.head.y) + sq(steps_dist_mm.z)
        #elif CORE_IS_XZ
          sq(steps_dist_mm.head.x) + sq(steps_dist_mm.y) + sq(steps_dist_mm.head.z)
        #elif CORE_IS_YZ
          sq(steps_dist_mm.x) + sq(steps_dist_mm.head.y) + sq(steps_dist_mm.head.z)
        #else
          sq(steps_dist_mm.x) + sq(steps_dist_mm.y) + sq(steps_dist_mm.z)
        #endif
      );

    #if ENABLED(HYSTERESIS_FEATURE)
      hysteresis.add_correction_step(block);
    #endif

  }

  block->steps.e = esteps;
  block->step_event_count = MAX(block->steps.x, block->steps.y, block->steps.z, esteps);

  // Bail if this is a zero-length block
  if (printer.mode == PRINTER_MODE_FFF && block->step_event_count < MIN_STEPS_PER_SEGMENT) return false;

  // For a mixing extruder, get a magnified step_event_count for each
  #if ENABLED(COLOR_MIXING_EXTRUDER)
    mixer.populate_block(block->b_color);
  #endif

  #if ENABLED(BARICUDA)
    block->valve_pressure   = printer.baricuda_valve_pressure;
    block->e_to_p_pressure  = printer.baricuda_e_to_p_pressure;
  #endif

  #if EXTRUDERS > 1
    block->active_extruder = extruder;
  #endif

  #if HAS_POWER_SWITCH
    if (block->steps.x || block->steps.y || block->steps.z)
      powerManager.power_on();
  #endif

  // Enable active axes
  #if CORE_IS_XY
    if (block->steps.a || block->steps.b) {
      stepper.enable_X();
      stepper.enable_Y();
    }
    #if DISABLED(Z_LATE_ENABLE)
      if (block->steps.z) stepper.enable_Z();
    #endif
  #elif CORE_IS_XZ
    if (block->steps.a || block->steps.c) {
      stepper.enable_X();
      stepper.enable_Z();
    }
    if (block->steps.y) stepper.enable_Y();
  #elif CORE_IS_YZ
    if (block->steps.b || block->steps.c) {
      stepper.enable_Y();
      stepper.enable_Z();
    }
    if (block->steps.x) stepper.enable_X();
  #else
    if (block->steps.x) stepper.enable_X();
    if (block->steps.y) stepper.enable_Y();
    #if DISABLED(Z_LATE_ENABLE)
      if (block->steps.z) stepper.enable_Z();
    #endif
  #endif

  // Enable extruder(s)
  if (esteps) {

    #if HAS_POWER_SWITCH
      powerManager.power_on();
    #endif

    #if !HAS_MKMULTI_TOOLS && !ENABLED(DONDOLO_SINGLE_MOTOR)

      #if ENABLED(DISABLE_INACTIVE_EXTRUDER) // Enable only the selected extruder

        LOOP_EXTRUDER() {
          if (g_uc_extruder_last_move[e] > 0) g_uc_extruder_last_move[e]--;
          if (e == extruder) {
            stepper.enable_E(e);
            g_uc_extruder_last_move[e] = (BLOCK_BUFFER_SIZE) * 2;
          }
          else
            if (!g_uc_extruder_last_move[e]) stepper.disable_E(e);
          #if ENABLED(DUAL_X_CARRIAGE)
            if (e == 0 && mechanics.extruder_duplication_enabled) {
              stepper.enable_E(1);
              g_uc_extruder_last_move[1] = (BLOCK_BUFFER_SIZE) * 2;
            }
          #endif
        }

      #else // enable all
        stepper.enable_E();
      #endif
    #elif ENABLED(MKR6)
      switch (extruder) {
        case 0:
        case 1:
        case 2:
          stepper.enable_E(0);
          break;
        case 3:
        case 4:
        case 5:
          stepper.enable_E(1);
          break;
      }
    #elif ENABLED(MKR12)
      switch (extruder) {
        case 0:
        case 1:
        case 2:
          stepper.enable_E(0);
          break;
        case 3:
        case 4:
        case 5:
          stepper.enable_E(1);
          break;
        case 6:
        case 7:
        case 8:
          stepper.enable_E(2);
          break;
        case 9:
        case 10:
        case 11:
          stepper.enable_E(3);
          break;
      }
    #elif ENABLED(MKR4)
      switch (extruder) {
        case 0:
          stepper.enable_E(0);
        break;
        case 1:
          stepper.enable_E(1);
        break;
        case 2:
          stepper.enable_E(0);
        break;
        case 3:
          stepper.enable_E(1);
        break;
      }
    #else
      stepper.enable_E(0);
    #endif
  }

  if (esteps)
    NOLESS(fr_mm_s, mechanics.data.min_feedrate_mm_s);
  else
    NOLESS(fr_mm_s, mechanics.data.min_travel_feedrate_mm_s);

  #if ENABLED(LASER)

    block->laser_intensity  = laser.intensity;
    block->laser_duration   = laser.duration;
    block->laser_status     = laser.status;
    block->laser_mode       = laser.mode;

    // When operating in PULSED or RASTER modes, laser pulsing must operate in sync with movement.
    // Calculate steps between laser firings (steps_l) and consider that when determining largest
    // interval between steps for X, Y, Z, E, L to feed to the motion control code.
    if (laser.mode == RASTER || laser.mode == PULSED) {
      block->steps_l = ABS(block->millimeters * laser.ppm);
      #if ENABLED(LASER_RASTER)
        for (uint8_t i = 0; i < LASER_MAX_RASTER_LINE; i++) {
          // Scale the image intensity based on the raster power.
          // 100% power on a pixel basis is 255, convert back to 255 = 100.
          #if ENABLED(LASER_REMAP_INTENSITY)
            const int NewRange = (laser.rasterlaserpower * 255.0 / 100.0 - LASER_REMAP_INTENSITY);
            float     NewValue = (float)(((((float)laser.raster_data[i] - 0) * NewRange) / 255.0) + LASER_REMAP_INTENSITY);
          #else
            const int NewRange = (laser.rasterlaserpower * 255.0 / 100.0);
            float     NewValue = (float)(((((float)laser.raster_data[i] - 0) * NewRange) / 255.0));
          #endif

          #if ENABLED(LASER_REMAP_INTENSITY)
            // If less than 7%, turn off the laser tube.
            if (NewValue <= LASER_REMAP_INTENSITY) NewValue = 0;
          #endif

          block->laser_raster_data[i] = NewValue;
        }
      #endif
    }
    else
      block->steps_l = 0;

    block->step_event_count = MAX(block->step_event_count, block->steps_l);

    if (laser.diagnostics && block->laser_status == LASER_ON)
      SERIAL_LM(ECHO, "Laser firing enabled");

  #endif // LASER

  const float inverse_millimeters = 1.0f / block->millimeters;  // Inverse millimeters to remove multiple divides

  // Calculate inverse time for this move. No divide by zero due to previous checks.
  // Example: At 120mm/s a 60mm move takes 0.5s. So this will give 2.0.
  float inverse_secs = fr_mm_s * inverse_millimeters;

  // Get the number of non busy movements in queue (non busy means that they can be altered)
  const uint8_t moves_queued = nonbusy_moves_planned();

  // Slow down when the buffer starts to empty, rather than wait at the corner for a buffer refill
  #if ENABLED(SLOWDOWN) || HAS_SPI_LCD || ENABLED(XY_FREQUENCY_LIMIT)
    // Segment time im micro seconds
    int32_t segment_time_us = LROUND(1000000.0f / inverse_secs);
  #endif

  #if ENABLED(SLOWDOWN)
    if (WITHIN(moves_queued, 2, (BLOCK_BUFFER_SIZE) / 2 - 1)) {
      if (segment_time_us < mechanics.data.min_segment_time_us) {
        // buffer is draining, add extra time.  The amount of time added increases if the buffer is still emptied more.
        const uint32_t nst = segment_time_us + LROUND(2 * (mechanics.data.min_segment_time_us - segment_time_us) / moves_queued);
        inverse_secs = 1000000.0f / nst;
        #if ENABLED(XY_FREQUENCY_LIMIT) || HAS_SPI_LCD
          segment_time_us = nst;
        #endif
      }
    }
  #endif

  #if HAS_SPI_LCD
    // Disable stepper ISR
    const bool isr_enabled = STEPPER_ISR_ENABLED();
    if (isr_enabled) DISABLE_STEPPER_INTERRUPT();

    block_buffer_runtime_us += segment_time_us;
    block->segment_time_us = segment_time_us;

    // Reenable Stepper ISR
    if (isr_enabled) ENABLE_STEPPER_INTERRUPT();
  #endif

  block->nominal_speed_sqr  = sq(block->millimeters * inverse_secs);        //   (mm/sec)^2 Always > 0
  block->nominal_rate       = CEIL(block->step_event_count * inverse_secs); // (step/sec)   Always > 0

  #if ENABLED(FILAMENT_WIDTH_SENSOR)
    static float filwidth_e_count = 0, filwidth_delay_dist = 0;

    // FMM update ring buffer used for delay with filament measurements
    if (extruder == FILAMENT_SENSOR_EXTRUDER_NUM && filwidth_delay_index[1] >= 0) {  // only for extruder with filament sensor and if ring buffer is initialized

      constexpr int MMD_CM = MAX_MEASUREMENT_DELAY + 1, MMD_MM = MMD_CM * 10;

      // increment counters with next move in e axis
      filwidth_e_count += steps_dist_mm.e;
      filwidth_delay_dist += steps_dist_mm.e;

      // Only get new measurements on forward E movement
      if (!UNEAR_ZERO(filwidth_e_count)) {

        // Loop the delay distance counter (modulus by the mm length)
        while (filwidth_delay_dist >= MMD_MM) filwidth_delay_dist -= MMD_MM;

        // Convert into an index into the measurement array
        filwidth_delay_index[0] = int8_t(filwidth_delay_dist * 0.1);

        // If the index has changed (must have gone forward)...
        if (filwidth_delay_index[0] != filwidth_delay_index[1]) {
          filwidth_e_count = 0; // Reset the E movement counter
          const int8_t meas_sample = tempManager.widthFil_to_size_ratio();
          do {
            filwidth_delay_index[1] = (filwidth_delay_index[1] + 1) % MMD_CM; // The next unused slot
            measurement_delay[filwidth_delay_index[1]] = meas_sample;         // Store the measurement
          } while (filwidth_delay_index[0] != filwidth_delay_index[1]);       // More slots to fill?
        }
      }
    }
  #endif

  // Calculate and limit speed in mm/sec for each axis
  xyze_float_t current_speed;
  float speed_factor = 1.0f; // factor < 1 decreases speed
  LOOP_XYZE(i) {
    current_speed[i]          = steps_dist_mm[i] * inverse_secs;
    const feedrate_t      cs  = ABS(current_speed[i]),
                      max_fr  = (i == E_AXIS) ? extruders[extruder]->data.max_feedrate_mm_s : mechanics.data.max_feedrate_mm_s[i];
    if (cs > max_fr) NOMORE(speed_factor, max_fr / cs);
  }

  // Max segment time in µs.
  #if HAS_XY_FREQUENCY_LIMIT

    static uint8_t old_direction_bits; // = 0

    if (mechanics.data.xy_freq_limit_hz) {
      // Check and limit the xy direction change frequency
      const uint8_t direction_change_bits = block->direction_bits ^ old_direction_bits;
      old_direction_bits = block->direction_bits;
      segment_time_us = LROUND((float)segment_time_us / speed_factor);

      static int32_t xs0, xs1, xs2, ys0, ys1, ys2;
      if (segment_time_us > mechanics.xy_freq_min_interval_us)
        xs2 = xs1 = ys2 = ys1 = mechanics.xy_freq_min_interval_us;
      else {
        xs2 = xs1; xs1 = xs0;
        ys2 = ys1; ys1 = ys0;
      }
      xs0 = TEST(direction_change_bits, X_AXIS) ? segment_time_us : mechanics.xy_freq_min_interval_us;
      ys0 = TEST(direction_change_bits, Y_AXIS) ? segment_time_us : mechanics.xy_freq_min_interval_us;

      if (segment_time_us < mechanics.xy_freq_min_interval_us) {
        const int32_t least_xy_segment_time = MIN(MAX(xs0, xs1, xs2), MAX(ys0, ys1, ys2));
        if (least_xy_segment_time < mechanics.xy_freq_min_interval_us) {
          float freq_xy_feedrate = (speed_factor * least_xy_segment_time) / mechanics.xy_freq_min_interval_us;
          NOLESS(freq_xy_feedrate, mechanics.data.xy_freq_min_speed_factor);
          NOMORE(speed_factor, freq_xy_feedrate);
        }
      }
    }
  #endif // HAS_XY_FREQUENCY_LIMIT

  // Correct the speed
  if (speed_factor < 1.0f) {
    current_speed *= speed_factor;
    block->nominal_rate *= speed_factor;
    block->nominal_speed_sqr = block->nominal_speed_sqr * sq(speed_factor);
  }

  // Compute and limit the acceleration rate for the trapezoid generator.
  const float steps_per_mm = block->step_event_count * inverse_millimeters;
  uint32_t accel;
  if (!block->steps.x && !block->steps.y && !block->steps.z) {
    // convert to: acceleration steps/sec^2
    accel = CEIL(extruders[extruder]->data.retract_acceleration * steps_per_mm);
    #if ENABLED(LIN_ADVANCE)
      block->use_advance_lead = false;
    #endif
  }
  else {

    // Start with print or travel acceleration
    accel = CEIL((esteps ? mechanics.data.acceleration : mechanics.data.travel_acceleration) * steps_per_mm);

    #if ENABLED(LIN_ADVANCE)

      /**
       *
       * Use LIN_ADVANCE for blocks if all these are true:
       *
       * esteps             : This is a print move, because we checked for A, B, C steps before.
       *
       * extruder advance K : There is an advance factor set.
       *
       * de > 0             : Extruder is running forward (e.g., for "Wipe while retracting" (Slic3r) or "Combing" (Cura) moves)
       */
      block->use_advance_lead =  esteps
                              && extruders[extruder]->data.advance_K
                              && de > 0;

      if (block->use_advance_lead) {
        block->e_D_ratio = (target_float.e - position_float.e) /
          #if IS_KINEMATIC
            block->millimeters
          #else
            SQRT(sq(target_float.x - position_float.x)
               + sq(target_float.y - position_float.y)
               + sq(target_float.z - position_float.z))
          #endif
        ;

        // Check for unusual high e_D ratio to detect if a retract move was combined with the last print move due to min. steps per segment. Never execute this with advance!
        // This assumes no one will use a retract length of 0mm < retr_length < ~0.2mm and no one will print 100mm wide lines using 3mm filament or 35mm wide lines using 1.75mm filament.
        if (block->e_D_ratio > 3.0f)
          block->use_advance_lead = false;
        else {
          const uint32_t max_accel_steps_per_s2 = extruders[extruder]->data.max_jerk / (extruders[extruder]->data.advance_K * block->e_D_ratio) * steps_per_mm;
          if (printer.debugFeature() && accel > max_accel_steps_per_s2) DEBUG_EM("Acceleration limited.");
          NOMORE(accel, max_accel_steps_per_s2);
        }
      }
    #endif

    // Limit acceleration per axis
    if (block->step_event_count <= cutoff_long) {
      LOOP_XYZ(axis) {
        if (block->steps[axis] && mechanics.max_acceleration_steps_per_s2[axis] < accel) {
          const uint32_t comp = mechanics.max_acceleration_steps_per_s2[axis] * block->step_event_count;
          if (accel * block->steps[axis] > comp) accel = comp / block->steps[axis];
        }
      }
      if (block->steps.e && extruders[extruder]->max_acceleration_steps_per_s2 < accel) {
        const uint32_t comp = extruders[extruder]->max_acceleration_steps_per_s2 * block->step_event_count;
        if (accel * block->steps.e > comp) accel = comp / block->steps.e;
      }
    }
    else {
      LOOP_XYZ(axis) {
        if (block->steps[axis] && mechanics.max_acceleration_steps_per_s2[axis] < accel) {
          const float comp = (float)mechanics.max_acceleration_steps_per_s2[axis] * (float)block->step_event_count;
          if ((float)accel * (float)block->steps[axis] > comp) accel = comp / (float)block->steps[axis];
        }
      }
      if (block->steps.e && extruders[extruder]->max_acceleration_steps_per_s2 < accel) {
        const float comp = (float)extruders[extruder]->max_acceleration_steps_per_s2 * (float)block->step_event_count;
        if ((float)accel * (float)block->steps.e > comp) accel = comp / (float)block->steps.e;
      }
    }
  }
  block->acceleration_steps_per_s2 = accel;
  block->acceleration = accel / steps_per_mm;
  #if DISABLED(BEZIER_JERK_CONTROL)
    block->acceleration_rate = (uint32_t)(accel * (4096.0f * 4096.0f / (STEPPER_TIMER_RATE)));
  #endif
  #if ENABLED(LIN_ADVANCE)
    if (block->use_advance_lead) {
      block->advance_speed = (STEPPER_TIMER_RATE) / (extruders[extruder]->data.advance_K * block->e_D_ratio * block->acceleration * extruders[extruder]->data.axis_steps_per_mm);
      if (printer.debugFeature()) {
        if (extruders[extruder]->data.advance_K * block->e_D_ratio * block->acceleration * 2 < SQRT(block->nominal_speed_sqr) * block->e_D_ratio)
          DEBUG_EM("More than 2 steps per eISR loop executed.");
        if (block->advance_speed < 200)
          DEBUG_EM("eISR running at > 10kHz.");
      }
    }
  #endif

  float vmax_junction_sqr; // Initial limit on the segment entry velocity (mm/s)^2

  #if HAS_JUNCTION_DEVIATION

    // Unit vector of previous path line segment
    static xyze_float_t previous_unit_vec;

    #if HAS_DIST_MM_ARG
      xyze_float_t unit_vec = cart_dist_mm;
    #else
      xyze_float_t unit_vec = { steps_dist_mm.x, steps_dist_mm.y, steps_dist_mm.z, steps_dist_mm.e };
    #endif

    #if IS_CORE
      /**
       * On CoreXY the length of the vector [A,B] is SQRT(2) times the length of the head movement vector [X,Y].
       * So taking Z and E into account, we cannot scale to a unit vector with "inverse_millimeters".
       * => normalize the complete junction vector
       */
      normalize_junction_vector(unit_vec);
    #else
      if (esteps > 0)
        normalize_junction_vector(unit_vec);  // Normalize with XYZE components
      else
        unit_vec *= inverse_millimeters;      // Use pre-calculated (1 / SQRT(x^2 + y^2 + z^2))
    #endif

    // Skip first block or when previous_nominal_speed is used as a flag for homing and offset cycles.
    if (moves_queued && !UNEAR_ZERO(previous_nominal_speed_sqr)) {
      // Compute cosine of angle between previous and current path. (prev_unit_vec is negative)
      // NOTE: Max junction velocity is computed without sin() or acos() by trig half angle identity.
      float junction_cos_theta =  (-previous_unit_vec.x * unit_vec.x)
                                 +(-previous_unit_vec.y * unit_vec.y)
                                 +(-previous_unit_vec.z * unit_vec.z)
                                 +(-previous_unit_vec.e * unit_vec.e);

      // NOTE: Computed without any expensive trig, sin() or acos(), by trig half angle identity of cos(theta).
      if (junction_cos_theta > 0.999999f) {
        // For a 0 degree acute junction, just set minimum junction speed.
        vmax_junction_sqr = sq(float(MINIMUM_PLANNER_SPEED));
      }
      else {
        NOLESS(junction_cos_theta, -0.999999f);  // Check for numerical round-off to avoid divide by zero.

        xyze_float_t junction_unit_vec = unit_vec - previous_unit_vec;
        normalize_junction_vector(junction_unit_vec);

        const float junction_acceleration = limit_value_by_axis_maximum(block->acceleration, junction_unit_vec),
                    sin_theta_d2 = SQRT(0.5f * (1.0f - junction_cos_theta)); // Trig half angle identity. Always positive.

        vmax_junction_sqr = (mechanics.data.junction_deviation_mm * junction_acceleration * sin_theta_d2) / (1.0f - sin_theta_d2);

        // For small moves with >135° junction (octagon) find speed for approximate arc
        if (block->millimeters < 1 && junction_cos_theta < -0.7071067812f) {

          const float neg = junction_cos_theta < 0 ? -1 : 1,
                      t   = neg * junction_cos_theta;

          #if ENABLED(JUNCTION_DEVIATION_USE_TABLE)

            // Fast acos approximation (max. error +-0.01 rads)
            // Based on LUT table and linear interpolation
            static constexpr int16_t  jd_lut_count = 15;
            static constexpr uint16_t jd_lut_tll   = 1 << jd_lut_count;
            static constexpr int16_t  jd_lut_tll0  = __builtin_clz(jd_lut_tll) + 1;
            static constexpr float    jd_lut_k[jd_lut_count] PROGMEM = {
              -1.03146219f, -1.30760407f, -1.75205469f, -2.41705418f, -3.37768555f,
              -4.74888229f, -6.69648552f, -9.45659828f, -13.3640289f, -18.8927879f,
              -26.7136307f, -37.7754059f, -53.4200745f, -75.5457306f,   0.0f
            };
            static constexpr float    jd_lut_b[jd_lut_count] PROGMEM = {
              1.57079637f, 1.70886743f, 2.04220533f, 2.62408018f, 3.52467203f,
              4.85301876f, 6.77019119f, 9.50873947f, 13.4009094f, 18.9188652f,
              26.7320709f, 37.7884521f, 53.4292908f, 75.5522461f,  0.0f
            };

            const int16_t idx = (t == 0.0f) ? 0 : __builtin_clz(int16_t((1.0f - t) * jd_lut_tll)) - jd_lut_tll0;

            float junction_theta = t * pgm_read_float(&jd_lut_k[idx]) + pgm_read_float(&jd_lut_b[idx]);
            if (neg > 0) junction_theta = RADIANS(180) - junction_theta;

          #else

            // Fast acos(-t) approximation (max. error +-0.033rad = 1.89°)
            // Based on MinMax polynomial published by W. Randolph Franklin, see
            // https://wrf.ecse.rpi.edu/Research/Short_Notes/arcsin/onlyelem.html
            //  acos( t) = pi / 2 - asin(x)
            //  acos(-t) = pi - acos(t) ... pi / 2 + asin(x)

            const float asinx =       0.032843707f
                              + t * (-1.451838349f
                              + t * ( 29.66153956f
                              + t * (-131.1123477f
                              + t * ( 262.8130562f
                              + t * (-242.7199627f
                              + t * ( 84.31466202f ) ))))),
                        junction_theta = RADIANS(90) + neg * asinx; // acos(-t)

          #endif

          // NOTE: MinMax acos approximation and thereby also junction_theta top out at pi-0.033, which avoids division by 0
          const float limit_sqr = block->millimeters / (RADIANS(180) - junction_theta) * junction_acceleration;
          NOMORE(vmax_junction_sqr, limit_sqr);
        }
      }

      // Get the lowest speed
      vmax_junction_sqr = MIN(vmax_junction_sqr, block->nominal_speed_sqr, previous_nominal_speed_sqr);
    }
    else // Init entry speed to zero. Assume it starts from rest. Planner will correct this later.
      vmax_junction_sqr = 0;

    previous_unit_vec = unit_vec;

  #endif // HAS_JUNCTION_DEVIATION

  #if HAS_CLASSIC_JERK

    const float nominal_speed = SQRT(block->nominal_speed_sqr);

    // Exit speed limited by a jerk to full halt of a previous last segment
    static float previous_safe_speed;

    // Start with a safe speed (from which the machine may halt to stop immediately).
    float safe_speed = nominal_speed;

    uint8_t limited = 0;
    #if HAS_LINEAR_E_JERK
      LOOP_XYZ(i)
    #else
      LOOP_XYZE(i)
    #endif
    {
      const float jerk = ABS(current_speed[i]),
                  maxj = (i == E_AXIS) ? extruders[extruder]->data.max_jerk : mechanics.data.max_jerk[i];

      if (jerk > maxj) {
        if (limited) {
          const float mjerk = maxj * nominal_speed;
          if (jerk * safe_speed > mjerk) safe_speed = mjerk / jerk;
        }
        else {
          safe_speed *= maxj / jerk;
          ++limited;
        }
      }
    }

    float vmax_junction;
    if (moves_queued && !UNEAR_ZERO(previous_nominal_speed_sqr)) {
      // Estimate a maximum velocity allowed at a joint of two successive segments.
      // If this maximum velocity allowed is lower than the minimum of the entry / exit safe velocities,
      // then the machine is not coasting anymore and the safe entry / exit velocities shall be used.

      // Factor to multiply the previous / current nominal velocities to get componentwise limited velocities.
      float v_factor = 1;
      limited = 0;

      // The junction velocity will be shared between successive segments. Limit the junction velocity to their minimum.
      // Pick the smaller of the nominal speeds. Higher speed shall not be achieved at the junction during coasting.
      const float previous_nominal_speed = SQRT(previous_nominal_speed_sqr);
      vmax_junction = MIN(nominal_speed, previous_nominal_speed);

      // Now limit the jerk in all axes.
      const float smaller_speed_factor = vmax_junction / previous_nominal_speed;
      #if HAS_LINEAR_E_JERK
        LOOP_XYZ(axis)
      #else
        LOOP_XYZE(axis)
      #endif
      {
        // Limit an axis. We have to differentiate: coasting, reversal of an axis, full stop.
        float v_exit = previous_speed[axis] * smaller_speed_factor,
              v_entry = current_speed[axis];
        if (limited) {
          v_exit  *= v_factor;
          v_entry *= v_factor;
        }

        // Calculate jerk depending on whether the axis is coasting in the same direction or reversing.
        const float jerk = (v_exit > v_entry)
            ? //                                  coasting             axis reversal
              ( (v_entry > 0 || v_exit < 0) ? (v_exit - v_entry) : MAX(v_exit, -v_entry) )
            : // v_exit <= v_entry                coasting             axis reversal
              ( (v_entry < 0 || v_exit > 0) ? (v_entry - v_exit) : MAX(-v_exit, v_entry) );

        const float maxj = (axis == E_AXIS) ? extruders[extruder]->data.max_jerk : mechanics.data.max_jerk[axis];
        if (jerk > maxj) {
          v_factor *= maxj / jerk;
          ++limited;
        }
      }
      if (limited) vmax_junction *= v_factor;
      // Now the transition velocity is known, which maximizes the shared exit / entry velocity while
      // respecting the jerk factors, it may be possible, that applying separate safe exit / entry velocities will achieve faster prints.
      const float vmax_junction_threshold = vmax_junction * 0.99f;
      if (previous_safe_speed > vmax_junction_threshold && safe_speed > vmax_junction_threshold)
        vmax_junction = safe_speed;
    }
    else
      vmax_junction = safe_speed;

    previous_safe_speed = safe_speed;

    #if HAS_JUNCTION_DEVIATION
      NOMORE(vmax_junction_sqr, sq(vmax_junction)); // Throttle down to max speed
    #else
      vmax_junction_sqr = sq(vmax_junction);        // Go up or down to the new speed
    #endif

  #endif // Classic Jerk Limiting

  // Max entry speed of this block equals the max exit speed of the previous block.
  block->max_entry_speed_sqr = vmax_junction_sqr;

  // Initialize block entry speed. Compute based on deceleration to user-defined MINIMUM_PLANNER_SPEED.
  const float v_allowable_sqr = max_allowable_speed_sqr(-block->acceleration, sq(MINIMUM_PLANNER_SPEED), block->millimeters);

  // If we are trying to add a split block, start with the
  // max. allowed speed to avoid an interrupted first move.
  block->entry_speed_sqr = !split_move ? sq(float(MINIMUM_PLANNER_SPEED)) : MIN(vmax_junction_sqr, v_allowable_sqr);

  // Initialize planner efficiency flags
  // Set flag if block will always reach maximum junction speed regardless of entry/exit speeds.
  // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
  // the current block and next block junction speeds are guaranteed to always be at their maximum
  // junction speeds in deceleration and acceleration, respectively. This is due to how the current
  // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
  // the reverse and forward planners, the corresponding block junction speed will always be at the
  // the maximum junction speed and may always be ignored for any speed reduction checks.
  block->flag |= block->nominal_speed_sqr <= v_allowable_sqr ? BLOCK_FLAG_RECALCULATE | BLOCK_FLAG_NOMINAL_LENGTH : BLOCK_FLAG_RECALCULATE;

  // Update previous path unit_vector and nominal speed
  previous_speed = current_speed;
  previous_nominal_speed_sqr = block->nominal_speed_sqr;

  // Update the position
  position = target;
  #if HAS_POSITION_FLOAT
    position_float = target_float;
  #endif

  #if HAS_GRADIENT_MIX
    mixer.gradient_control(target_float.z);
  #endif

  #if HAS_SD_RESTART
    block->sdpos = restart.get_sdpos();
  #endif

  // Movement was accepted
  return true;

} // fill_block()

/**
 * Planner::buffer_sync_block
 * Add a block to the buffer that just updates the position
 */
void Planner::buffer_sync_block() {
  // Wait for the next available block
  uint8_t next_buffer_head;
  block_t * const block = get_next_free_block(next_buffer_head);

  // Clear block
  memset(block, 0, sizeof(block_t));

  block->flag = BLOCK_FLAG_SYNC_POSITION;

  block->position = position;

  // If this is the first added movement, reload the delay, otherwise, cancel it.
  if (block_buffer_head == block_buffer_tail) {
    // If it was the first queued block, restart the 1st block delivery delay, to
    // give the planner an opportunity to queue more movements and plan them
    // As there are no queued movements, the Stepper ISR will not touch this
    // variable, so there is no risk setting this here (but it MUST be done
    // before the following line!!)
    delay_before_delivering = BLOCK_DELAY_FOR_1ST_MOVE;
  }

  block_buffer_head = next_buffer_head;

  stepper.wake_up();
}

/**
 * Planner::buffer_segment
 *
 * Add a new linear movement to the buffer in axis units.
 *
 * Leveling and kinematics should be applied ahead of calling this.
 *
 *  a,b,c,e     - target positions in mm and/or degrees
 *  fr_mm_s     - (target) speed of the move
 *  extruder    - target extruder
 *  millimeters - the length of the movement, if known
 */
bool Planner::buffer_segment(const float &a, const float &b, const float &c, const float &e
  #if HAS_DIST_MM_ARG
    , const xyze_float_t &cart_dist_mm
  #endif
  , const feedrate_t &fr_mm_s, const uint8_t extruder, const float &millimeters/*=0.0*/
) {

  // If we are cleaning, do not accept queuing of movements
  if (flag.clean_buffer) return false;

  // The target position of the tool in absolute steps
  // Calculate target position in absolute steps
  const abce_long_t target = {
    static_cast<int32_t>(FLOOR(a * mechanics.data.axis_steps_per_mm.a + 0.5f)),
    static_cast<int32_t>(FLOOR(b * mechanics.data.axis_steps_per_mm.b + 0.5f)),
    static_cast<int32_t>(FLOOR(c * mechanics.data.axis_steps_per_mm.c + 0.5f)),
    static_cast<int32_t>(FLOOR(e * extruders[extruder]->data.axis_steps_per_mm + 0.5f))
  };

  #if HAS_POSITION_FLOAT
    const xyze_pos_t target_float = { a, b, c, e };
  #endif

  // DRYRUN or Simulation prevents E moves from taking place
  if (printer.debugDryrun() || printer.debugSimulation()) {
    position.e = target.e;
    #if HAS_POSITION_FLOAT
      position_float.e = e;
    #endif
  }

  /* <-- add a slash to enable
    SERIAL_MV("  buffer_segment FR:", fr_mm_s);
    #if IS_KINEMATIC
      SERIAL_MV(" A:", a);
      SERIAL_MV(" (", position.a);
      SERIAL_MV("->", target.a);
      SERIAL_MV(") B:", b);
    #else
      SERIAL_MV(" X:", a);
      SERIAL_MV(" (", position.x);
      SERIAL_MV("->", target.x);
      SERIAL_MV(") Y:", b);
    #endif
    SERIAL_MV(" (", position.y);
    SERIAL_MV("->", target.y);
    #if MECH(DELTA)
      SERIAL_MV(") C:", c);
    #else
      SERIAL_MV(") Z:", c);
    #endif
    SERIAL_MV(" (", position.z);
    SERIAL_MV("->", target.z);
    SERIAL_MV(") E:", e);
    SERIAL_MV(" (", position.e);
    SERIAL_MV("->", target.e);
    SERIAL_EM(")");
  //*/

  // Simulation Mode no movement
  if (printer.debugSimulation()) position = target;

  // Queue the movement
  if (!buffer_steps(target
    #if HAS_POSITION_FLOAT
      , target_float
    #endif
    #if HAS_DIST_MM_ARG
      , cart_dist_mm
    #endif
    , fr_mm_s, extruder, millimeters
  )) return false;

  stepper.wake_up();
  return true;

}

/**
 * Add a new linear movement to the buffer.
 * The target is cartesian, it's translated to delta/scara if
 * needed.
 *
 *
 *  rx,ry,rz,e   - target position in mm or degrees
 *  fr_mm_s      - (target) speed of the move (mm/s)
 *  extruder     - target extruder
 *  millimeters  - the length of the movement, if known
 *  inv_duration - the reciprocal if the duration of the movement, if known (kinematic only if feeedrate scaling is enabled)
 */
bool Planner::buffer_line(const float &rx, const float &ry, const float &rz, const float &e, const feedrate_t &fr_mm_s, const uint8_t extruder, const float millimeters/*=0.0*/) {

  xyze_pos_t raw = { rx, ry, rz, e };
  #if HAS_POSITION_MODIFIERS
    apply_modifiers(raw);
  #endif

  #if IS_KINEMATIC

    #if HAS_JUNCTION_DEVIATION
      const xyze_pos_t cart_dist_mm = {
        rx - position_cart.x, ry - position_cart.y,
        rz - position_cart.z, e  - position_cart.e
      };
    #else
      const xyz_pos_t cart_dist_mm = { rx - position_cart.x, ry - position_cart.y, rz - position_cart.z };
    #endif

    float mm = millimeters;
    if (mm == 0.0)
      mm = (cart_dist_mm.x != 0.0 || cart_dist_mm.y != 0.0) ? cart_dist_mm.magnitude() : ABS(cart_dist_mm.z);

    mechanics.Transform(raw);

    #if ENABLED(SCARA_FEEDRATE_SCALING)
      // For SCARA scale the feed rate from mm/s to degrees/s
      // i.e., Complete the angular vector in the given time.
      const float duration_recip = inv_duration ? inv_duration : fr_mm_s / mm,
                  feedrate = HYPOT(delta.a - position_float.a, delta.b - position_float.b) * duration_recip;
    #else
      const float feedrate = fr_mm_s;
    #endif

    if (buffer_segment(mechanics.delta.a, mechanics.delta.b, mechanics.delta.c, raw.e
      #if HAS_JUNCTION_DEVIATION
        , cart_dist_mm
      #endif
      , feedrate, extruder, mm
    )) {
      position_cart.set(rx, ry, rz, e);
      return true;
    }
    else
      return false;

  #else

    return buffer_segment(raw, fr_mm_s, extruder, millimeters);

  #endif

}

/**
 * Directly set the planner ABC position (and stepper positions)
 * converting mm (or angles for SCARA) into steps.
 *
 * The provided ABC position is in machine units.
 */
void Planner::set_machine_position_mm(const float &a, const float &b, const float &c, const float &e) {

  position.set( static_cast<int32_t>(FLOOR(a * mechanics.data.axis_steps_per_mm.a + 0.5f)),
                static_cast<int32_t>(FLOOR(b * mechanics.data.axis_steps_per_mm.b + 0.5f)),
                static_cast<int32_t>(FLOOR(c * mechanics.data.axis_steps_per_mm.c + 0.5f)),
                static_cast<int32_t>(FLOOR(e * extruders[toolManager.extruder.active]->data.axis_steps_per_mm + 0.5f)));

  #if HAS_POSITION_FLOAT
    position_float.set(a, b, c, e);
  #endif

  if (has_blocks_queued()) {
    //previous_nominal_speed_sqr = 0.0;
    //ZERO(previous_speed);
    buffer_sync_block();
  }
  else
    stepper.set_position(position);

}

void Planner::set_position_mm(const float &rx, const float &ry, const float &rz, const float &e) {

  xyze_pos_t raw = { rx, ry, rz, e };

  #if HAS_POSITION_MODIFIERS
    apply_modifiers(raw, true);
  #endif

  #if IS_KINEMATIC
    position_cart.set(rx, ry, rz, e);
    mechanics.Transform(raw);
    set_machine_position_mm(mechanics.delta.a, mechanics.delta.b, mechanics.delta.c, raw.e);
  #else
    set_machine_position_mm(raw.x, raw.y, raw.z, raw.e);
  #endif
}

void Planner::set_e_position_mm(const float &e) {

  #if ENABLED(FWRETRACT)
    float e_new = e - fwretract.current_retract[toolManager.extruder.active];
  #else
    const float e_new = e;
  #endif

  position.e = static_cast<int32_t>(FLOOR(e_new * extruders[toolManager.extruder.active]->data.axis_steps_per_mm + 0.5f));

  #if HAS_POSITION_FLOAT
    position_float.e = e_new;
  #endif

  #if IS_KINEMATIC
    position_cart.e = e;
  #endif

  if (has_blocks_queued())
    buffer_sync_block();
  else
    stepper.set_position(E_AXIS, position.e);

}

/**
 * Recalculate the steps/s^2 acceleration rates, based on the mm/s^2
 */
void Planner::reset_acceleration_rates() {

  uint32_t highest_rate = 1;

  LOOP_XYZ(i) {
    mechanics.max_acceleration_steps_per_s2[i] = mechanics.data.max_acceleration_mm_per_s2[i] * mechanics.data.axis_steps_per_mm[i];
    NOLESS(highest_rate, mechanics.max_acceleration_steps_per_s2[i]);
  }
  LOOP_EXTRUDER() {
    extruders[e]->max_acceleration_steps_per_s2 = extruders[e]->data.max_acceleration_mm_per_s2 * extruders[e]->data.axis_steps_per_mm;
    if (e == toolManager.extruder.active) NOLESS(highest_rate, extruders[e]->max_acceleration_steps_per_s2);
  }

  cutoff_long = 4294967295UL / highest_rate; // 0xFFFFFFFFUL

  #if HAS_LINEAR_E_JERK
    mechanics.recalculate_max_e_jerk();
  #endif

}

/**
 * Recalculate position, steps_to_mm if data.axis_steps_per_mm changes!
 */
void Planner::refresh_positioning() {
  LOOP_XYZ(axis)  mechanics.steps_to_mm[axis] = 1.0f / mechanics.data.axis_steps_per_mm[axis];
  LOOP_EXTRUDER() extruders[e]->steps_to_mm   = 1.0f / extruders[e]->data.axis_steps_per_mm;
  set_position_mm(mechanics.position);
  reset_acceleration_rates();
}

#if HAS_TEMP_HOTEND && ENABLED(AUTOTEMP)

  void Planner::autotemp_M104_M109() {
    if ((flag.autotemp_enabled = parser.seen('F'))) autotemp_factor = parser.value_float();
    if (parser.seen('S')) autotemp_min = parser.value_celsius();
    if (parser.seen('B')) autotemp_max = parser.value_celsius();
  }

#endif

/** Private Function */
/**
 * Calculate trapezoid parameters, multiplying the entry- and exit-speeds
 * by the provided factors.
 **
 * ############ VERY IMPORTANT ############
 * NOTE that the PRECONDITION to call this function is that the block is
 * NOT BUSY and it is marked as RECALCULATE. That WARRANTIES the Stepper ISR
 * is not and will not use the block while we modify it, so it is safe to
 * alter it's values.
 */
#define MINIMAL_STEP_RATE 120

void Planner::calculate_trapezoid_for_block(block_t* const block, const float &entry_factor, const float &exit_factor) {

  uint32_t initial_rate = CEIL(entry_factor * block->nominal_rate),
           final_rate   = CEIL(exit_factor  * block->nominal_rate); // (steps per second)

  // Limit minimal step rate (Otherwise the timer will overflow.)
  NOLESS(initial_rate,  uint32_t(MINIMAL_STEP_RATE));
  NOLESS(final_rate,    uint32_t(MINIMAL_STEP_RATE));

  #if ENABLED(BEZIER_JERK_CONTROL)
    uint32_t cruise_rate = initial_rate;
  #endif

  const int32_t accel = block->acceleration_steps_per_s2;

            // Steps required for acceleration, deceleration to/from nominal rate
  uint32_t  accelerate_steps = CEIL(estimate_acceleration_distance(initial_rate, block->nominal_rate, accel)),
            decelerate_steps = FLOOR(estimate_acceleration_distance(block->nominal_rate, final_rate, -accel));
            // Steps between acceleration and deceleration, if any
  int32_t   plateau_steps = block->step_event_count - accelerate_steps - decelerate_steps;

  // Does accelerate_steps + decelerate_steps exceed step_event_count?
  // Then we can't possibly reach the nominal rate, there will be no cruising.
  // Use intersection_distance() to calculate accel / braking time in order to
  // reach the final_rate exactly at the end of this block.
  if (plateau_steps < 0) {
    const float accelerate_steps_float = CEIL(intersection_distance(initial_rate, final_rate, accel, block->step_event_count));
    accelerate_steps = MIN(uint32_t(MAX(accelerate_steps_float, 0)), block->step_event_count);
    plateau_steps = 0;

    #if ENABLED(BEZIER_JERK_CONTROL)
      // We won't reach the cruising rate. Let's calculate the speed we will reach
      cruise_rate = final_speed(initial_rate, accel, accelerate_steps);
    #endif
  }
  #if ENABLED(BEZIER_JERK_CONTROL)
    else // We have some plateau time, so the cruise rate will be the nominal rate
      cruise_rate = block->nominal_rate;
  #endif

  #if ENABLED(BEZIER_JERK_CONTROL)
    // Jerk controlled speed requires to express speed versus time, NOT steps
    uint32_t  acceleration_time = ((float)(cruise_rate - initial_rate) / accel) * (STEPPER_TIMER_RATE),
              deceleration_time = ((float)(cruise_rate - final_rate) / accel) * (STEPPER_TIMER_RATE);

    // And to offload calculations from the ISR, we also calculate the inverse of those times here
    uint32_t  acceleration_time_inverse = get_period_inverse(acceleration_time),
              deceleration_time_inverse = get_period_inverse(deceleration_time);
  #endif

  // Store new block parameters
  block->accelerate_until = accelerate_steps;
  block->decelerate_after = accelerate_steps + plateau_steps;
  block->initial_rate = initial_rate;
  #if ENABLED(BEZIER_JERK_CONTROL)
    block->acceleration_time = acceleration_time;
    block->deceleration_time = deceleration_time;
    block->acceleration_time_inverse = acceleration_time_inverse;
    block->deceleration_time_inverse = deceleration_time_inverse;
    block->cruise_rate = cruise_rate;
  #endif
  block->final_rate = final_rate;

}

/*                            PLANNER SPEED DEFINITION
                                     +--------+   <- current->nominal_speed
                                    /          \
         current->entry_speed ->   +            \
                                   |             + <- next->entry_speed (aka exit speed)
                                   +-------------+
                                       time -->

  Recalculates the motion plan according to the following basic guidelines:

    1. Go over every feasible block sequentially in reverse order and calculate the junction speeds
        (i.e. current->entry_speed) such that:
      a. No junction speed exceeds the pre-computed maximum junction speed limit or nominal speeds of
         neighboring blocks.
      b. A block entry speed cannot exceed one reverse-computed from its exit speed (next->entry_speed)
         with a maximum allowable deceleration over the block travel distance.
      c. The last (or newest appended) block is planned from a complete stop (an exit speed of zero).
    2. Go over every block in chronological (forward) order and dial down junction speed values if
      a. The exit speed exceeds the one forward-computed from its entry speed with the maximum allowable
         acceleration over the block travel distance.

  When these stages are complete, the planner will have maximized the velocity profiles throughout the all
  of the planner blocks, where every block is operating at its maximum allowable acceleration limits. In
  other words, for all of the blocks in the planner, the plan is optimal and no further speed improvements
  are possible. If a new block is added to the buffer, the plan is recomputed according to the said
  guidelines for a new optimal plan.

  To increase computational efficiency of these guidelines, a set of planner block pointers have been
  created to indicate stop-compute points for when the planner guidelines cannot logically make any further
  changes or improvements to the plan when in normal operation and new blocks are streamed and added to the
  planner buffer. For example, if a subset of sequential blocks in the planner have been planned and are
  bracketed by junction velocities at their maximums (or by the first planner block as well), no new block
  added to the planner buffer will alter the velocity profiles within them. So we no longer have to compute
  them. Or, if a set of sequential blocks from the first block in the planner (or a optimal stop-compute
  point) are all accelerating, they are all optimal and can not be altered by a new block added to the
  planner buffer, as this will only further increase the plan speed to chronological blocks until a maximum
  junction velocity is reached. However, if the operational conditions of the plan changes from infrequently
  used feed holds or feedrate overrides, the stop-compute pointers will be reset and the entire plan is
  recomputed as stated in the general guidelines.

  Planner buffer index mapping:
  - block_buffer_tail: Points to the beginning of the planner buffer. First to be executed or being executed.
  - block_buffer_head: Points to the buffer block after the last block in the buffer. Used to indicate whether
      the buffer is full or empty. As described for standard ring buffers, this block is always empty.
  - block_buffer_planned: Points to the first buffer block after the last optimally planned block for normal
      streaming operating conditions. Use for planning optimizations by avoiding recomputing parts of the
      planner buffer that don't change with the addition of a new block, as describe above. In addition,
      this block can never be less than block_buffer_tail and will always be pushed forward and maintain
      this requirement when encountered by the Planner::discard_current_block() routine during a cycle.

  NOTE: Since the planner only computes on what's in the planner buffer, some motions with lots of short
  line segments, like G2/3 arcs or complex curves, may seem to move slow. This is because there simply isn't
  enough combined distance traveled in the entire buffer to accelerate up to the nominal speed and then
  decelerate to a complete stop at the end of the buffer, as stated by the guidelines. If this happens and
  becomes an annoyance, there are a few simple solutions: (1) Maximize the machine acceleration. The planner
  will be able to compute higher velocity profiles within the same combined distance. (2) Maximize line
  motion(s) distance per block to a desired tolerance. The more combined distance the planner has to use,
  the faster it can go. (3) Maximize the planner buffer size. This also will increase the combined distance
  for the planner to compute over. It also increases the number of computations the planner has to perform
  to compute an optimal plan, so select carefully.
*/

// The kernel called by recalculate() when scanning the plan from last to first entry.
void Planner::reverse_pass_kernel(block_t* const current_block, const block_t* const next_block) {

  if (current_block) {
    // If entry speed is already at the maximum entry speed, and there was no change of speed
    // in the next block, there is no need to recheck. Block is cruising and there is no need to
    // compute anything for this block,
    // If not, block entry speed needs to be recalculated to ensure maximum possible planned speed.
    const float max_entry_speed_sqr = current_block->max_entry_speed_sqr;

    // Compute maximum entry speed decelerating over the current block from its exit speed.
    // If not at the maximum entry speed, or the previous block entry speed changed
    if (current_block->entry_speed_sqr != max_entry_speed_sqr || (next_block && TEST(next_block->flag, BLOCK_BIT_RECALCULATE))) {

      // If nominal length true, max junction speed is guaranteed to be reached.
      // If a block can de/ac-celerate from nominal speed to zero within the length of the block, then
      // the current block and next block junction speeds are guaranteed to always be at their maximum
      // junction speeds in deceleration and acceleration, respectively. This is due to how the current
      // block nominal speed limits both the current and next maximum junction speeds. Hence, in both
      // the reverse and forward planners, the corresponding block junction speed will always be at the
      // the maximum junction speed and may always be ignored for any speed reduction checks.

      const float new_entry_speed_sqr = TEST(current_block->flag, BLOCK_BIT_NOMINAL_LENGTH)
        ? max_entry_speed_sqr
        : MIN(max_entry_speed_sqr, max_allowable_speed_sqr(-current_block->acceleration, next_block ? next_block->entry_speed_sqr : sq(MINIMUM_PLANNER_SPEED), current_block->millimeters));
      if (current_block->entry_speed_sqr != new_entry_speed_sqr) {

        // Need to recalculate the block speed - Mark it now, so the stepper
        // ISR does not consume the block before being recalculated
        SBI(current_block->flag, BLOCK_BIT_RECALCULATE);

        // But there is an inherent race condition here, as the block maybe
        // became BUSY, just before it was marked as RECALCULATE, so check
        // if that is the case!
        if (stepper.is_block_busy(current_block)) {
          // Block became busy. Clear the RECALCULATE flag (no point in
          //  recalculating BUSY blocks and don't set its speed, as it can't
          //  be updated at this time.
          CBI(current_block->flag, BLOCK_BIT_RECALCULATE);
        }
        else {
          // Block is not BUSY, we won the race against the Stepper ISR:
          // Just Set the new entry speed
          current_block->entry_speed_sqr = new_entry_speed_sqr;
        }
      }
    }
  }
}

// The kernel called by recalculate() when scanning the plan from first to last entry.
void Planner::forward_pass_kernel(const block_t* const previous_block, block_t* const current_block, const uint8_t block_index) {

  if (previous_block) {
    // If the previous block is an acceleration block, too short to complete the full speed
    // change, adjust the entry speed accordingly. Entry speeds have already been reset,
    // maximized, and reverse-planned. If nominal length is set, max junction speed is
    // guaranteed to be reached. No need to recheck.
    if (!TEST(previous_block->flag, BLOCK_BIT_NOMINAL_LENGTH) &&
      previous_block->entry_speed_sqr < current_block->entry_speed_sqr) {

      // Compute the maximum allowable speed
      const float new_entry_speed_sqr = max_allowable_speed_sqr(-previous_block->acceleration, previous_block->entry_speed_sqr, previous_block->millimeters);

      // If true, current block is full-acceleration and we can move the planned pointer forward.
      if (new_entry_speed_sqr < current_block->entry_speed_sqr) {

        // Mark we need to recompute the trapezoidal shape, and do it now,
        // so the stepper ISR does not consume the block before being recalculated
        SBI(current_block->flag, BLOCK_BIT_RECALCULATE);

        // But there is an inherent race condition here, as the block maybe
        // became BUSY, just before it was marked as RECALCULATE, so check
        // if that is the case!
        if (stepper.is_block_busy(current_block)) {

          // Block became busy... Clear the RECALCULATE flag -There is no point
          //  recalculating BUSY blocks- and do not set it's speed, as it can't
          //  be updated at this time.
          CBI(current_block->flag, BLOCK_BIT_RECALCULATE);
        }
        else {
          // Block is not BUSY, we won the race against the Stepper ISR:

          // Always <= max_entry_speed_sqr. Backward pass sets this.
          current_block->entry_speed_sqr = new_entry_speed_sqr; // Always <= max_entry_speed_sqr. Backward pass sets this.

          // Set optimal plan pointer.
          block_buffer_planned = block_index;
        }
      }
    }

    // Any block set at its maximum entry speed also creates an optimal plan up to this
    // point in the buffer. When the plan is bracketed by either the beginning of the
    // buffer and a maximum entry speed or two maximum entry speeds, every block in between
    // cannot logically be further improved. Hence, we don't have to recompute them anymore.
    if (current_block->entry_speed_sqr == current_block->max_entry_speed_sqr)
      block_buffer_planned = block_index;
  }
}

/**
 * recalculate() needs to go over the current plan twice.
 * Once in reverse and once forward. This implements the reverse pass.
 */
void Planner::reverse_pass() {

  // Initialize block index to the last block in the planner buffer.
  uint8_t block_index = prev_block_index(block_buffer_head);

  // Read the index of the last buffer planned block. The ISR can change it
  // so it is better to have an stable local copy of it.
  uint8_t planned_block_index = block_buffer_planned;

  // If there was a race condition and block_buffer_planned was incremented
  //  or was pointing at the head (queue empty) break loop now and avoid
  //  planning already consumed blocks
  if (planned_block_index == block_buffer_head) return;

  // Reverse Pass: Coarsely maximize all possible deceleration curves back-planning from the last
  // block in buffer. Cease planning when the last optimal planned or tail pointer is reached.
  // NOTE: Forward pass will later refine and correct the reverse pass to create an optimal plan.
  const block_t *next_block = nullptr;
  while (block_index != planned_block_index) {

    // Perform the reverse pass
    block_t *current_block = &block_buffer[block_index];

    // Only consider non sync blocks
    if (!TEST(current_block->flag, BLOCK_BIT_SYNC_POSITION)) {
      reverse_pass_kernel(current_block, next_block);
      next_block = current_block;
    }

    // Advance to the next
    block_index = prev_block_index(block_index);

    // The ISR could advance the block_buffer_planned while we were doing the reverse pass.
    // We must try to avoid using an already consumed block as the last one - So follow
    // changes to the pointer and make sure to limit the loop to the currently busy block
    while (planned_block_index != block_buffer_planned) {

      // If we reached the busy block or an already processed block, break the loop now
      if (block_index == planned_block_index) return;

      // Advance the pointer, following the busy block
      planned_block_index = next_block_index(planned_block_index);
    }
  }
}

/**
 * recalculate() needs to go over the current plan twice.
 * Once in reverse and once forward. This implements the forward pass.
 */
void Planner::forward_pass() {

  // Forward Pass: Forward plan the acceleration curve from the planned pointer onward.
  // Also scans for optimal plan breakpoints and appropriately updates the planned pointer.

  // Begin at buffer planned pointer. Note that block_buffer_planned can be modified
  //  by the stepper ISR,  so read it ONCE. It it guaranteed that block_buffer_planned
  //  will never lead head, so the loop is safe to execute. Also note that the forward
  //  pass will never modify the values at the tail.
  uint8_t block_index = block_buffer_planned;

  block_t *current_block;
  const block_t * previous_block = nullptr;
  while (block_index != block_buffer_head) {

    // Perform the forward pass
    current_block = &block_buffer[block_index];

    // Skip SYNC blocks
    if (!TEST(current_block->flag, BLOCK_BIT_SYNC_POSITION)) {
      //  If we don't have a previous block or the previous block
      // is not busy (thus, modifiable), run the forward_pass_kernel.
      //  Otherwise, the previous block became BUSY (read only), so
      // we must assume the entry speed of the current block can't be
      // altered (as that would also require to update the exit speed
      // of the previous block)
      if (!previous_block || !stepper.is_block_busy(previous_block))
        forward_pass_kernel(previous_block, current_block, block_index);
      previous_block = current_block;
    }

    // Advance to the previous
    block_index = next_block_index(block_index);
  }

}

/**
 * Recalculate the trapezoid speed profiles for all blocks in the plan
 * according to the entry_factor for each junction. Must be called by
 * recalculate() after updating the blocks.
 */
void Planner::recalculate_trapezoids() {

  uint8_t block_index       = block_buffer_tail,
          head_block_index  = block_buffer_head;

  // Since there could be a sync block in the head of the queue, and the
  // next loop must not recalculate the head block (as it needs to be
  // specially handled), scan backwards to the first non-SYNC block.
  while (head_block_index != block_index) {

    // Go back (head always point to the first free block)
    const uint8_t prev_index = prev_block_index(head_block_index);

    // Get the pointer to the block
    block_t *prev = &block_buffer[prev_index];

    // If not dealing with a sync block, we are done. The last block is not a SYNC block
    if (!TEST(prev->flag, BLOCK_BIT_SYNC_POSITION)) break;

    // Lets examine the previous block. This one and all the following are SYNC blocks
    head_block_index = prev_index;
  };

  // Go from the tail (currently executed block) to the first block, without including it)
  block_t *current_block  = nullptr,
          *next_block     = nullptr;
  float   current_entry_speed = 0.0,
          next_entry_speed    = 0.0;

  while (block_index != head_block_index) {

    next_block = &block_buffer[block_index];

    // Skip sync blocks
    if (!TEST(next_block->flag, BLOCK_BIT_SYNC_POSITION)) {
      next_entry_speed = SQRT(next_block->entry_speed_sqr);

      if (current_block) {
        // Recalculate if current block entry or exit junction speed has changed.
        if (TEST(current_block->flag, BLOCK_BIT_RECALCULATE) || TEST(next_block->flag, BLOCK_BIT_RECALCULATE)) {

          // Mark the current block as RECALCULATE, to protect it from the Stepper ISR running it.
          // Note that due to the above condition, there's a chance the current block isn't marked as
          // RECALCULATE yet, but the next one is. That's the reason for the following line.
          SBI(current_block->flag, BLOCK_BIT_RECALCULATE);

          // But there is an inherent race condition here, as the block maybe
          // became BUSY, just before it was marked as RECALCULATE, so check
          // if that is the case!
          if (!stepper.is_block_busy(current_block)) {
            // Block is not BUSY, we won the race against the Stepper ISR:

            // NOTE: Entry and exit factors always > 0 by all previous logic operations.
            const float current_nominal_speed = SQRT(current_block->nominal_speed_sqr),
                        nomr = 1.0f / current_nominal_speed;
            calculate_trapezoid_for_block(current_block, current_entry_speed * nomr, next_entry_speed * nomr);
            #if ENABLED(LIN_ADVANCE)
              if (current_block->use_advance_lead) {
                const float comp = current_block->e_D_ratio * extruders[toolManager.extruder.active]->data.advance_K * extruders[toolManager.extruder.active]->data.axis_steps_per_mm;
                current_block->max_adv_steps = current_nominal_speed * comp;
                current_block->final_adv_steps = next_entry_speed * comp;
              }
            #endif
          }

          // Reset current only to ensure next trapezoid is computed - The
          // stepper is free to use the block from now on.
          CBI(current_block->flag, BLOCK_BIT_RECALCULATE);
        }
      }

      current_block = next_block;
      current_entry_speed = next_entry_speed;
    }

    block_index = next_block_index(block_index);
  }

  // Last/newest block in buffer. Exit speed is set with MINIMUM_PLANNER_SPEED. Always recalculated.
  if (next_block) {

    // Mark the next(last) block as RECALCULATE, to prevent the Stepper ISR running it.
    // As the last block is always recalculated here, there is a chance the block isn't
    // marked as RECALCULATE yet. That's the reason for the following line.
    SBI(next_block->flag, BLOCK_BIT_RECALCULATE);

    // But there is an inherent race condition here, as the block maybe
    // became BUSY, just before it was marked as RECALCULATE, so check
    // if that is the case!
    if (!stepper.is_block_busy(current_block)) {
      // Block is not BUSY, we won the race against the Stepper ISR:

      const float next_nominal_speed = SQRT(next_block->nominal_speed_sqr),
                  nomr = 1.0f / next_nominal_speed;
      calculate_trapezoid_for_block(next_block, next_entry_speed * nomr, (MINIMUM_PLANNER_SPEED) * nomr);
      #if ENABLED(LIN_ADVANCE)
        if (next_block->use_advance_lead) {
          const float comp = next_block->e_D_ratio * extruders[toolManager.extruder.active]->data.advance_K * extruders[toolManager.extruder.active]->data.axis_steps_per_mm;
          next_block->max_adv_steps = next_nominal_speed * comp;
          next_block->final_adv_steps = (MINIMUM_PLANNER_SPEED) * comp;
        }
      #endif
    }

    // Reset next only to ensure its trapezoid is computed - The stepper is free to use
    // the block from now on.
    CBI(next_block->flag, BLOCK_BIT_RECALCULATE);
  }

}

void Planner::recalculate() {
  // Initialize block index to the last block in the planner buffer.
  const uint8_t block_index = prev_block_index(block_buffer_head);

  // If there is just one block, no planning can be done. Avoid it!
  if (block_index != block_buffer_planned) {
    reverse_pass();
    forward_pass();
  }

  recalculate_trapezoids();
}
