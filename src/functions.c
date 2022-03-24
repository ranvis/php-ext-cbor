/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"
#include <Zend/zend_smart_string.h>

static void throw_error(php_cbor_error error, bool has_arg, size_t arg);

/* {{{ proto string cbor_encode(mixed $value, int $flags = CBOR_BYTE, ?array $options = [...])
   Return a CBOR encoded string of a value. */
PHP_FUNCTION(cbor_encode)
{
	zval *value;
	zend_long flags = PHP_CBOR_BYTE | PHP_CBOR_KEY_BYTE;
	HashTable *options = NULL;
	zend_string *str = NULL;
	php_cbor_error error;
	php_cbor_encode_args args;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|lh", &value, &flags, &options) == FAILURE) {
		RETURN_THROWS();
	}
	args.flags = (int)flags;
	error = php_cbor_set_encode_options(&args, options);
	if (!error) {
		error = php_cbor_encode(value, &str, &args);
	}
	if (error) {
		if (error != PHP_CBOR_ERROR_EXCEPTION) {
			throw_error(error, false, 0);
		}
		RETURN_THROWS();
	}
	assert(str);
	RETURN_STR(str);
}
/* }}} */


/* {{{ proto mixed cbor_decode(string $data, int $flags = CBOR_BYTE, ?array $options = [...])
   Decode a CBOR encoded string. */
PHP_FUNCTION(cbor_decode)
{
	zend_string *data;
	zend_long flags = PHP_CBOR_BYTE | PHP_CBOR_KEY_BYTE;
	HashTable *options = NULL;
	zval value;
	php_cbor_error error;
	php_cbor_decode_args args;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|lh", &data, &flags, &options) == FAILURE) {
		RETURN_THROWS();
	}
	args.flags = (int)flags;
	error = php_cbor_set_decode_options(&args, options);
	if (!error) {
		error = php_cbor_decode(data, &value, &args);
	}
	if (error) {
		if (error != PHP_CBOR_ERROR_EXCEPTION) {
			throw_error(error, true, args.error_arg);
		}
		RETURN_THROWS();
	}
	RETVAL_COPY_VALUE(&value);
}
/* }}} */

static void throw_error(php_cbor_error error, bool has_arg, size_t arg)
{
	const char *message = "Unknown error code.";
	bool can_have_arg = true;
	switch (error) {
	case PHP_CBOR_ERROR_INVALID_FLAGS:
		message = "Invalid flags are specified.";
		can_have_arg = false;
		break;
	case PHP_CBOR_ERROR_INVALID_OPTIONS:
		message = "Invalid options are specified.";
		can_have_arg = false;
		break;
	case PHP_CBOR_ERROR_DEPTH:
		message = "Depth limit exceeded.";
		break;
	case PHP_CBOR_ERROR_RECURSION:
		message = "Recursion is detected.";
		can_have_arg = false;
		break;
	case PHP_CBOR_ERROR_SYNTAX:
		message = "Data syntax error.";
		break;
	case PHP_CBOR_ERROR_UTF8:
		message = "Invalid UTF-8 sequences.";
		break;
	case PHP_CBOR_ERROR_UNSUPPORTED_TYPE:
		message = "Unsupported type of data.";
		break;
	case PHP_CBOR_ERROR_UNSUPPORTED_VALUE:
		message = "Unsupported value.";
		break;
	case PHP_CBOR_ERROR_UNSUPPORTED_SIZE:
		message = "Unsupported size of data.";
		break;
	case PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE:
		message = "Unsupported type of data for key.";
		break;
	case PHP_CBOR_ERROR_UNSUPPORTED_KEY_VALUE:
		message = "Unsupported value for key.";
		break;
	case PHP_CBOR_ERROR_UNSUPPORTED_KEY_SIZE:
		message = "Unsupported size of data for key.";
		break;
	case PHP_CBOR_ERROR_TRUNCATED_DATA:
		message = "Data is truncated.";
		break;
	case PHP_CBOR_ERROR_MALFORMED_DATA:
		message = "Data is malformed.";
		break;
	case PHP_CBOR_ERROR_EXTRANEOUS_DATA:
		message = "Extraneous data.";
		break;
	case PHP_CBOR_ERROR_TAG_SYNTAX:
		message = "The tag cannot be used here.";
		break;
	case PHP_CBOR_ERROR_TAG_TYPE:
		message = "Invalid data type for the tag content.";
		break;
	case PHP_CBOR_ERROR_TAG_VALUE:
		message = "Invalid data value for the tag content.";
		break;
	case PHP_CBOR_ERROR_INTERNAL:
		message = "Internal error.";
		break;
	}
	if (can_have_arg && has_arg) {
		smart_string fmt_msg = {0};
		smart_string_appendl(&fmt_msg, message, strlen(message) - 1);
		smart_string_appends(&fmt_msg, " at offset %zu.");
		smart_string_0(&fmt_msg);
		zend_throw_exception_ex(CBOR_CE(exception), error, fmt_msg.c, arg);
		smart_string_free(&fmt_msg);
	} else {
		zend_throw_exception(CBOR_CE(exception), message, error);
	}
}
