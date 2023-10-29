/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "codec.h"
#include <Zend/zend_exceptions.h>
#include <assert.h>

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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|lh!", &value, &flags, &options) != SUCCESS) {
		RETURN_THROWS();
	}
	args.u_flags = (uint32_t)flags;
	if ((error = cbor_set_encode_options(&args, options)) == 0
			&& (error = cbor_check_encode_params(&args)) == 0) {
		error = cbor_encode(value, &str, &args);
	}
	if (error) {
		cbor_throw_error(error, false, &args.error_args);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|lh!", &data, &flags, &options) != SUCCESS) {
		RETURN_THROWS();
	}
	cbor_init_decode_options(&args);
	args.flags = (uint32_t)flags;
	error = cbor_set_decode_options(&args, options);
	if (!error) {
		error = cbor_decode(data, &value, &args);
	}
	cbor_free_decode_options(&args);
	if (error) {
		cbor_throw_error(error, true, &args.error_args);
		RETURN_THROWS();
	}
	RETVAL_COPY_VALUE(&value);
}
/* }}} */

#define DESC_MSG(m)  do { \
		desc_msg = ". " m; \
		goto MSG_SET; \
	} while (0)

void cbor_throw_error(cbor_error error, bool decoding, const cbor_error_args *args)
{
	const char *message = "Unknown error code";
	const char *desc_msg = "";
	bool is_formatted = false;
	bool can_have_offset = true;
	if (error == CBOR_ERROR_EXCEPTION && EG(exception)) {
		return;
	}
	cbor_error error_code = error & CBOR_ERROR_CODE_MASK;
	cbor_error error_desc = (error & CBOR_ERROR_DESC_MASK) >> CBOR_ERROR_DESC_SHIFT;
	switch (error_code) {
	case CBOR_ERROR_INVALID_FLAGS:
		message = "Invalid flags are specified";
		can_have_offset = false;
		switch (error_desc) {
		case CBOR_ERROR_INVALID_FLAGS__BOTH_STRING_FLAG:
			DESC_MSG("CBOR_BYTE and CBOR_TEXT flags cannot be specified simultaneously on encoding");
		case CBOR_ERROR_INVALID_FLAGS__BOTH_KEY_STRING_FLAG:
			DESC_MSG("CBOR_KEY_BYTE and CBOR_KEY_TEXT flags cannot be specified simultaneously on encoding");
		case CBOR_ERROR_INVALID_FLAGS__NO_STRING_FLAG:
			DESC_MSG("Either CBOR_BYTE or CBOR_TEXT flag must be specified to encode string");
		case CBOR_ERROR_INVALID_FLAGS__NO_KEY_STRING_FLAG:
			DESC_MSG("Either CBOR_KEY_BYTE or CBOR_KEY_TEXT flag must be specified to encode string as key");
		case CBOR_ERROR_INVALID_FLAGS__CONTEXTUAL_CDE:
			DESC_MSG("CBOR_CDE flag cannot be used in conjunction with options that makes CBOR data contextual");
		case CBOR_ERROR_INVALID_FLAGS__CLEAR_CDE:
			DESC_MSG("CBOR_CDE flag cannot be cleared during encoding");
		}
		break;
	case CBOR_ERROR_INVALID_OPTIONS:
		message = "Invalid options are specified";
		can_have_offset = false;
		break;
	case CBOR_ERROR_DEPTH:
		message = "Depth limit exceeded";
		break;
	case CBOR_ERROR_RECURSION:
		message = "Recursion is detected";
		can_have_offset = false;
		break;
	case CBOR_ERROR_SYNTAX:
		message = "Data syntax error";
		switch (error_desc) {
		case CBOR_ERROR_SYNTAX__BREAK_UNDERFLOW:
			DESC_MSG("Break without indefinite type");
		case CBOR_ERROR_SYNTAX__BREAK_UNEXPECTED:
			DESC_MSG("Break without indefinite type or it is too early");
		case CBOR_ERROR_SYNTAX__INCONSISTENT_STRING_TYPE:
			DESC_MSG("Inner string type is inconsistent with outer indefinite-length string");
		case CBOR_ERROR_SYNTAX__INDEF_STRING_CHUNK_TYPE:
			DESC_MSG("Indefinite-length string can only contain definite-length strings of the same type as chunks");
		}
		break;
	case CBOR_ERROR_UTF8:
		message = "Invalid UTF-8 sequences";
		break;
	case CBOR_ERROR_UNSUPPORTED_TYPE:
		message = "Unsupported type of data";
		if (!decoding) {
			if (args->u.ce_name) {
				is_formatted = true;
				message = "Encoding of %s is not supported.";
				desc_msg = ZSTR_VAL(args->u.ce_name);
			}
		}
		switch (error_desc) {
		case CBOR_ERROR_UNSUPPORTED_TYPE__SIMPLE:
			DESC_MSG("Unknown simple value");
		}
		break;
	case CBOR_ERROR_UNSUPPORTED_VALUE:
		message = "Unsupported value";
		switch (error_desc) {
		case CBOR_ERROR_UNSUPPORTED_VALUE__INT_RANGE:
			DESC_MSG("Integer is out of range");
		}
		break;
	case CBOR_ERROR_UNSUPPORTED_SIZE:
		message = "Unsupported size of data";
		break;
	case CBOR_ERROR_UNSUPPORTED_KEY_TYPE:
		message = "Unsupported type of data for key";
		switch (error_desc) {
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__INT_KEY:
			DESC_MSG("Integer key while flag CBOR_INT_KEY is not specified");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__NULL:
			DESC_MSG("Null cannot be a map key");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__BOOL:
			DESC_MSG("Bool cannot be a map key");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__FLOAT:
			DESC_MSG("Float cannot be a map key");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__BYTE:
			DESC_MSG("Uncoerced string cannot be a map key. Specify flag CBOR_KEY_BYTE to circumvent this");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__TEXT:
			DESC_MSG("Uncoerced string cannot be a map key. Specify flag CBOR_KEY_TEXT to circumvent this");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__ARRAY:
			DESC_MSG("Array cannot be a map key");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__OBJECT:
			DESC_MSG("Object cannot be a map key");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__UNDEF:
			DESC_MSG("Undefined cannot be a map key");
		case CBOR_ERROR_UNSUPPORTED_KEY_TYPE__TAG:
			DESC_MSG("Tag cannot be a map key");
		}
		break;
	case CBOR_ERROR_UNSUPPORTED_KEY_VALUE:
		message = "Unsupported value for key";
		switch (error_desc) {
		case CBOR_ERROR_UNSUPPORTED_KEY_VALUE__RESERVED_PROP_NAME:
			DESC_MSG("Object property name starting with NUL character is reserved. Specify flag CBOR_MAP_AS_ARRAY and decode map into array to circumvent this");
		}
		break;
	case CBOR_ERROR_UNSUPPORTED_KEY_SIZE:  /* bogus error? */
		message = "Unsupported size of data for key";
		break;
	case CBOR_ERROR_TRUNCATED_DATA:
		message = "Data is truncated";
		break;
	case CBOR_ERROR_MALFORMED_DATA:
		message = "Data is malformed";
		break;
	case CBOR_ERROR_EXTRANEOUS_DATA:
		message = "Extraneous data";
		break;
	case CBOR_ERROR_TAG_SYNTAX:
		message = "The tag cannot be used here";
		switch (error_desc) {
		case CBOR_ERROR_TAG_SYNTAX__STR_REF_NO_NS:
			DESC_MSG("Stringref without stringref-namespace");
		case CBOR_ERROR_TAG_SYNTAX__SHARE_NESTED:
			DESC_MSG("Shareable cannot be nested");
		}
		break;
	case CBOR_ERROR_TAG_TYPE:
		message = "Invalid data type for the tag content";
		switch (error_desc) {
		case CBOR_ERROR_TAG_TYPE__STR_REF_NOT_INT:
			DESC_MSG("Stringref expects integer");
		case CBOR_ERROR_TAG_TYPE__SHARE_INCOMPATIBLE:
			DESC_MSG("Incompatible type is marked as shareable. Specify option ['shared_ref' => 'shareable'] to circumvent this");
		case CBOR_ERROR_TAG_TYPE__SHARE_NOT_INT:
			DESC_MSG("Sharedref expects integer");
		}
		break;
	case CBOR_ERROR_TAG_VALUE:
		message = "Invalid data value for the tag content";
		switch (error_desc) {
		case CBOR_ERROR_TAG_VALUE__STR_REF_RANGE:
			DESC_MSG("Stringref is out of range");
		case CBOR_ERROR_TAG_VALUE__SHARE_SELF:
			DESC_MSG("Shareable cannot have sharedref pointing to itself to create recursion under ['shared_ref' => 'unsafe_ref']. Specify option ['shared_ref' => 'shareable'] to circumvent this");
		case CBOR_ERROR_TAG_VALUE__SHARE_RANGE:
			DESC_MSG("Sharedref is out of range");
		}
		break;
	case CBOR_ERROR_INTERNAL:
		message = "Internal error";
		break;
	}
MSG_SET:
	if (can_have_offset && decoding && args) {
		zend_throw_exception_ex(CBOR_CE(exception), error_code, "%s%s; at offset %zu", message, desc_msg, args->offset);
	} else {
		if (is_formatted) {
			zend_throw_exception_ex(CBOR_CE(exception), error_code, message, desc_msg);
		} else {
			zend_throw_exception_ex(CBOR_CE(exception), error_code, "%s%s.", message, desc_msg);
		}
	}
}
