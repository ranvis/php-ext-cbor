/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

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
