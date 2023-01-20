/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "di.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
	size_t read_len;
	size_t req_len;  /* if returned false and req_len == 0, data is truncated */
	union cbor_di_decoded_t {
		uint32_t i32;
		uint64_t i64;
		struct {
			size_t len;
			const char *val;  /* NULL: indefinite */
		} str;
		uint16_t f16;
		float f32;
		double f64;
		struct {
			uint64_t int_filler;
			bool is_indef;
		} ext;
	} v;
} cbor_di_decoded;

#define DI_IS_SINGLE(out)  ((out).read_len == 1 + 4)
#define DI_IS_DOUBLE(out)  ((out).read_len == 1 + 8)

uint8_t cbor_di_get_type(const uint8_t *data, size_t len);
bool cbor_di_read_int(const uint8_t *data, size_t len, cbor_di_decoded *out);
bool cbor_di_read_str(const uint8_t *data, size_t len, cbor_di_decoded *out);
bool cbor_di_read_list(const uint8_t *data, size_t len, cbor_di_decoded *out);
bool cbor_di_read_float(const uint8_t *data, size_t len, cbor_di_decoded *out);
