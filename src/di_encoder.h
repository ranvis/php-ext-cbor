#include "di.h"
#include <Zend/zend_smart_str_public.h>

void cbor_di_write_int(smart_str *buf, uint8_t di_type, uint64_t val);
void cbor_di_write_indef(smart_str *buf, uint8_t di_type);
void cbor_di_write_bool(smart_str *buf, bool val);
void cbor_di_write_null(smart_str *buf);
void cbor_di_write_undef(smart_str *buf);
void cbor_di_write_simple(smart_str *buf, uint8_t val);
uint8_t *cbor_di_write_float_head(smart_str *buf, int bits);
void cbor_di_write_float64(smart_str *buf, double val);
void cbor_di_write_float32(smart_str *buf, float val);
void cbor_di_write_float16(smart_str *buf, uint16_t val);
void cbor_di_write_break(smart_str *buf);
