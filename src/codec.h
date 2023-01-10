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
	/*   D   */ CBOR_ERROR_DUPLICATE_KEY,
	/*   D   */ CBOR_ERROR_TRUNCATED_DATA = 33,
	/*   D   */ CBOR_ERROR_MALFORMED_DATA,
	/*   D   */ CBOR_ERROR_EXTRANEOUS_DATA,
	/* E D   */ CBOR_ERROR_TAG_SYNTAX = 41,
	/* E D   */ CBOR_ERROR_TAG_TYPE,
	/* E D   */ CBOR_ERROR_TAG_VALUE,
	/* E D   */ CBOR_ERROR_INTERNAL = 241,

	CBOR_STATUS_VALUE_FOLLOWS = 254,
	CBOR_ERROR_EXCEPTION,

	CBOR_ERROR_CODE_MASK = 0xff,
	CBOR_ERROR_DESC_SHIFT = 8,
	CBOR_ERROR_DESC_MASK = (0xff << CBOR_ERROR_DESC_SHIFT),

	CBOR_ERROR_INVALID_FLAGS__BOTH_STRING_FLAG = 1,
	CBOR_ERROR_INVALID_FLAGS__BOTH_KEY_STRING_FLAG,
	CBOR_ERROR_INVALID_FLAGS__NO_STRING_FLAG,
	CBOR_ERROR_INVALID_FLAGS__NO_KEY_STRING_FLAG,

	CBOR_ERROR_SYNTAX__BREAK_UNDERFLOW = 1,
	CBOR_ERROR_SYNTAX__BREAK_UNEXPECTED,
	CBOR_ERROR_SYNTAX__INCONSISTENT_STRING_TYPE,
	CBOR_ERROR_SYNTAX__NESTED_INDEF_STRING,

	CBOR_ERROR_UNSUPPORTED_TYPE__SIMPLE = 1,

	CBOR_ERROR_UNSUPPORTED_VALUE__INT_RANGE = 1,

	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__INT_KEY = 1,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__NULL,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__BOOL,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__FLOAT,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__BYTE,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__TEXT,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__ARRAY,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__OBJECT,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__UNDEF,
	CBOR_ERROR_UNSUPPORTED_KEY_TYPE__TAG,

	CBOR_ERROR_UNSUPPORTED_KEY_VALUE__RESERVED_PROP_NAME = 1,

	CBOR_ERROR_TAG_SYNTAX__STR_REF_NO_NS = 1,
	CBOR_ERROR_TAG_SYNTAX__SHARE_NESTED,

	CBOR_ERROR_TAG_TYPE__STR_REF_NOT_INT = 1,
	CBOR_ERROR_TAG_TYPE__SHARE_INCOMPATIBLE,
	CBOR_ERROR_TAG_TYPE__SHARE_NOT_INT,

	CBOR_ERROR_TAG_VALUE__STR_REF_RANGE = 1,
	CBOR_ERROR_TAG_VALUE__SHARE_RANGE,
} cbor_error;

#define E_DESC(e, d)  ((e) | (e##__##d << CBOR_ERROR_DESC_SHIFT))

enum {
	LEN_DEFAULT = -1,

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
	bool uri;
} cbor_encode_args;

typedef struct {
	uint32_t flags;
	uint32_t max_depth;
	uint32_t max_size;
	zend_long offset;
	zend_long length;
	size_t error_arg;
	bool string_ref;
	uint8_t shared_ref;
	struct {
		uint8_t indent;
		char indent_char;
		bool space;
		uint8_t byte_space;
		uint16_t byte_wrap;
	} edn;
} cbor_decode_args;

typedef struct {
	size_t length;
	size_t offset;
	size_t limit;  /* 0:unknown */
	size_t base;
	const uint8_t *ptr;
} cbor_fragment;

typedef struct cbor_decode_context cbor_decode_context;

void php_cbor_minit_encode();
void php_cbor_minit_decode();

/* options */
cbor_error php_cbor_override_encode_options(cbor_encode_args *args, HashTable *options);
cbor_error php_cbor_set_encode_options(cbor_encode_args *args, HashTable *options);
cbor_error php_cbor_set_decode_options(cbor_decode_args *args, HashTable *options);

void php_cbor_throw_error(cbor_error error, bool has_arg, size_t arg);

/* encode */
cbor_error php_cbor_encode(zval *value, zend_string **data, const cbor_encode_args *args);

/* decode */
cbor_error php_cbor_decode(zend_string *data, zval *value, cbor_decode_args *args);
cbor_decode_context *php_cbor_decode_new(const cbor_decode_args *args, cbor_fragment *mem);
void php_cbor_decode_delete(cbor_decode_context *ctx);
cbor_error php_cbor_decode_process(cbor_decode_context *ctx);
cbor_error php_cbor_decode_finish(cbor_decode_context *ctx, cbor_decode_args *args, cbor_error error, zval *value);

bool cbor_is_len_string_ref(size_t str_len, uint32_t next_index);
