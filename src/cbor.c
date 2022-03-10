/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "php_cbor.h"
#include "cbor_arginfo.h"
#include "cbor_ns_arginfo.h"
#include "compatibility.h"
#include "tags.h"

#if CBOR_MAJOR_VERSION == 0 && CBOR_MINOR_VERSION < 9
#error "libcbor version must be 0.9 or later."
#endif

#define PHP_CBOR_VERSION "0.2.0a1"

ZEND_DECLARE_MODULE_GLOBALS(cbor)

/* True global resources - no need for thread safety here */
zend_class_entry
	*CBOR_CE(exception),
	*CBOR_CE(encodable),
	*CBOR_CE(undefined),
	*CBOR_CE(xstring),
	*CBOR_CE(byte),
	*CBOR_CE(text),
	*CBOR_CE(floatx),
	*CBOR_CE(float16),
	*CBOR_CE(float32),
	*CBOR_CE(tag)
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


/* {{{ php_cbor_init_globals
 */
static void php_cbor_init_globals(zend_cbor_globals *cbor_globals)
{
	cbor_globals->undef_ins = NULL;
}
/* }}} */

static void *php_cbor_malloc(size_t size)
{
	return emalloc(size);
}

static void *php_cbor_realloc(void *ptr, size_t size)
{
	return erealloc(ptr, size);
}

static void php_cbor_free(void *ptr)
{
	efree(ptr);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(cbor)
{
	ZEND_INIT_MODULE_GLOBALS(cbor, php_cbor_init_globals, NULL);
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/

	REGISTER_STRING_CONSTANT("CBOR_SELF_DESCRIBE_DATA", PHP_CBOR_SELF_DESCRIBE_DATA, CONST_CS | CONST_PERSISTENT);
	#define REG_CONST_LONG(name)  REGISTER_LONG_CONSTANT(#name, PHP_##name, CONST_CS | CONST_PERSISTENT)
	/* flags start */
	REG_CONST_LONG(CBOR_SELF_DESCRIBE);
	REG_CONST_LONG(CBOR_BYTE);
	REG_CONST_LONG(CBOR_TEXT);
	REG_CONST_LONG(CBOR_INT_KEY);
	REG_CONST_LONG(CBOR_KEY_BYTE);
	REG_CONST_LONG(CBOR_KEY_TEXT);
	REG_CONST_LONG(CBOR_MAP_AS_ARRAY);
	REG_CONST_LONG(CBOR_UNSAFE_TEXT);
	REG_CONST_LONG(CBOR_FLOAT16);
	REG_CONST_LONG(CBOR_FLOAT32);
	REG_CONST_LONG(CBOR_COMPACT_FLOAT);
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
	REG_CONST_LONG(CBOR_ERROR_TRUNCATED_DATA);
	REG_CONST_LONG(CBOR_ERROR_MALFORMED_DATA);
	REG_CONST_LONG(CBOR_ERROR_EXTRANEOUS_DATA);
	REG_CONST_LONG(CBOR_ERROR_INTERNAL);
	/* errors end */

	#define REG_CLASS(name, name_cc)  CBOR_CE(name) = register_class_Cbor_##name_cc
	REG_CLASS(exception, Exception)(zend_ce_exception);
	REG_CLASS(encodable, Encodable)();
	REG_CLASS(undefined, Undefined)();
	REG_CLASS(xstring, XString)();
	REG_CLASS(byte, Byte)(CBOR_CE(xstring));
	REG_CLASS(text, Text)(CBOR_CE(xstring));
	REG_CLASS(floatx, FloatX)();
	REG_CLASS(float16, Float16)(CBOR_CE(floatx));
	REG_CLASS(float32, Float32)(CBOR_CE(floatx));
	REG_CLASS(tag, Tag)();

	#define REG_CLASS_CONST_LONG(cls, prefix, name)  zend_declare_class_constant_long(CBOR_CE(cls), ZEND_STRL(#name), prefix##name);
	/* tag constants start */
	REG_CLASS_CONST_LONG(tag, PHP_CBOR_TAG_, SELF_DESCRIBE);
	/* tag constants end */

	php_cbor_minit_types();
	php_cbor_minit_encode();
	php_cbor_minit_decode();

	#if CBOR_CUSTOM_ALLOC
	cbor_set_allocs(php_cbor_malloc, php_cbor_realloc, php_cbor_free);
	#endif

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

#if 0
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

#endif
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(cbor)
{
	if (CBOR_G(undef_ins)) {
		OBJ_RELEASE(CBOR_G(undef_ins));
		CBOR_G(undef_ins) = NULL;
	}
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(cbor)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "cbor support", "enabled");
	php_info_print_table_row(2, "Module version", PHP_CBOR_VERSION);
	php_info_print_table_row(2, "libcbor version", CBOR_VERSION);
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
	PHP_RSHUTDOWN(cbor),
	PHP_MINFO(cbor),
	PHP_CBOR_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_CBOR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(cbor)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
