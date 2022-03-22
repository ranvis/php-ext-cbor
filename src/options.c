/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"

#define CHECK_ERROR(e) do { \
		if ((error = (e)) != 0) { \
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

static php_cbor_error bool_n_option(uint8_t *opt_value, const char *name, size_t name_len, char *other_values, HashTable *options)
{
	zval *value = zend_hash_str_find_deref(options, name, name_len);
	if (value == NULL) {
		return 0;
	}
	ZVAL_DEREF(value);
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
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
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
	if (long_value < 0) {
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
	}
	*opt_value = (uint32_t)long_value;
	if (*opt_value < min || *opt_value > max) {
		return PHP_CBOR_ERROR_INVALID_OPTIONS;
	}
	return 0;
}

php_cbor_error php_cbor_set_encode_options(php_cbor_encode_args *args, HashTable *options)
{
	php_cbor_error error = 0;
	args->max_depth = 64;
	args->string_ref = 0;
	args->shared_ref = 0;
	args->datetime = true;
	args->bignum = true;
	args->decimal = true;
	if (options == NULL) {
		return 0;
	}
	CHECK_ERROR(uint32_option(&args->max_depth, ZEND_STRL("max_depth"), 0, 10000, options));
	CHECK_ERROR(bool_n_option(&args->string_ref, ZEND_STRL("string_ref"), "explicit\0", options));
	CHECK_ERROR(bool_n_option(&args->shared_ref, ZEND_STRL("shared_ref"), "-\0unsafe_ref\0", options));
	CHECK_ERROR(bool_option(&args->datetime, ZEND_STRL("datetime"), options));
	CHECK_ERROR(bool_option(&args->bignum, ZEND_STRL("bignum"), options));
	CHECK_ERROR(bool_option(&args->decimal, ZEND_STRL("decimal"), options));
FINALLY:
	return error;
}

php_cbor_error php_cbor_set_decode_options(php_cbor_decode_args *args, HashTable *options)
{
	php_cbor_error error = 0;
	args->max_depth = 64;
	args->max_size = 65536;
	args->string_ref = true;
	args->shared_ref = 0;
	if (options == NULL) {
		return 0;
	}
	CHECK_ERROR(uint32_option(&args->max_depth, ZEND_STRL("max_depth"), 0, 10000, options));
	CHECK_ERROR(uint32_option(&args->max_size, ZEND_STRL("max_size"), 0, 0xffffffff, options));
	CHECK_ERROR(bool_option(&args->string_ref, ZEND_STRL("string_ref"), options));
	CHECK_ERROR(bool_n_option(&args->shared_ref, ZEND_STRL("shared_ref"), "shareable\0unsafe_ref\0", options));
FINALLY:
	return error;
}
