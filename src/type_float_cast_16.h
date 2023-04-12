/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define FC_SIGN_SHIFT (FC_EXP_BITS + FC_FRAC_BITS)
#define FC_SIGN_MASK (1ull << FC_SIGN_SHIFT)

#define SIGN_BITS_DIFF  (FC_SIGN_SHIFT - (F16_EXP_BITS + F16_FRAC_BITS))

cbor_fp16i FC_FN_CAST_NAME(FC_FP_TYPE value)
{
	if (has_f16c()) {
		return fp32_to_fp16_i((float)value);
	}
	cbor_fp16i bin16 = 0;
	FC_ALIAS_TYPE bin;
	bin.f = value;
	int exp0 = ((bin.i & (FC_EXP_FILL << FC_FRAC_BITS)) >> FC_FRAC_BITS) - FC_EXP_BIAS;
	if (exp0 < F16_EXP_MIN - F16_FRAC_BITS) {  // Too small
		bin16 = (cbor_fp16i)((bin.i & FC_SIGN_MASK) >> SIGN_BITS_DIFF);
		// For exp0 = F16_EXP_MIN - F16_FRAC_BITS - 1, think this does round to even to avoid negative shift below
	} else if (exp0 < F16_EXP_MIN) {
		FC_UINT_TYPE frac = bin.i & FC_FRAC_MASK;
		int shift = exp0 - F16_EXP_MIN + F16_FRAC_BITS;
		assert(shift >= 0);
		bin16 = (cbor_fp16i)(0
			| ((bin.i & FC_SIGN_MASK) >> SIGN_BITS_DIFF)
			| (uint64_t)(1u << shift)  // exp to fract
			| (((frac >> (FC_FRAC_BITS - shift - 1)) + 1) >> 1)  // Round half away from zero for simplicity
		);
	} else if (exp0 <= F16_EXP_MAX) {
		FC_UINT_TYPE frac = bin.i & FC_FRAC_MASK;
		bin16 = (cbor_fp16i)(0
			| ((bin.i & FC_SIGN_MASK) >> SIGN_BITS_DIFF)
			| ((exp0 + F16_EXP_BIAS) << F16_FRAC_BITS)
			| (((frac >> (FC_FRAC_BITS - F16_FRAC_BITS - 1)) + 1) >> 1)  // Round half away from zero for simplicity
		);
	} else {
		bin16 = (cbor_fp16i)(0
			| ((bin.i & FC_SIGN_MASK) >> SIGN_BITS_DIFF)
			| (0x1f << F16_FRAC_BITS)
		);
	}
	return bin16;
}

cbor_fp16i FC_FN_NAME(FC_FP_TYPE value)
{
	if (EXPECTED(!isnan(value))) {
		return FC_FN_CAST_NAME(value);
	}
	cbor_fp16i bin16 = 0;
	FC_ALIAS_TYPE bin;
	bin.f = value;
	bin16 = 0x7c00  // sNaN 0b0_11111_qn_nnnn_nnnn (INF)
		| (cbor_fp16i)((bin.i & FC_SIGN_MASK) >> SIGN_BITS_DIFF)
		| (cbor_fp16i)((bin.i & ((FC_UINT_TYPE)F16_EXP_FILL << (FC_FRAC_BITS - 10))) >> (FC_FRAC_BITS - F16_FRAC_BITS));  // high bits
	if (bin16 == 0x7c00) {
		bin16 = 0x7e00; // INF => qNaN
	}
	return bin16;
}

#undef FC_SIGN_SHIFT
#undef FC_SIGN_MASK

#undef SIGN_BITS_DIFF

#undef FC_FN_NAME
#undef FC_FN_CAST_NAME
#undef FC_ALIAS_TYPE
#undef FC_FP_TYPE
#undef FC_UINT_TYPE
#undef FC_EXP_BITS
#undef FC_EXP_FILL
#undef FC_FRAC_BITS
#undef FC_EXP_BIAS
#undef FC_FRAC_MASK
