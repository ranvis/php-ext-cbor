/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include <immintrin.h>

#define F64_ALIAS_TYPE  binary64_alias
#define F64_FP_TYPE  double
#define F64_UINT_TYPE  uint64_t
#define F64_EXP_BITS  11
#define F64_EXP_FILL  0x7ffull
#define F64_EXP_BIAS  1023
#define F64_FRAC_BITS  52
#define F64_FRAC_MASK  0xfffffffffffffull

#define F32_ALIAS_TYPE  binary32_alias
#define F32_FP_TYPE  float
#define F32_UINT_TYPE  uint32_t
#define F32_EXP_BITS  8
#define F32_EXP_FILL  0xfful
#define F32_EXP_BIAS  127
#define F32_EXP_MIN  -126
#define F32_EXP_MAX  127
#define F32_FRAC_BITS  23
#define F32_FRAC_MASK  0x7ffffful

#define F16_EXP_BITS  5
#define F16_EXP_FILL  0x3ffu
#define F16_EXP_BIAS  15
#define F16_EXP_MIN  -14
#define F16_EXP_MAX  15
#define F16_FRAC_BITS  10

static bool f16c_available;

// Test if smaller type will fit. (But not to subnormal numbers.)

int test_fp32_size(float value)
{
	F32_ALIAS_TYPE bin;
	bin.f = value;
	if (bin.i == 0) { // +0
		return 2;
	}
	F32_UINT_TYPE exp = (bin.i >> F32_FRAC_BITS) & F32_EXP_FILL;
	F32_UINT_TYPE frac = bin.i & F32_FRAC_MASK;
	bool is_nonfinite = exp == F32_EXP_FILL;
	if ((is_nonfinite || (exp >= F32_EXP_BIAS + F16_EXP_MIN && exp <= F32_EXP_BIAS + F16_EXP_MAX)) && !(frac & (F32_FRAC_MASK >> F16_FRAC_BITS))) {
		return 2;
	}
	return 4;
}

int test_fp64_size(double value)
{
	F64_ALIAS_TYPE bin;
	bin.f = value;
	if (bin.i == 0) { // +0
		return 2;
	}
	F64_UINT_TYPE exp = (bin.i >> F64_FRAC_BITS) & F64_EXP_FILL;
	F64_UINT_TYPE frac = bin.i & F64_FRAC_MASK;
	bool is_nonfinite = exp == F64_EXP_FILL;
	if ((is_nonfinite || (exp >= F64_EXP_BIAS + F32_EXP_MIN && exp <= F64_EXP_BIAS + F32_EXP_MAX)) && !(frac & (F64_FRAC_MASK >> F32_FRAC_BITS))) {
		if ((is_nonfinite || (exp >= F64_EXP_BIAS + F16_EXP_MIN && exp <= F64_EXP_BIAS + F16_EXP_MAX)) && !(frac & (F64_FRAC_MASK >> F16_FRAC_BITS))) {
			return 2;
		}
		return 4;
	}
	return 8;
}

void php_cbor_minit_types_float_cast()
{
	int cpu_id[4];
	const int flags = CPU_ID_1_2_SSE3 | CPU_ID_1_2_F16C;
	f16c_available = get_cpu_id(1, cpu_id) && (cpu_id[2] & flags) == flags;
}

static bool has_f16c()
{
#if defined(__SSE2__) && defined(__F16C__)
#define COMPILE_F16C
	return f16c_available;
#else
	return false;
#endif
}

static float fp16_to_fp32_i(cbor_fp16i sh)
{
#ifdef COMPILE_F16C
	__m128i vh = _mm_cvtsi32_si128(sh);
	__m128 vs = _mm_cvtph_ps(vh);
	return _mm_cvtss_f32(vs);
#else
	return 0;
#endif
}

static cbor_fp16i fp32_to_fp16_i(float ss)
{
#ifdef COMPILE_F16C
	__m128 vs = _mm_set_ss(ss);
	__m128i vh = _mm_cvtps_ph(vs, 0);
	return (cbor_fp16i)_mm_cvtsi128_si32(vh);
#else
	return 0;
#endif
}


#define FC_FN_NAME  cbor_float_64_to_16
#define FC_FN_CAST_NAME  cbor_float_64_to_16_cast
#define FC_ALIAS_TYPE  F64_ALIAS_TYPE
#define FC_FP_TYPE  F64_FP_TYPE
#define FC_UINT_TYPE  F64_UINT_TYPE
#define FC_EXP_BITS  F64_EXP_BITS
#define FC_EXP_FILL  F64_EXP_FILL
#define FC_FRAC_BITS  F64_FRAC_BITS
#define FC_FRAC_MASK  F64_FRAC_MASK
#define FC_EXP_BIAS  F64_EXP_BIAS

#include "type_float_cast_16.h"

#define FC_FN_NAME  cbor_float_32_to_16
#define FC_FN_CAST_NAME  cbor_float_32_to_16_cast
#define FC_ALIAS_TYPE  F32_ALIAS_TYPE
#define FC_FP_TYPE  F32_FP_TYPE
#define FC_UINT_TYPE  F32_UINT_TYPE
#define FC_EXP_BITS  F32_EXP_BITS
#define FC_EXP_FILL  F32_EXP_FILL
#define FC_EXP_BIAS  F32_EXP_BIAS
#define FC_FRAC_BITS  F32_FRAC_BITS
#define FC_FRAC_MASK  F32_FRAC_MASK

#include "type_float_cast_16.h"
