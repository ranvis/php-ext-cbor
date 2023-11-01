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

#define CBOR_CE(name)  php_cbor_##name##_ce

extern zend_class_entry
	/* ce start */
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
	*CBOR_CE(shareable),
	*CBOR_CE(decoder)
	/* ce end */
;

#endif	/* PHP_CBOR_H */
