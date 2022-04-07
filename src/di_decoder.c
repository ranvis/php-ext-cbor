/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "di_decoder.h"

/* This program assumes IEEE 754 binary32/64 to be used as float/double. */

#define DI_GET_INFO(ini_byte)  ((ini_byte) & 0x1f)  /* 0b0001_0000 */

#define SET_OUT_LEN(n, len, out) do { \
		if (!len || (n) > (len) - 1) { \
			(out)->req_len = 1 + (n);  /* may wrap around */ \
			(out)->read_len = 0; \
			return false; \
		} \
		(out)->read_len = 1 + (n); \
	} while (0)

#define ADD_OUT_LEN(n, len, out) do { \
		if (n > (len) - (out)->read_len) { \
			(out)->req_len = (out)->read_len + (n);  /* may wrap around */ \
			(out)->read_len = 0; \
			return false; \
		} \
		(out)->read_len += (n); \
	} while (0)

#define DI_READ_1(data)  ((data)[1])

#define DI_READ_2(data)  ( \
		((uint16_t)(data)[1] << 8) \
		| ((data)[2] << 0) \
	)
#define DI_READ_4(data)  ( \
		((uint32_t)(data)[1] << 24) \
		| ((uint32_t)(data)[2] << 16) \
		| ((uint32_t)(data)[3] << 8) \
		| ((data)[4] << 0) \
	)

#define DI_READ_8(data)  ( \
		((uint64_t)(data)[1] << 56) \
		| ((uint64_t)(data)[2] << 48) \
		| ((uint64_t)(data)[3] << 40) \
		| ((uint64_t)(data)[4] << 32) \
		| ((uint64_t)(data)[5] << 24) \
		| ((uint64_t)(data)[6] << 16) \
		| ((uint64_t)(data)[7] << 8) \
		| ((data)[8] << 0) \
	)

#define DECODE_ERROR(out)  do { \
		(out)->read_len = (out)->req_len = 0; \
		return false; \
	} while (0)

uint8_t cbor_di_get_type(const uint8_t *data, size_t len)
{
	assert(len > 0);
	uint8_t major = data[0] & DI_MAJOR_TYPE_MASK;
	switch (major) {
	case DI_MAJOR_TYPE(0):  /* unsigned integer */
		return DI_UINT;
	case DI_MAJOR_TYPE(1):  /* negative integer */
		return DI_NINT;
	case DI_MAJOR_TYPE(2):  /* byte string */
		return DI_BSTR;
	case DI_MAJOR_TYPE(3):  /* text string */
		return DI_TSTR;
	case DI_MAJOR_TYPE(4):  /* array */
		return DI_ARRAY;
	case DI_MAJOR_TYPE(5):  /* map */
		return DI_MAP;
	case DI_MAJOR_TYPE(6):  /* tagged data */
		return DI_TAG;
	}
	/* DI_MAJOR_TYPE(7) */
	uint8_t info = DI_GET_INFO(data[0]);
	switch (info) {
	/* simple values */
	case DI_INFO_FALSE:
		return DI_FALSE;
	case DI_INFO_TRUE:
		return DI_TRUE;
	case DI_INFO_NULL:
		return DI_NULL;
	case DI_INFO_UNDEF:
		return DI_UNDEF;
	case DI_INFO_SIMPLE:  /* simple values (DI_INFO_MAX + 1)..255 */
		return DI_SIMPLE;
	case DI_INFO_FLOAT16:
		return DI_FLOAT16;
	case DI_INFO_FLOAT32:
		return DI_FLOAT32;
	case DI_INFO_FLOAT64:
		return DI_FLOAT64;
	case DI_INFO_BREAK:
		return DI_BREAK;
	}
	/* 0..19: unassigned, 28..30: reserved */
	return 0;
}

bool cbor_di_read_int(const uint8_t *data, size_t len, cbor_di_decoded *out)
{
	uint8_t info = DI_GET_INFO(data[0]);
	if (info <= DI_INFO_INT0_MAX) {
		SET_OUT_LEN(0, len, out);
		out->v.i32 = info;
		return true;
	}
	switch (info) {
	case DI_INFO_INT8:
		SET_OUT_LEN(1, len, out);
		out->v.i32 = DI_READ_1(data);
		break;
	case DI_INFO_INT16:
		SET_OUT_LEN(2, len, out);
		out->v.i32 = DI_READ_2(data);
		break;
	case DI_INFO_INT32:
		SET_OUT_LEN(4, len, out);
		out->v.i32 = DI_READ_4(data);
		break;
	case DI_INFO_INT64:
		SET_OUT_LEN(8, len, out);
		out->v.i64 = DI_READ_8(data);
		break;
	default:  /* reserved or not well-formed */
		DECODE_ERROR(out);
	}
	return true;
}

bool cbor_di_read_str(const uint8_t *data, size_t len, cbor_di_decoded *out)
{
	uint8_t info = DI_GET_INFO(data[0]);
	size_t str_len;
	if (info <= DI_INFO_INT0_MAX) {
		SET_OUT_LEN(0, len, out);
		out->v.str.val = (const char *)&data[1];
		str_len = info;
	} else {
		switch (info) {
		case DI_INFO_INT8:
			SET_OUT_LEN(1, len, out);
			out->v.str.val = (const char *)&data[1 + 1];
			str_len = DI_READ_1(data);
			break;
		case DI_INFO_INT16:
			SET_OUT_LEN(2, len, out);
			out->v.str.val = (const char *)&data[1 + 2];
			str_len = DI_READ_2(data);
			break;
		case DI_INFO_INT32:
			SET_OUT_LEN(4, len, out);
			out->v.str.val = (const char *)&data[1 + 4];
			str_len = DI_READ_4(data);
			break;
		case DI_INFO_INT64:
			SET_OUT_LEN(8, len, out);
			out->v.str.val = (const char *)&data[1 + 8];
			str_len = DI_READ_8(data);
			break;
		case DI_INFO_INDEF:
			str_len = 0;
			out->v.str.val = NULL;
			break;
		default:  /* reserved */
			DECODE_ERROR(out);
		}
	}
	ADD_OUT_LEN(str_len, len, out);
	out->v.str.len = str_len;
	return true;
}

bool cbor_di_read_list(const uint8_t *data, size_t len, cbor_di_decoded *out)
{
	uint8_t info = DI_GET_INFO(data[0]);
	out->v.ext.is_indef = false;
	if (info <= DI_INFO_INT0_MAX) {
		SET_OUT_LEN(0, len, out);
		out->v.i64 = info;
		return true;
	}
	switch (info) {
	case DI_INFO_INT8:
		SET_OUT_LEN(1, len, out);
		out->v.i64 = DI_READ_1(data);
		break;
	case DI_INFO_INT16:
		SET_OUT_LEN(2, len, out);
		out->v.i64 = DI_READ_2(data);
		break;
	case DI_INFO_INT32:
		SET_OUT_LEN(4, len, out);
		out->v.i64 = DI_READ_4(data);
		break;
	case DI_INFO_INT64:
		SET_OUT_LEN(8, len, out);
		out->v.i64 = DI_READ_8(data);
		break;
	case DI_INFO_INDEF:
		SET_OUT_LEN(0, len, out);
		out->v.ext.is_indef = true;
		break;
	default:  /* reserved */
		DECODE_ERROR(out);
	}
	return true;
}

bool cbor_di_read_float(const uint8_t *data, size_t len, cbor_di_decoded *out)
{
	uint8_t info = DI_GET_INFO(data[0]);
	switch (info) {
	case DI_INFO_INT16:
		SET_OUT_LEN(2, len, out);
		out->v.f16 = DI_READ_2(data);
		break;
	case DI_INFO_INT32:
		SET_OUT_LEN(4, len, out);
		out->v.i32 = DI_READ_4(data);
		break;
	case DI_INFO_INT64:
		SET_OUT_LEN(8, len, out);
		out->v.i64 = DI_READ_8(data);
		break;
	default:  /* caller's logic error */
		DECODE_ERROR(out);
	}
	return true;
}
