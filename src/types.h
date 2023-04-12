/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define CUSTOM_OBJ(obj_type_t, obj)  ((obj_type_t *)((char *)(obj) - XtOffsetOf(obj_type_t, std)))
#define ZVAL_CUSTOM_OBJ(obj_type_t, zv)  CUSTOM_OBJ(obj_type_t, Z_OBJ_P(zv))

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

typedef uint16_t cbor_fp16i;

void php_cbor_minit_types();

/* undefined */
zend_object *cbor_get_undef();

/* xstring */
zend_object *cbor_xstring_create(zend_class_entry *ce);
void cbor_xstring_set_value(zend_object *obj, zend_string *value);
zend_string *cbor_get_xstring_value(zval *value);

/* floatx */
zend_object *cbor_floatx_create(zend_class_entry *ce);
bool cbor_floatx_set_value(zend_object *obj, zval *value, uint32_t raw);
size_t cbor_floatx_get_value_be(zend_object *obj, char *out);
float cbor_float32_get_value(zend_object *obj);
double cbor_from_fp16i(cbor_fp16i value);
double cbor_from_fp32(float value);
float cbor_to_fp32(double value);
int test_fp32_size(float value);
int test_fp64_size(double value);
void php_cbor_minit_types_float_cast();
cbor_fp16i cbor_float_32_to_16(float value);
cbor_fp16i cbor_float_32_to_16_cast(float value);
cbor_fp16i cbor_float_64_to_16(double value);
cbor_fp16i cbor_float_64_to_16_cast(double value);

/* decoder */
void php_cbor_minit_decoder();
