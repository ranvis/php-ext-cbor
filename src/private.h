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
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_VALUE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_SIZE,
	/* E D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE = 25,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_VALUE,
	/*   D   */ PHP_CBOR_ERROR_UNSUPPORTED_KEY_SIZE,
	/*   D   */ PHP_CBOR_ERROR_TRUNCATED_DATA = 33,
	/*   D   */ PHP_CBOR_ERROR_MALFORMED_DATA,
	/*   D   */ PHP_CBOR_ERROR_EXTRANEOUS_DATA,
	/* E D   */ PHP_CBOR_ERROR_INTERNAL = 255,
} php_cbor_error;

typedef struct {
	int flags;
	int max_depth;
} php_cbor_encode_args;

typedef struct {
	int flags;
	int max_depth;
	uint32_t max_size;
	size_t error_arg;
} php_cbor_decode_args;

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
	*CBOR_CE(tag)
;

extern void php_cbor_minit_types();
extern void php_cbor_minit_encode();
extern void php_cbor_minit_decode();

extern zend_object *php_cbor_get_undef();

extern php_cbor_error php_cbor_set_encode_options(php_cbor_encode_args *args, HashTable *options);
extern php_cbor_error php_cbor_set_decode_options(php_cbor_decode_args *args, HashTable *options);

extern php_cbor_error php_cbor_encode(zval *value, zend_string **data, const php_cbor_encode_args *args);
extern php_cbor_error php_cbor_decode(zend_string *data, zval *value, php_cbor_decode_args *args);
