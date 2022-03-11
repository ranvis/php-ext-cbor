/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#ifndef PHP_CBOR_H
#define PHP_CBOR_H

extern zend_module_entry cbor_module_entry;
#define phpext_cbor_ptr &cbor_module_entry

#ifdef PHP_WIN32
#	define PHP_CBOR_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_CBOR_API __attribute__ ((visibility("default")))
#else
#	define PHP_CBOR_API
#endif

#ifdef ZTS
#include <TSRM/TSRM.h>
#endif

ZEND_BEGIN_MODULE_GLOBALS(cbor)
	zend_object *undef_ins;
ZEND_END_MODULE_GLOBALS(cbor)

#define CBOR_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(cbor, v)

ZEND_EXTERN_MODULE_GLOBALS(cbor)

#if defined(ZTS) && defined(COMPILE_DL_CBOR)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_CBOR_H */
