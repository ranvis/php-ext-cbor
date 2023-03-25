/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "codec.h"

#define CHECK_ERROR(e) do { \
		if ((error = (e)) != 0) { \
			goto FINALLY; \
		} \
	} while (0)

static cbor_error bool_option(bool *opt_value, const char *name, size_t name_len, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	if (value == NULL) {
		return 0;
	}
	if (Z_TYPE_P(value) == IS_TRUE) {
		*opt_value = true;
	} else if (Z_TYPE_P(value) == IS_FALSE) {
		*opt_value = false;
	} else {
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	return 0;
}

static cbor_error bool_n_option(uint8_t *opt_value, const char *name, size_t name_len, char *other_values, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	if (value == NULL) {
		return 0;
	}
	switch (Z_TYPE_P(value)) {
	case IS_TRUE:
		*opt_value = OPT_TRUE;
		break;
	case IS_FALSE:
		*opt_value = 0;
		break;
	case IS_STRING:
		for (uint8_t i = 2; *other_values != '\0'; i++) {
			size_t other_len = strlen(other_values);
			if (*other_values == '-') {
				/* skip */
			} else if (zend_binary_strcmp(Z_STRVAL_P(value), Z_STRLEN_P(value), other_values, other_len) == 0) {
				*opt_value = i;
				return 0;
			}
			other_values += other_len + 1;
		}
		return CBOR_ERROR_INVALID_OPTIONS;
	default:
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	return 0;
}

static cbor_error long_option(zend_long *opt_value, const char *name, size_t name_len, zend_long min, zend_long max, HashTable *options, bool nullable)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	zend_long long_value;
	if (value == NULL) {
		return 0;
	}
	if (nullable && Z_TYPE_P(value) == IS_NULL) {
		return 0;
	} else if (Z_TYPE_P(value) != IS_LONG) {
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	long_value = Z_LVAL_P(value);
	if (long_value < min || long_value > max) {
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	*opt_value = long_value;
	return 0;
}

static cbor_error uint32_option(uint32_t *opt_value, const char *name, size_t name_len, uint32_t min, uint32_t max, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	zend_long long_value;
	if (value == NULL) {
		return 0;
	}
	if (Z_TYPE_P(value) != IS_LONG) {
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	long_value = Z_LVAL_P(value);
	if (long_value < 0) {
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	*opt_value = (uint32_t)long_value;
	if (*opt_value < min || *opt_value > max) {
		return CBOR_ERROR_INVALID_OPTIONS;
	}
	return 0;
}

cbor_error php_cbor_override_encode_options(cbor_encode_args *args, HashTable *options)
{
	cbor_error error = 0;
	CHECK_ERROR(bool_option(&args->datetime, ZEND_STRL("datetime"), options));
	CHECK_ERROR(bool_option(&args->bignum, ZEND_STRL("bignum"), options));
	CHECK_ERROR(bool_option(&args->decimal, ZEND_STRL("decimal"), options));
	CHECK_ERROR(bool_option(&args->uri, ZEND_STRL("uri"), options));
FINALLY:
	return error;
}

cbor_error php_cbor_set_encode_options(cbor_encode_args *args, HashTable *options)
{
	cbor_error error = 0;
	args->max_depth = 64;
	args->string_ref = 0;
	args->shared_ref = 0;
	args->datetime = true;
	args->bignum = true;
	args->decimal = true;
	args->uri = true;
	if (options == NULL) {
		return 0;
	}
	CHECK_ERROR(uint32_option(&args->max_depth, ZEND_STRL("max_depth"), 0, 10000, options));
	CHECK_ERROR(bool_n_option(&args->string_ref, ZEND_STRL("string_ref"), "explicit\0", options));
	CHECK_ERROR(bool_n_option(&args->shared_ref, ZEND_STRL("shared_ref"), "-\0-\0unsafe_ref\0", options));
	CHECK_ERROR(php_cbor_override_encode_options(args, options));
FINALLY:
	return error;
}

void php_cbor_init_decode_options(cbor_decode_args *args)
{
	args->flags = CBOR_BYTE | CBOR_KEY_BYTE;
	args->max_depth = 64;
	args->max_size = 65536;
	args->offset = 0;
	args->length = LEN_DEFAULT;
	args->string_ref = true;
	args->shared_ref = 0;
	args->edn.indent = 0;
	args->edn.indent_char = 0;
	args->edn.space = true;
	args->edn.byte_space = 0;
	args->edn.byte_wrap = 0;
}

void php_cbor_free_decode_options(cbor_decode_args *args)
{
}

cbor_error php_cbor_set_decode_options(cbor_decode_args *args, HashTable *options)
{
	cbor_error error = 0;
	if (options == NULL) {
		return 0;
	}
	CHECK_ERROR(uint32_option(&args->max_depth, ZEND_STRL("max_depth"), 0, 10000, options));
	CHECK_ERROR(uint32_option(&args->max_size, ZEND_STRL("max_size"), 0, 0xffffffff, options));
	if (args->max_size > HT_MAX_SIZE) {
		args->max_size = HT_MAX_SIZE; /* clamp silently */
	}
	CHECK_ERROR(long_option(&args->offset, ZEND_STRL("offset"), 0, ZEND_LONG_MAX, options, false));
	/* no negative length like substr() */
	CHECK_ERROR(long_option(&args->length, ZEND_STRL("length"), 0, ZEND_LONG_MAX, options, true));
	CHECK_ERROR(bool_option(&args->string_ref, ZEND_STRL("string_ref"), options));
	CHECK_ERROR(bool_n_option(&args->shared_ref, ZEND_STRL("shared_ref"), "shareable\0shareable_only\0unsafe_ref\0", options));
	if (args->flags & CBOR_EDN) {
		zval *opt_val;
		opt_val = zend_hash_str_find_deref(options, ZEND_STRL("indent"));
		if (opt_val) {
			if (Z_TYPE_P(opt_val) == IS_FALSE) {
				args->edn.indent = 0;
				args->edn.indent_char = 0;
			} else if (Z_TYPE_P(opt_val) == IS_LONG) {
				zend_long value = Z_LVAL_P(opt_val);
				if (value < 0 || value > 16) {
					CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
				}
				args->edn.indent = (uint8_t)value;
				args->edn.indent_char = ' ';
			} else if (Z_TYPE_P(opt_val) == IS_STRING) {
				zend_string *value = Z_STR_P(opt_val);
				if (ZSTR_LEN(value) != 1 || *ZSTR_VAL(value) != '\t') {
					CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
				}
				args->edn.indent = 1;
				args->edn.indent_char = '\t';
			} else {
				CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
			}
		}
		CHECK_ERROR(bool_option(&args->edn.space, ZEND_STRL("space"), options));
		opt_val = zend_hash_str_find_deref(options, ZEND_STRL("byte_space"));
		if (opt_val) {
			if (Z_TYPE_P(opt_val) == IS_LONG) {
				zend_long value = Z_LVAL_P(opt_val);
				if (value < 0 || value > 63) {
					CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
				}
				args->edn.byte_space = (uint8_t)value;
			} else {
				CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
			}
		}
		opt_val = zend_hash_str_find_deref(options, ZEND_STRL("byte_wrap"));
		if (opt_val) {
			if (Z_TYPE_P(opt_val) == IS_FALSE) {
				args->edn.byte_wrap = 0;
			} else if (Z_TYPE_P(opt_val) == IS_LONG) {
				zend_long value = Z_LVAL_P(opt_val);
				if (value <= 0 || value > 1024) {
					CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
				}
				args->edn.byte_wrap = (uint16_t)value;
			} else {
				CHECK_ERROR(CBOR_ERROR_INVALID_OPTIONS);
			}
		}
	}
FINALLY:
	return error;
}
