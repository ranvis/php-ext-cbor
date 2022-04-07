/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "codec.h"

/* {{{ proto string cbor_encode(mixed $value, int $flags = CBOR_BYTE, ?array $options = [...])
   Return a CBOR encoded string of a value. */
PHP_FUNCTION(cbor_encode)
{
	zval *value;
	zend_long flags = CBOR_BYTE | CBOR_KEY_BYTE;
	HashTable *options = NULL;
	zend_string *str = NULL;
	cbor_error error;
	cbor_encode_args args;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|lh", &value, &flags, &options) == FAILURE) {
		RETURN_THROWS();
	}
	args.flags = (uint32_t)flags;
	error = php_cbor_set_encode_options(&args, options);
	if (!error) {
		error = php_cbor_encode(value, &str, &args);
	}
	if (error) {
		if (error != CBOR_ERROR_EXCEPTION) {
			php_cbor_throw_error(error, false, 0);
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
	zend_long flags = CBOR_BYTE | CBOR_KEY_BYTE;
	HashTable *options = NULL;
	zval value;
	cbor_error error;
	cbor_decode_args args;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|lh", &data, &flags, &options) == FAILURE) {
		RETURN_THROWS();
	}
	args.flags = (uint32_t)flags;
	error = php_cbor_set_decode_options(&args, options);
	if (!error) {
		error = php_cbor_decode(data, &value, &args);
	}
	if (error) {
		if (error != CBOR_ERROR_EXCEPTION) {
			php_cbor_throw_error(error, true, args.error_arg);
		}
		RETURN_THROWS();
	}
	RETVAL_COPY_VALUE(&value);
}
/* }}} */

void php_cbor_throw_error(cbor_error error, bool has_arg, size_t arg)
{
	const char *message = "Unknown error code.";
	bool can_have_arg = true;
	switch (error) {
	case CBOR_ERROR_INVALID_FLAGS:
		message = "Invalid flags are specified.";
		can_have_arg = false;
		break;
	case CBOR_ERROR_INVALID_OPTIONS:
		message = "Invalid options are specified.";
		can_have_arg = false;
		break;
	case CBOR_ERROR_DEPTH:
		message = "Depth limit exceeded.";
		break;
	case CBOR_ERROR_RECURSION:
		message = "Recursion is detected.";
		can_have_arg = false;
		break;
	case CBOR_ERROR_SYNTAX:
		message = "Data syntax error.";
		break;
	case CBOR_ERROR_UTF8:
		message = "Invalid UTF-8 sequences.";
		break;
	case CBOR_ERROR_UNSUPPORTED_TYPE:
		message = "Unsupported type of data.";
		break;
	case CBOR_ERROR_UNSUPPORTED_VALUE:
		message = "Unsupported value.";
		break;
	case CBOR_ERROR_UNSUPPORTED_SIZE:
		message = "Unsupported size of data.";
		break;
	case CBOR_ERROR_UNSUPPORTED_KEY_TYPE:
		message = "Unsupported type of data for key.";
		break;
	case CBOR_ERROR_UNSUPPORTED_KEY_VALUE:
		message = "Unsupported value for key.";
		break;
	case CBOR_ERROR_UNSUPPORTED_KEY_SIZE:
		message = "Unsupported size of data for key.";
		break;
	case CBOR_ERROR_TRUNCATED_DATA:
		message = "Data is truncated.";
		break;
	case CBOR_ERROR_MALFORMED_DATA:
		message = "Data is malformed.";
		break;
	case CBOR_ERROR_EXTRANEOUS_DATA:
		message = "Extraneous data.";
		break;
	case CBOR_ERROR_TAG_SYNTAX:
		message = "The tag cannot be used here.";
		break;
	case CBOR_ERROR_TAG_TYPE:
		message = "Invalid data type for the tag content.";
		break;
	case CBOR_ERROR_TAG_VALUE:
		message = "Invalid data value for the tag content.";
		break;
	case CBOR_ERROR_INTERNAL:
		message = "Internal error.";
		break;
	}
	if (can_have_arg && has_arg) {
		zend_throw_exception_ex(CBOR_CE(exception), error, "%s at offset %zu.", message, arg);
	} else {
		zend_throw_exception(CBOR_CE(exception), message, error);
	}
}
