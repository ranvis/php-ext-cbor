/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"

#define CHECK_ERROR(e) do { \
		if (e) { \
			goto FINALLY; \
		} \
	} while (0)

static php_cbor_error bool_option(bool *opt_value, const char *name, size_t name_len, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	if (value == NULL) {
		return 0;
	}
	ZVAL_DEREF(value);
	if (Z_TYPE_P(value) == IS_TRUE) {
		*opt_value = true;
	} else if (Z_TYPE_P(value) == IS_FALSE) {
		*opt_value = false;
	} else {
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
	}
	return 0;
}

static php_cbor_error bool3_option(uint8_t *opt_value, const char *name, size_t name_len, char *value3, size_t value3_len, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	if (value == NULL) {
		return 0;
	}
	ZVAL_DEREF(value);
	switch (Z_TYPE_P(value)) {
	case IS_TRUE:
		*opt_value = 1;
		break;
	case IS_FALSE:
		*opt_value = 0;
		break;
	case IS_STRING:
		if (zend_binary_strcmp(Z_STRVAL_P(value), Z_STRLEN_P(value), value3, value3_len) != 0) {
			return PHP_CBOR_ERROR_INVALID_OPTIONS;
		}
		*opt_value = 2;
		break;
	default:
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
	}
	return 0;
}

static php_cbor_error uint32_option(uint32_t *opt_value, const char *name, size_t name_len, uint32_t min, uint32_t max, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	zend_long long_value;
	if (value == NULL) {
		return 0;
	}
	ZVAL_DEREF(value);
	if (Z_TYPE_P(value) != IS_LONG) {
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
	}
	long_value = Z_LVAL_P(value);
	if (long_value < min || long_value > max) {
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
	}
	*opt_value = (uint32_t)long_value;
	return 0;
}

php_cbor_error php_cbor_set_encode_options(php_cbor_encode_args *args, HashTable *options)
{
	php_cbor_error error = 0;
	args->max_depth = 64;
	args->string_ref = 0;
	if (options == NULL) {
		return 0;
	}
	CHECK_ERROR(uint32_option(&args->max_depth, ZEND_STRL("max_depth"), 0, 10000, options));
	CHECK_ERROR(bool3_option(&args->string_ref, ZEND_STRL("string_ref"), ZEND_STRL("explicit"), options));
FINALLY:
	return error;
}

php_cbor_error php_cbor_set_decode_options(php_cbor_decode_args *args, HashTable *options)
{
	php_cbor_error error = 0;
	args->max_depth = 64;
	args->max_size = 65536;
	args->string_ref = true;
	if (options == NULL) {
		return 0;
	}
	CHECK_ERROR(uint32_option(&args->max_depth, ZEND_STRL("max_depth"), 0, 10000, options));
	CHECK_ERROR(uint32_option(&args->max_size, ZEND_STRL("max_size"), 0, 0xffffffff, options));
	CHECK_ERROR(bool_option(&args->string_ref, ZEND_STRL("string_ref"), options));
FINALLY:
	return error;
}
