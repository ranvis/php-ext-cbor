/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"

#define BOOL_OPTION(name)  do { \
		value = zend_hash_str_find_deref(options, ZEND_STRL(#name)); \
		if (value == NULL) { \
			break; \
		} \
		if (Z_TYPE_P(value) == IS_TRUE) { \
			args->name = true; \
		} else if (Z_TYPE_P(value) == IS_FALSE) { \
			args->name = false; \
		} else { \
			return PHP_CBOR_ERROR_INVALID_OPTIONS; \
		} \
	} while (0)

#define LONG_OPTION(name, cast, min, max)  do { \
		value = zend_hash_str_find_deref(options, ZEND_STRL(#name)); \
		if (value == NULL) { \
			break; \
		} \
		if (Z_TYPE_P(value) != IS_LONG) { \
			return PHP_CBOR_ERROR_INVALID_OPTIONS; \
		} \
		long_value = Z_LVAL_P(value); \
		if (long_value < min || long_value > max) { \
			return PHP_CBOR_ERROR_INVALID_OPTIONS; \
		} \
		args->name = cast long_value; \
	} while (0)

php_cbor_error php_cbor_set_encode_options(php_cbor_encode_args *args, HashTable *options)
{
	zval *value;
	zend_long long_value;
	args->max_depth = 64;
	if (options == NULL) {
		return 0;
	}
	LONG_OPTION(max_depth, (uint32_t), 0, 10000);
	return 0;
}

php_cbor_error php_cbor_set_decode_options(php_cbor_decode_args *args, HashTable *options)
{
	zval *value;
	zend_long long_value;
	args->max_depth = 64;
	args->max_size = 65536;
	args->string_ref = true;
	if (options == NULL) {
		return 0;
	}
	LONG_OPTION(max_depth, (uint32_t), 0, 10000);
	LONG_OPTION(max_size, (uint32_t), 0, 0xffffffff);
	BOOL_OPTION(string_ref);
	return 0;
}
