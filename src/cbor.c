/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include <ext/json/php_json.h>
#include "cbor_arginfo.h"
#include "cbor_ns_arginfo.h"
#include "codec.h"
#include "compatibility.h"
#include "tags.h"
#include "types.h"

#define PHP_CBOR_VERSION "0.3.2"

ZEND_DECLARE_MODULE_GLOBALS(cbor)

/* True global resources - no need for thread safety here */
zend_class_entry
	*CBOR_CE(exception),
	*CBOR_CE(serializable),
	*CBOR_CE(encodeparams),
	*CBOR_CE(undefined),
	*CBOR_CE(xstring),
	*CBOR_CE(byte),
	*CBOR_CE(text),
	*CBOR_CE(floatx),
	*CBOR_CE(float16),
	*CBOR_CE(float32),
	*CBOR_CE(tag),
	*CBOR_CE(shareable)
;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("cbor.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_cbor_globals, cbor_globals)
	STD_PHP_INI_ENTRY("cbor.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_cbor_globals, cbor_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(cbor)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/

	REGISTER_STRING_CONSTANT("CBOR_SELF_DESCRIBE_DATA", CBOR_SELF_DESCRIBE_DATA, CONST_CS | CONST_PERSISTENT);
#define REG_CONST_LONG(name)  REGISTER_LONG_CONSTANT(#name, ##name, CONST_CS | CONST_PERSISTENT)
	/* flags start */
	REG_CONST_LONG(CBOR_SELF_DESCRIBE);
	REG_CONST_LONG(CBOR_BYTE);
	REG_CONST_LONG(CBOR_TEXT);
	REG_CONST_LONG(CBOR_INT_KEY);
	REG_CONST_LONG(CBOR_KEY_BYTE);
	REG_CONST_LONG(CBOR_KEY_TEXT);
	REG_CONST_LONG(CBOR_UNSAFE_TEXT);
	REG_CONST_LONG(CBOR_MAP_AS_ARRAY);
	REG_CONST_LONG(CBOR_MAP_NO_DUP_KEY);
	REG_CONST_LONG(CBOR_FLOAT16);
	REG_CONST_LONG(CBOR_FLOAT32);
	/* flags end */
	/* errors start */
	REG_CONST_LONG(CBOR_ERROR_INVALID_FLAGS);
	REG_CONST_LONG(CBOR_ERROR_INVALID_OPTIONS);
	REG_CONST_LONG(CBOR_ERROR_DEPTH);
	REG_CONST_LONG(CBOR_ERROR_RECURSION);
	REG_CONST_LONG(CBOR_ERROR_SYNTAX);
	REG_CONST_LONG(CBOR_ERROR_UTF8);
	REG_CONST_LONG(CBOR_ERROR_UNSUPPORTED_TYPE);
	REG_CONST_LONG(CBOR_ERROR_UNSUPPORTED_VALUE);
	REG_CONST_LONG(CBOR_ERROR_UNSUPPORTED_SIZE);
	REG_CONST_LONG(CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
	REG_CONST_LONG(CBOR_ERROR_UNSUPPORTED_KEY_VALUE);
	REG_CONST_LONG(CBOR_ERROR_UNSUPPORTED_KEY_SIZE);
	REG_CONST_LONG(CBOR_ERROR_DUPLICATE_KEY);
	REG_CONST_LONG(CBOR_ERROR_TRUNCATED_DATA);
	REG_CONST_LONG(CBOR_ERROR_MALFORMED_DATA);
	REG_CONST_LONG(CBOR_ERROR_EXTRANEOUS_DATA);
	REG_CONST_LONG(CBOR_ERROR_TAG_SYNTAX);
	REG_CONST_LONG(CBOR_ERROR_TAG_TYPE);
	REG_CONST_LONG(CBOR_ERROR_TAG_VALUE);
	REG_CONST_LONG(CBOR_ERROR_INTERNAL);
	/* errors end */

#define REG_CLASS(name, name_cc)  CBOR_CE(name) = register_class_Cbor_##name_cc
	REG_CLASS(exception, Exception)(zend_ce_exception);
	REG_CLASS(serializable, Serializable)();
	REG_CLASS(encodeparams, EncodeParams)();
	REG_CLASS(undefined, Undefined)(php_json_serializable_ce);
	REG_CLASS(xstring, XString)(php_json_serializable_ce);
	REG_CLASS(byte, Byte)(CBOR_CE(xstring));
	REG_CLASS(text, Text)(CBOR_CE(xstring));
	REG_CLASS(floatx, FloatX)(php_json_serializable_ce);
	REG_CLASS(float16, Float16)(CBOR_CE(floatx));
	REG_CLASS(float32, Float32)(CBOR_CE(floatx));
	REG_CLASS(tag, Tag)();
	REG_CLASS(shareable, Shareable)(php_json_serializable_ce);

#define REG_CLASS_CONST_LONG(cls, prefix, name)  zend_declare_class_constant_long(CBOR_CE(cls), ZEND_STRL(#name), prefix##name);
	/* tag constants start */
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, SELF_DESCRIBE);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, STRING_REF_NS);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, STRING_REF);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, SHAREABLE);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, SHARED_REF);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, DATETIME);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, EPOCH);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, BIGNUM_U);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, BIGNUM_N);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, DECIMAL);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, BIGFLOAT);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, TO_BASE64URL);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, TO_BASE64);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, TO_HEX);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, CBOR_DATA);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, URI);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, BASE64URL);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, BASE64);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, PCRE_REGEX);
	REG_CLASS_CONST_LONG(tag, CBOR_TAG_, MIME_MSG);
	/* tag constants end */

	php_cbor_minit_types();
	php_cbor_minit_encode();
	php_cbor_minit_decode();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(cbor)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

#if defined(_MSC_VER)
#pragma warning(disable: 4459)
#endif
/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(cbor)
{
#if defined(_MSC_VER)
#pragma warning(default: 4459)
#endif
	cbor_globals->undef_ins = NULL;
}
/* }}} */

#if 0
/* {{{ PHP_GSHUTDOWN_FUNCTION
 */
static PHP_GSHUTDOWN_FUNCTION(cbor)
{
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(cbor)
{
#if defined(COMPILE_DL_CBOR) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(cbor)
{
	return SUCCESS;
}
/* }}} */
#endif

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(cbor)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "CBOR support", "enabled");
	php_info_print_table_row(2, "Module version", PHP_CBOR_VERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ cbor_module_entry
 */
zend_module_entry cbor_module_entry = {
	STANDARD_MODULE_HEADER,
	"cbor",
	ext_functions,
	PHP_MINIT(cbor),
	PHP_MSHUTDOWN(cbor),
	NULL, /* PHP_RINIT(cbor), */
	NULL, /* PHP_RSHUTDOWN(cbor), */
	PHP_MINFO(cbor),
	PHP_CBOR_VERSION,
	PHP_MODULE_GLOBALS(cbor),
	PHP_GINIT(cbor),
	NULL, /* PHP_GSHUTDOWN(cbor), */
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_CBOR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(cbor)
#endif
