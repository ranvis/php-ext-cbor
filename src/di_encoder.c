/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "di.h"
#include "types.h"
#include <Zend/zend_smart_str.h>
#include <assert.h>

void cbor_di_write_int(smart_str *buf, uint8_t di_type, uint64_t val)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(di_type - DI_UINT);
	assert(di_type >= DI_UINT && di_type <= DI_TAG && DI_TAG - DI_UINT == 6);
	if (val <= DI_INFO_INT0_MAX) {
		smart_str_appendc(buf, ini_byte | (uint8_t)val);
	} else if (val <= 0xff) {
		uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 1);
		*ptr++ = ini_byte | DI_INFO_INT8;
		*ptr++ = (uint8_t)val;
	} else if (val <= 0xffff) {
		uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 2);
		*ptr++ = ini_byte | DI_INFO_INT16;
		*ptr++ = (uint8_t)(val >> 8);
		*ptr++ = (uint8_t)(val >> 0);
	} else if (val <= 0xffffffff) {
		uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 4);
		*ptr++ = ini_byte | DI_INFO_INT32;
		*ptr++ = (uint8_t)(val >> 24);
		*ptr++ = (uint8_t)(val >> 16);
		*ptr++ = (uint8_t)(val >> 8);
		*ptr++ = (uint8_t)(val >> 0);
	} else {
		uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 8);
		*ptr++ = ini_byte | DI_INFO_INT64;
		*ptr++ = (uint8_t)(val >> 56);
		*ptr++ = (uint8_t)(val >> 48);
		*ptr++ = (uint8_t)(val >> 40);
		*ptr++ = (uint8_t)(val >> 32);
		*ptr++ = (uint8_t)(val >> 24);
		*ptr++ = (uint8_t)(val >> 16);
		*ptr++ = (uint8_t)(val >> 8);
		*ptr++ = (uint8_t)(val >> 0);
	}
}

void cbor_di_write_indef(smart_str *buf, uint8_t di_type)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(di_type - DI_UINT);
	assert(di_type >= DI_BSTR && di_type <= DI_MAP);
	smart_str_appendc(buf, ini_byte | DI_INFO_INDEF);
}

void cbor_di_write_bool(smart_str *buf, bool val)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	smart_str_appendc(buf, ini_byte | (val ? DI_INFO_TRUE : DI_INFO_FALSE));
}

void cbor_di_write_null(smart_str *buf)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	smart_str_appendc(buf, ini_byte | DI_INFO_NULL);
}

void cbor_di_write_undef(smart_str *buf)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	smart_str_appendc(buf, ini_byte | DI_INFO_UNDEF);
}

void cbor_di_write_simple(smart_str *buf, uint8_t val)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	if (val <= DI_INFO_MAX) {
		smart_str_appendc(buf, ini_byte | val);
	} else {
		uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 1);
		*ptr++ = ini_byte | DI_INFO_SIMPLE;
		*ptr++ = (uint8_t)val;
	}
}

uint8_t *cbor_di_write_float_head(smart_str *buf, int bits)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	uint8_t add_info;
	if (bits == 64) {
		add_info = DI_INFO_FLOAT64;
	} else if (bits == 32) {
		add_info = DI_INFO_FLOAT32;
	} else {
		assert(bits == 16);
		add_info = DI_INFO_FLOAT16;
	}
	uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + (bits / 8));
	*ptr++ = ini_byte | add_info;
	return ptr;
}

void cbor_di_write_float64(smart_str *buf, double val)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	binary64_alias bin;
	uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 8);
	bin.f = val;
	*ptr++ = ini_byte | DI_INFO_FLOAT64;
	*ptr++ = (uint8_t)(bin.i >> 56);
	*ptr++ = (uint8_t)(bin.i >> 48);
	*ptr++ = (uint8_t)(bin.i >> 40);
	*ptr++ = (uint8_t)(bin.i >> 32);
	*ptr++ = (uint8_t)(bin.i >> 24);
	*ptr++ = (uint8_t)(bin.i >> 16);
	*ptr++ = (uint8_t)(bin.i >> 8);
	*ptr++ = (uint8_t)(bin.i >> 0);
}

void cbor_di_write_float32(smart_str *buf, float val)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	binary32_alias bin;
	uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 4);
	bin.f = val;
	*ptr++ = ini_byte | DI_INFO_FLOAT32;
	*ptr++ = (uint8_t)(bin.i >> 24);
	*ptr++ = (uint8_t)(bin.i >> 16);
	*ptr++ = (uint8_t)(bin.i >> 8);
	*ptr++ = (uint8_t)(bin.i >> 0);
}

void cbor_di_write_float16(smart_str *buf, uint16_t val)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	uint8_t *ptr = (uint8_t *)smart_str_extend(buf, 1 + 2);
	*ptr++ = ini_byte | DI_INFO_FLOAT16;
	*ptr++ = (uint8_t)(val >> 8);
	*ptr++ = (uint8_t)(val >> 0);
}

void cbor_di_write_break(smart_str *buf)
{
	uint8_t ini_byte = DI_MAJOR_TYPE(7);
	smart_str_appendc(buf, ini_byte | DI_INFO_BREAK);
}
