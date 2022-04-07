#define CBOR_B32A_ISNAN(b32a)  ((binary32.i & 0x7f800000) == 0x7f800000 && (binary32.i & 0x007fffff) != 0) /* isnan(b32a.f) */

typedef union binary32_alias {
	uint32_t i;
	float f;
	struct {
		ZEND_ENDIAN_LOHI_4(
			char c3,
			char c2,
			char c1,
			char c0
		)
	} c;
} binary32_alias;

typedef union binary64_alias {
	uint64_t i;
	double f;
} binary64_alias;

extern void php_cbor_minit_types();

/* undefined */
extern zend_object *cbor_get_undef();

/* xstring */
extern zend_object *cbor_xstring_create(zend_class_entry *ce);
extern void cbor_xstring_set_value(zend_object *obj, zend_string *value);
extern zend_string *cbor_get_xstring_value(zval *value);

/* floatx */
extern zend_object *cbor_floatx_create(zend_class_entry *ce);
extern bool cbor_floatx_set_value(zend_object *obj, zval *value, uint32_t raw);
extern size_t cbor_floatx_get_value(zend_object *obj, char *out);
extern double cbor_from_float16(uint16_t value);
extern uint16_t cbor_to_float16(float value);
