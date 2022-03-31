/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "php_cbor.h"
#include "flags.h"

/* error codes */
typedef enum {
	/* E D   */ PHP_CBOR_ERROR_INVALID_FLAGS = 1,
	/* E D   */ PHP_CBOR_ERROR_INVALID_OPTIONS,
	/* E D   */ PHP_CBOR_ERROR_DEPTH,
	/* E     */ PHP_CBOR_ERROR_RECURSION,
	/* E D   */ PHP_CBOR_ERROR_SYNTAX,
	/* E D   */ PHP_CBOR_ERROR_UTF8,
	/* E D   */ PHP_CBOR_ERROR_UNSUPPORTED_TYPE = 17,
	/* E D   */ PHP_CBOR_ERROR_UNSUPPORTED_VALUE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_SIZE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE = 25,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_VALUE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_SIZE,
	/*   D   */ PHP_CBOR_ERROR_TRUNCATED_DATA = 33,
	/*   D   */ PHP_CBOR_ERROR_MALFORMED_DATA,
	/*   D   */ PHP_CBOR_ERROR_EXTRANEOUS_DATA,
	/* E D   */ PHP_CBOR_ERROR_TAG_SYNTAX = 41,
	/* E D   */ PHP_CBOR_ERROR_TAG_TYPE,
	/* E D   */ PHP_CBOR_ERROR_TAG_VALUE,
	/* E D   */ PHP_CBOR_ERROR_INTERNAL = 241,

	PHP_CBOR_STATUS_VALUE_FOLLOWS = 254,
	PHP_CBOR_ERROR_EXCEPTION,
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
	uint32_t flags;
	uint32_t max_depth;
	uint8_t string_ref;
	uint8_t shared_ref;
	bool datetime;
	bool bignum;
	bool decimal;
} php_cbor_encode_args;

typedef struct {
	uint32_t flags;
	uint32_t max_depth;
	uint32_t max_size;
	size_t error_arg;
	bool string_ref;
	uint8_t shared_ref;
} php_cbor_decode_args;

typedef union binary32_alias {
	uint32_t i;
	float f;
	struct {
		ZEND_ENDIAN_LOHI_4(
			char c3,
			char c2,
			char c1,
			char c0
		)
	} c;
} binary32_alias;

typedef union binary64_alias {
	uint64_t i;
	double f;
} binary64_alias;

#define CBOR_B32A_ISNAN(b32a)  ((binary32.i & 0x7f800000) == 0x7f800000 && (binary32.i & 0x007fffff) != 0) /* isnan(b32a.f) */

#define CBOR_CE(name)  php_cbor_##name##_ce

extern zend_class_entry
	*CBOR_CE(exception),
	*CBOR_CE(serializable),
	*CBOR_CE(encodeparams),
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

extern void php_cbor_throw_error(php_cbor_error error, bool has_arg, size_t arg);

extern php_cbor_error php_cbor_override_encode_options(php_cbor_encode_args *args, HashTable *options);
extern php_cbor_error php_cbor_set_encode_options(php_cbor_encode_args *args, HashTable *options);
extern php_cbor_error php_cbor_set_decode_options(php_cbor_decode_args *args, HashTable *options);

extern php_cbor_error php_cbor_encode(zval *value, zend_string **data, const php_cbor_encode_args *args);
extern php_cbor_error php_cbor_decode(zend_string *data, zval *value, php_cbor_decode_args *args);

extern zend_object *php_cbor_xstring_create(zend_class_entry *ce);
extern void php_cbor_xstring_set_value(zend_object *obj, zend_string *value);
extern zend_string *php_cbor_get_xstring_value(zval *value);
extern zend_object *php_cbor_floatx_create(zend_class_entry *ce);
extern bool php_cbor_floatx_set_value(zend_object *obj, zval *value, const char *raw);
extern size_t php_cbor_floatx_get_value(zend_object *obj, char *out);
extern double php_cbor_from_float16(uint16_t value);
extern uint16_t php_cbor_to_float16(float value);

extern bool php_cbor_is_len_string_ref(size_t str_len, uint32_t next_index);
