/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "php_cbor.h"
#include "flags.h"

/* Override libcbor fixed-value process of half-precision NaN */
#define PHP_CBOR_LIBCBOR_HACK_B16_NAN 1

/* Override libcbor process of half-precision denormalized number */
#define PHP_CBOR_LIBCBOR_HACK_B16_DENORM 1

/* error codes */
typedef enum {
	/* E D   */ PHP_CBOR_ERROR_INVALID_FLAGS = 1,
	/* E D   */ PHP_CBOR_ERROR_INVALID_OPTIONS,
	/* E D   */ PHP_CBOR_ERROR_DEPTH,
	/* E     */ PHP_CBOR_ERROR_RECURSION,
	/* E D   */ PHP_CBOR_ERROR_SYNTAX,
	/* E D   */ PHP_CBOR_ERROR_UTF8,
	/* E D   */ PHP_CBOR_ERROR_UNSUPPORTED_TYPE = 17,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_VALUE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_SIZE,
	/* E D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE = 25,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_VALUE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_SIZE,
	/*   D   */ PHP_CBOR_ERROR_TRUNCATED_DATA = 33,
	/*   D   */ PHP_CBOR_ERROR_MALFORMED_DATA,
	/*   D   */ PHP_CBOR_ERROR_EXTRANEOUS_DATA,
	/*   D   */ PHP_CBOR_ERROR_TAG_SYNTAX = 41,
	/*   D   */ PHP_CBOR_ERROR_TAG_TYPE,
	/*   D   */ PHP_CBOR_ERROR_TAG_VALUE,
	/* E D   */ PHP_CBOR_ERROR_INTERNAL = 255,

	PHP_CBOR_STATUS_VALUE_FOLLOWS,
} php_cbor_error;

enum {
	OPT_TRUE = 1,
	/* string ref */
	OPT_EXPLICIT = 2,
	/* shared ref */
	OPT_SHAREABLE = 2,
	OPT_UNSAFE_REF = 3,
};

typedef struct {
	int flags;
	uint32_t max_depth;
	uint8_t string_ref;
	uint8_t shared_ref;
	bool datetime;
} php_cbor_encode_args;

typedef struct {
	int flags;
	uint32_t max_depth;
	uint32_t max_size;
	size_t error_arg;
	bool string_ref;
	uint8_t shared_ref;
} php_cbor_decode_args;

typedef union binary32_alias_t {
	uint32_t i;
	float f;
} binary32_alias;

#define CBOR_B32A_ISNAN(b32a)  ((binary32.i & 0x7f800000) == 0x7f800000 && (binary32.i & 0x007fffff) != 0) /* isnan(b32a.f) */

#define CBOR_CE(name)  php_cbor_##name##_ce

extern zend_class_entry
	*CBOR_CE(exception),
	*CBOR_CE(encodable),
	*CBOR_CE(undefined),
	*CBOR_CE(xstring),
	*CBOR_CE(byte),
	*CBOR_CE(text),
	*CBOR_CE(floatx),
	*CBOR_CE(float16),
	*CBOR_CE(float32),
	*CBOR_CE(tag),
	*CBOR_CE(shareable)
;

extern void php_cbor_minit_types();
extern void php_cbor_minit_encode();
extern void php_cbor_minit_decode();

extern zend_object *php_cbor_get_undef();

extern php_cbor_error php_cbor_set_encode_options(php_cbor_encode_args *args, HashTable *options);
extern php_cbor_error php_cbor_set_decode_options(php_cbor_decode_args *args, HashTable *options);

extern php_cbor_error php_cbor_encode(zval *value, zend_string **data, const php_cbor_encode_args *args);
extern php_cbor_error php_cbor_decode(zend_string *data, zval *value, php_cbor_decode_args *args);

extern zend_string *php_cbor_get_xstring_value(zval *value);

bool php_cbor_is_len_string_ref(size_t str_len, uint32_t next_index);
