/*
 * dsh_fix16_float.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */
#include <math.h>
#include "inc/dtof_base_type.h"

#ifdef CALC_USING_FIX1616
static const dtof_real32_t fix16_maximum = 0x7FFFFFFF;  /*!< the maximum value of dtof_real32_t */
static const dtof_real32_t fix16_minimum = 0x80000000;  /*!< the minimum value of dtof_real32_t */
static const dtof_real32_t fix16_overflow = 0x80000000; /*!< the value used to indicate overflows when FIXMATH_NO_OVERFLOW is not specified */
static const dtof_real32_t fix16_one = 0x00010000;	   /*!< dtof_real32_t value of 1 */
static const dtof_real32_t fix8_one = 0x00001000;
static const dtof_real32_t fix16_eps = 1; /*!< dtof_real32_t epsilon */

/* Conversion functions between dtof_real32_t and float/integer.
 * These are inlined to allow compiler to optimize away constant numbers
 */
dtof_real32_t fix16_from_int(int a) { return a * fix16_one; }
dtof_real32_t fix16_from_float(float a) { return (dtof_real32_t)(a * fix16_one); }
int fix16_to_int(dtof_real32_t a) { return (int)(a / fix16_one); }

static inline uint32_t fix_abs(dtof_real32_t in)
{
	if (in == fix16_minimum)
	{
		// minimum negative number has same representation as
		// its absolute value in unsigned
		return 0x80000000;
	}
	else
	{
		return ((in >= 0) ? (in) : (-in));
	}
}

static uint8_t clz(uint32_t x)
{
	uint8_t result = 0;
	if (x == 0)
		return 32;
	while (!(x & 0xF0000000))
	{
		result += 4;
		x <<= 4;
	}
	while (!(x & 0x80000000))
	{
		result += 1;
		x <<= 1;
	}
	return result;
}

/* Subtraction and addition with overflow detection.
 * The versions without overflow detection are inlined in the header.
 */
dtof_real32_t fix16_add(dtof_real32_t a, dtof_real32_t b)
{
	// Use unsigned integers because overflow with signed integers is
	// an undefined operation (http://www.airs.com/blog/archives/120).
	dtof_real32_t _a = a;
	dtof_real32_t _b = b;
	dtof_real32_t sum = _a + _b;

	// Overflow can only happen if sign of a == sign of b, and then
	// it causes sign of sum != sign of a.
	if (!((_a ^ _b) & 0x80000000) && ((_a ^ sum) & 0x80000000))
		return fix16_overflow;

	return sum;
}

dtof_real32_t fix16_sub(dtof_real32_t a, dtof_real32_t b)
{
	dtof_real32_t _a = a;
	dtof_real32_t _b = b;
	dtof_real32_t diff = _a - _b;

	// Overflow can only happen if sign of a != sign of b, and then
	// it causes sign of diff != sign of a.
	if (((_a ^ _b) & 0x80000000) && ((_a ^ diff) & 0x80000000))
		return fix16_overflow;

	return diff;
}

/* 64-bit implementation for fix16_mul. Fastest version for e.g. ARM Cortex M3.
 * Performs a 32*32 -> 64bit multiplication. The middle 32 bits are the result,
 * bottom 16 bits are used for rounding, and upper 16 bits are used for overflow
 * detection.
 */

dtof_real32_t fix16_mul(dtof_real32_t inArg0, dtof_real32_t inArg1)
{
	int64_t product = (int64_t)inArg0 * inArg1;

#ifndef FIXMATH_NO_OVERFLOW
	// The upper 17 bits should all be the same (the sign).
	dtof_real32_t upper = (product >> 47);
#endif

	if (product < 0)
	{
#ifndef FIXMATH_NO_OVERFLOW
		if (~upper)
			return fix16_overflow;
#endif

#ifndef FIXMATH_NO_ROUNDING
		// This adjustment is required in order to round -1/2 correctly
		product--;
#endif
	}
	else
	{
#ifndef FIXMATH_NO_OVERFLOW
		if (upper)
			return fix16_overflow;
#endif
	}

#ifdef FIXMATH_NO_ROUNDING
	return product >> 16;
#else
	dtof_real32_t result = product >> 16;
	result += (product & 0x8000) >> 15;

	return result;
#endif
}

dtof_real32_t fix16_div(dtof_real32_t a, dtof_real32_t b)
{
	// This uses a hardware 32/32 bit division multiple times, until we have
	// computed all the bits in (a<<17)/b. Usually this takes 1-3 iterations.

	if (b == 0)
		return fix16_minimum;

	dtof_real32_t remainder = fix_abs(a);
	dtof_real32_t divider = fix_abs(b);
	uint64_t quotient = 0;
	int bit_pos = 17;

	// Kick-start the division a bit.
	// This improves speed in the worst-case scenarios where N and D are large
	// It gets a lower estimate for the result by N/(D >> 17 + 1).
	if (divider & 0xFFF00000)
	{
		dtof_real32_t shifted_div = ((divider >> 17) + 1);
		quotient = remainder / shifted_div;
		uint64_t tmp = ((uint64_t)quotient * (uint64_t)divider) >> 17;
		remainder -= (dtof_real32_t)(tmp);
	}

	// If the divider is divisible by 2^n, take advantage of it.
	while (!(divider & 0xF) && bit_pos >= 4)
	{
		divider >>= 4;
		bit_pos -= 4;
	}

	while (remainder && bit_pos >= 0)
	{
		// Shift remainder as much as we can without overflowing
		int shift = clz(remainder);
		if (shift > bit_pos)
			shift = bit_pos;
		remainder <<= shift;
		bit_pos -= shift;

		dtof_real32_t div = remainder / divider;
		remainder = remainder % divider;
		quotient += (uint64_t)div << bit_pos;

#ifndef FIXMATH_NO_OVERFLOW
		if (div & ~(0xFFFFFFFF >> bit_pos))
			return fix16_overflow;
#endif

		remainder <<= 1;
		bit_pos--;
	}

#ifndef FIXMATH_NO_ROUNDING
	// Quotient is always positive so rounding is easy
	quotient++;
#endif

	dtof_real32_t result = quotient >> 1;

	// Figure out the sign of the result
	if ((a ^ b) & 0x80000000)
	{
#ifndef FIXMATH_NO_OVERFLOW
		if (result == fix16_minimum)
			return fix16_overflow;
#endif

		result = -result;
	}

	return result;
}

dtof_real32_t fix16_sqrt(dtof_real32_t inValue)
{
	uint8_t neg = (inValue < 0);
	uint32_t num = fix_abs(inValue);
	uint32_t result = 0;
	uint32_t bit;
	uint8_t n;

	// Many numbers will be less than 15, so
	// this gives a good balance between time spent
	// in if vs. time spent in the while loop
	// when searching for the starting value.
	if (num & 0xFFF00000)
		bit = (uint32_t)1 << 30;
	else
		bit = (uint32_t)1 << 18;

	while (bit > num)
		bit >>= 2;

	// The main part is executed twice, in order to avoid
	// using 64 bit values in computations.
	for (n = 0; n < 2; n++)
	{
		// First we get the top 24 bits of the answer.
		while (bit)
		{
			if (num >= result + bit)
			{
				num -= result + bit;
				result = (result >> 1) + bit;
			}
			else
			{
				result = (result >> 1);
			}
			bit >>= 2;
		}

		if (n == 0)
		{
			// Then process it again to get the lowest 8 bits.
			if (num > 65535)
			{
				// The remainder 'num' is too large to be shifted left
				// by 16, so we have to add 1 to result manually and
				// adjust 'num' accordingly.
				// num = a - (result + 0.5)^2
				//	 = num + result^2 - (result + 0.5)^2
				//	 = num - result - 0.5
				num -= result;
				num = (num << 16) - 0x8000;
				result = (result << 16) + 0x8000;
			}
			else
			{
				num <<= 16;
				result <<= 16;
			}

			bit = 1 << 14;
		}
	}

#ifndef FIXMATH_NO_ROUNDING
	// Finally, if next bit would have been 1, round the result upwards.
	if (num > result)
	{
		result++;
	}
#endif

	return (neg ? -(dtof_real32_t)result : (dtof_real32_t)result);
}

static inline dtof_real32_t fix16_rs(dtof_real32_t x)
{
	dtof_real32_t y = (x / 10) + (x & 5);
	return y;
}

static dtof_real32_t fix16_log10_inner(dtof_real32_t x)
{
	dtof_real32_t result = 0;

	while (x >= fix16_from_int(10))
	{
		result++;
		x = fix16_rs(x);
	}

	if (x == 0)
		return (result << 16);

	uint_fast8_t i;
	for (i = 16; i > 0; i--)
	{
		x = fix16_mul(x, x);
		result <<= 1;
		if (x >= fix16_from_int(10))
		{
			result |= 1;
			x = fix16_rs(x);
		}
	}
	x = fix16_mul(x, x);
	if (x >= fix16_from_int(10))
		result++;

	return result;
}

dtof_real32_t fix16_log10(dtof_real32_t x)
{
	// Note that a negative x gives a non-real result.
	// If x == 0, the limit of log2(x)  as x -> 0 = -infinity.
	// log2(-ve) gives a complex result.
	if (x <= 0)
		return fix16_overflow;

	// If the input is less than one, the result is -log2(1.0 / in)
	if (x < fix16_one)
	{
		// Note that the inverse of this would overflow.
		// This is the exact answer for log2(1.0 / 65536)
		if (x == 1)
			return fix16_from_int(-16);

		dtof_real32_t inverse = fix16_div(fix16_one, x);
		return -fix16_log10_inner(inverse);
	}

	// If input >= 1, just proceed as normal.
	// Note that x == fix16_one is a special case, where the answer is 0.
	return fix16_log10_inner(x);
}

#else
/* Subtraction and addition with overflow detection.
 * The versions without overflow detection are inlined in the header.
 */
dtof_real32_t float_add(dtof_real32_t a, dtof_real32_t b)
{
	return (a + b);
}

dtof_real32_t float_sub(dtof_real32_t a, dtof_real32_t b)
{
	return (a - b);
}

/* 64-bit implementation for fix16_mul. Fastest version for e.g. ARM Cortex M3.
 * Performs a 32*32 -> 64bit multiplication. The middle 32 bits are the result,
 * bottom 16 bits are used for rounding, and upper 16 bits are used for overflow
 * detection.
 */

dtof_real32_t float_mul(dtof_real32_t a, dtof_real32_t b)
{
	return (a * b);
}

dtof_real32_t float_div(dtof_real32_t a, dtof_real32_t b)
{
	// This uses a hardware 32/32 bit division multiple times, until we have
	// computed all the bits in (a<<17)/b. Usually this takes 1-3 iterations.

	if (b == 0)
		return 0.0f;

	return (a / b);
}

dtof_real32_t float_sqrt(dtof_real32_t a)
{
	return powf(a, 0.5);
}

dtof_real32_t float_from_float(float a) { return (dtof_real32_t)a; }
dtof_real32_t float_from_int(int a) { return (dtof_real32_t)a; }
int float_to_int(dtof_real32_t a) { return (int)a; }
#endif
