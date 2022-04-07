#include "flags.h"

/* error codes */
typedef enum {
	/* E D   */ CBOR_ERROR_INVALID_FLAGS = 1,
	/* E D   */ CBOR_ERROR_INVALID_OPTIONS,
	/* E D   */ CBOR_ERROR_DEPTH,
	/* E     */ CBOR_ERROR_RECURSION,
	/* E D   */ CBOR_ERROR_SYNTAX,
	/* E D   */ CBOR_ERROR_UTF8,
	/* E D   */ CBOR_ERROR_UNSUPPORTED_TYPE = 17,
	/* E D   */ CBOR_ERROR_UNSUPPORTED_VALUE,
	/*   D   */ CBOR_ERROR_UNSUPPORTED_SIZE,
	/*   D   */ CBOR_ERROR_UNSUPPORTED_KEY_TYPE = 25,
	/*   D   */ CBOR_ERROR_UNSUPPORTED_KEY_VALUE,
	/*   D   */ CBOR_ERROR_UNSUPPORTED_KEY_SIZE,
	/*   D   */ CBOR_ERROR_TRUNCATED_DATA = 33,
	/*   D   */ CBOR_ERROR_MALFORMED_DATA,
	/*   D   */ CBOR_ERROR_EXTRANEOUS_DATA,
	/* E D   */ CBOR_ERROR_TAG_SYNTAX = 41,
	/* E D   */ CBOR_ERROR_TAG_TYPE,
	/* E D   */ CBOR_ERROR_TAG_VALUE,
	/* E D   */ CBOR_ERROR_INTERNAL = 241,

	CBOR_STATUS_VALUE_FOLLOWS = 254,
	CBOR_ERROR_EXCEPTION,
} cbor_error;

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
} cbor_encode_args;

typedef struct {
	uint32_t flags;
	uint32_t max_depth;
	uint32_t max_size;
	size_t error_arg;
	bool string_ref;
	uint8_t shared_ref;
} cbor_decode_args;

extern void php_cbor_minit_encode();
extern void php_cbor_minit_decode();

extern void php_cbor_throw_error(cbor_error error, bool has_arg, size_t arg);

extern cbor_error php_cbor_override_encode_options(cbor_encode_args *args, HashTable *options);
extern cbor_error php_cbor_set_encode_options(cbor_encode_args *args, HashTable *options);
extern cbor_error php_cbor_set_decode_options(cbor_decode_args *args, HashTable *options);

extern cbor_error php_cbor_encode(zval *value, zend_string **data, const cbor_encode_args *args);
extern cbor_error php_cbor_decode(zend_string *data, zval *value, cbor_decode_args *args);

extern bool php_cbor_is_len_string_ref(size_t str_len, uint32_t next_index);
