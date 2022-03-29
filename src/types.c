/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"

#define LIT_PROP(prop_literal)  prop_literal, sizeof prop_literal - 1
#define DEF_THIS_PROP(name, prop_literal)  CBOR_CE(name), Z_OBJ_P(ZEND_THIS), LIT_PROP(prop_literal)

typedef struct {
	union floatx_class_v {
		binary32_alias binary32;
		uint16_t binary16;
	} v;
	zend_object std;
} floatx_class;

#define CUSTOM_OBJ(obj_type_t, obj)  ((obj_type_t *)((char *)(obj) - XtOffsetOf(obj_type_t, std)))
#define ZVAL_CUSTOM_OBJ(obj_type_t, zv)  CUSTOM_OBJ(obj_type_t, Z_OBJ_P(zv))

static zend_object_handlers undef_handlers;
static zend_object_handlers xstring_handlers;
static zend_object_handlers floatx_handlers;

/* PHP has IS_UNDEF type, but it is semantically different from CBOR's "undefined" value. */

static zend_object *undef_clone_handler(zend_object *obj)
{
	GC_ADDREF(obj);
	return obj;
}

static zval *undef_read_property(zend_object *obj, zend_string *member, int type, void **cache_slot, zval *rv)
{
	zend_throw_error(NULL, "%s cannot read properties.", ZSTR_VAL(obj->ce->name));
	return &EG(error_zval);
}

static zval *undef_write_property(zend_object *obj, zend_string *member, zval *value, void **cache_slot)
{
	zend_throw_error(NULL, "%s cannot write properties.", ZSTR_VAL(obj->ce->name));
	return &EG(error_zval);
}

static zval *undef_get_property_ptr_ptr(zend_object *obj, zend_string *member, int type, void **cache_slot)
{
	return NULL;
}

static int undef_has_property(zend_object *obj, zend_string *member, int has_set_exists, void **cache_slot)
{
	return 0;
}

static void undef_unset_property(zend_object *obj, zend_string *member, void **cache_slot)
{
	zend_throw_error(NULL, "%s cannot unset properties.", ZSTR_VAL(obj->ce->name));
}

static int undef_cast_object_handler(zend_object *obj, zval *retval, int type)
{
	if (type != _IS_BOOL) {
		return FAILURE;
	}
	ZVAL_FALSE(retval);
	return SUCCESS;
}

PHP_METHOD(Cbor_Undefined, __construct)
{
	/* private constructor */
	zend_throw_error(NULL, "You cannot instantiate %s.", ZSTR_VAL(Z_OBJ_P(ZEND_THIS)->ce->name));
	RETURN_THROWS();
}

PHP_METHOD(Cbor_Undefined, __destruct)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	if (obj == CBOR_G(undef_ins)) {
		CBOR_G(undef_ins) = NULL;
	}
}

zend_object *php_cbor_get_undef()
{
	zend_object *obj = CBOR_G(undef_ins);
	if (!obj) {
		obj = zend_objects_new(CBOR_CE(undefined));
		obj->handlers = &undef_handlers;
		CBOR_G(undef_ins) = obj;
	} else {
		GC_ADDREF(obj);
	}
	return obj;
}

PHP_METHOD(Cbor_Undefined, __set_state)
{
	RETVAL_OBJ(php_cbor_get_undef());
}

PHP_METHOD(Cbor_Undefined, get)
{
	RETVAL_OBJ(php_cbor_get_undef());
}

PHP_METHOD(Cbor_Undefined, jsonSerialize)
{
	RETURN_NULL();
}

static zend_object *xstring_create_object_handler(zend_class_entry *ce)
{
	zend_object *obj = zend_objects_new(ce);
	object_properties_init(obj, ce);
	obj->handlers = &xstring_handlers;
	return obj;
}

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(xstring, prop_literal)

static int xstring_cast_object_handler(zend_object *obj, zval *retval, int type)
{
	zval *value, zv;
	if (type != IS_STRING) {
		return FAILURE;
	}
	value = zend_read_property(obj->ce, obj, LIT_PROP("value"), false, &zv);
	ZVAL_COPY(retval, value);
	return SUCCESS;
}

PHP_METHOD(Cbor_XString, __construct)
{
	zend_string *value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &value) == FAILURE) {
		RETURN_THROWS();
	}
	zend_update_property_str(THIS_PROP("value"), value);
}

zend_string *php_cbor_get_xstring_value(zval *ins)
{
	zval *value, zv;
	value = zend_read_property(Z_OBJ_P(ins)->ce, Z_OBJ_P(ins), LIT_PROP("value"), false, &zv);
	Z_TRY_ADDREF_P(value);
	return Z_STR_P(value);
}

PHP_METHOD(Cbor_XString, jsonSerialize)
{
	RETURN_STR(php_cbor_get_xstring_value(ZEND_THIS));
}

#undef THIS_PROP

zend_object *php_cbor_floatx_create(zend_class_entry *ce)
{
	floatx_class *base = zend_object_alloc(sizeof(floatx_class), ce);
	zend_object_std_init(&base->std, ce);
	base->std.handlers = &floatx_handlers;
	return &base->std;
}

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(floatx, prop_literal)

static double floatx_read_p_value(zend_object *obj)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	double value;
	if (obj->ce == CBOR_CE(float32)) {
		value = (double)base->v.binary32.f;
	} else {
		value = php_cbor_from_float16(base->v.binary16);
	}
	return value;
}

static int floatx_cast_object_handler(zend_object *obj, zval *retval, int type)
{
	if (type != IS_DOUBLE) {
		/* IS_STRING cast may not be necessarily desirable. */
		return FAILURE;
	}
	ZVAL_DOUBLE(retval, floatx_read_p_value(obj));
	return SUCCESS;
}

static bool test_floatx_class(zend_class_entry *ce, zend_function *f)
{
	if (ce == CBOR_CE(float16) || ce == CBOR_CE(float32)) {
		return true;
	}
	zend_throw_error(NULL, "FloatX::%s cannot be called on custom type.", ZSTR_VAL(f->common.function_name));
	return false;
}

#define TEST_FLOATX_CLASS(ce)  do { \
		if (!test_floatx_class(ce, EX(func))) { \
			RETURN_THROWS(); \
		} \
	} while (0)

PHP_METHOD(Cbor_FloatX, __construct)
{
	zval value;
	double num;
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	TEST_FLOATX_CLASS(obj->ce);
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "d", &num) == FAILURE) {
		RETURN_THROWS();
	}
	ZVAL_DOUBLE(&value, num);
	if (!php_cbor_floatx_set_value(obj, &value, NULL)) {
		RETURN_THROWS();
	}
}

PHP_METHOD(Cbor_FloatX, fromBinary)
{
	zend_class_entry *ctx_ce = zend_get_called_scope(execute_data);
	zend_object *obj;
	zend_string *str;
	zval value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &str) == FAILURE) {
		RETURN_THROWS();
	}
	TEST_FLOATX_CLASS(ctx_ce);
	obj = php_cbor_floatx_create(ctx_ce);
	ZVAL_STR(&value, str);
	if (!php_cbor_floatx_set_value(obj, &value, NULL)) {
		zend_objects_destroy_object(obj);
		RETURN_THROWS();
	}
	RETURN_OBJ(obj);
}

static bool floatx_restore(zend_object *obj, HashTable *ht)
{
	zval *value;
	value = zend_hash_index_find(ht, 0);
	if (!value || Z_TYPE_P(value) != IS_STRING
			|| !php_cbor_floatx_set_value(obj, value, NULL)) {
		zend_throw_error(NULL, "Unable to restore %s.", ZSTR_VAL(obj->ce->name));
		return false;
	}
	return true;
}

PHP_METHOD(Cbor_FloatX, __set_state)
{
	zend_class_entry *ctx_ce = zend_get_called_scope(execute_data);
	zend_object *obj;
	HashTable *ht;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &ht) == FAILURE) {
		RETURN_THROWS();
	}
	TEST_FLOATX_CLASS(ctx_ce);
	obj = php_cbor_floatx_create(ctx_ce);
	if (!floatx_restore(obj, ht)) {
		zend_objects_destroy_object(obj);
		RETURN_THROWS();
	}
	RETURN_OBJ(obj);
}

PHP_METHOD(Cbor_FloatX, __unserialize)
{
	zend_class_entry *ctx_ce = zend_get_called_scope(execute_data);
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	HashTable *ht;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &ht) == FAILURE) {
		RETURN_THROWS();
	}
	TEST_FLOATX_CLASS(ctx_ce);
	if (!floatx_restore(obj, ht)) {
		RETURN_THROWS();
	}
}

PHP_METHOD(Cbor_FloatX, jsonSerialize)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	TEST_FLOATX_CLASS(obj->ce);
	RETURN_DOUBLE(floatx_read_p_value(obj));
}

bool php_cbor_floatx_set_value(zend_object *obj, zval *value, const char *raw)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	if (value) {
		int type = Z_TYPE_P(value);
		if (type == IS_DOUBLE || type == IS_LONG) {
			float f_value;
			if (type == IS_DOUBLE) {
				f_value = (float)Z_DVAL_P(value);
			} else {  /* from write_property */
				f_value = (float)Z_LVAL_P(value);
			}
			if (base->std.ce == CBOR_CE(float32)) {
				base->v.binary32.f = f_value;
			} else {
				base->v.binary16 = php_cbor_to_float16(f_value);
			}
			return true;
		} else if (type == IS_STRING) {
			raw = Z_STRVAL_P(value);
			size_t req_len;
			req_len = (base->std.ce == CBOR_CE(float32)) ? 4 : 2;
			if (Z_STRLEN_P(value) != req_len) {
				zend_value_error("Unexpected value length.");
				return false;
			}
		} else {
			zend_type_error("Unexpected value type.");
			return false;
		}
	}
	if (base->std.ce == CBOR_CE(float32)) {
		base->v.binary32.c.c0 = raw[0];
		base->v.binary32.c.c1 = raw[1];
		base->v.binary32.c.c2 = raw[2];
		base->v.binary32.c.c3 = raw[3];
	} else {
		uint8_t *ptr = (uint8_t *)raw;
		base->v.binary16 = (ptr[0] << 8) | ptr[1];
	}
	return true;
}

static zend_object *floatx_clone_handler(zend_object *obj)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	zend_object *new_obj = php_cbor_floatx_create(obj->ce);
	floatx_class *new_base = CUSTOM_OBJ(floatx_class, new_obj);
	new_base->v = base->v;
	zend_objects_clone_members(new_obj, obj);
	return new_obj;
}

static zval *floatx_read_property(zend_object *obj, zend_string *member, int type, void **cache_slot, zval *rv)
{
	if (zend_string_equals_literal(member, "value")) {
		ZVAL_DOUBLE(rv, floatx_read_p_value(obj));
	} else {
		rv = zend_std_read_property(obj, member, type, cache_slot, rv);
	}
	return rv;
}

static zval *floatx_write_property(zend_object *obj, zend_string *member, zval *value, void **cache_slot)
{
	if (zend_string_equals_literal(member, "value")) {
		php_cbor_floatx_set_value(obj, value, NULL);
	} else {
		value = zend_std_write_property(obj, member, value, cache_slot);
	}
	return value;
}

static zval *floatx_get_property_ptr_ptr(zend_object *obj, zend_string *member, int type, void **cache_slot)
{
	if (zend_string_equals_literal(member, "value")) {
		return NULL;
	}
	return zend_std_get_property_ptr_ptr(obj, member, type, cache_slot);
}

static int floatx_has_property(zend_object *obj, zend_string *member, int has_set_exists, void **cache_slot)
{
	if (zend_string_equals_literal(member, "value")) {
		if (has_set_exists == ZEND_PROPERTY_NOT_EMPTY) {
			zval *value, tmp_v;
			value = floatx_read_property(obj, member, BP_VAR_R, cache_slot, &tmp_v);
			return Z_DVAL_P(value) != 0;
		}
		return 1; /* ZEND_PROPERTY_ISSET, ZEND_PROPERTY_EXISTS */
	}
	return zend_std_has_property(obj, member, has_set_exists, cache_slot);
}

static void floatx_unset_property(zend_object *obj, zend_string *member, void **cache_slot)
{
	if (zend_string_equals_literal(member, "value")) {
		zend_throw_error(NULL, "The property '%s' cannot be unset.", ZSTR_VAL(member));
		return;
	}
	zend_std_unset_property(obj, member, cache_slot);
}

size_t php_cbor_floatx_get_value(zend_object *obj, char *out)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	if (obj->ce == CBOR_CE(float32)) {
		out[0] = base->v.binary32.c.c0;
		out[1] = base->v.binary32.c.c1;
		out[2] = base->v.binary32.c.c2;
		out[3] = base->v.binary32.c.c3;
		return 4;
	}
	out[0] = (char)(base->v.binary16 >> 8);
	out[1] = (char)(base->v.binary16 & 0xff);
	return 2;
}

static zend_array *floatx_get_properties_for(zend_object *obj, zend_prop_purpose purpose)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	zend_array *props;
	zval zv;
	bool decode = false;
	switch (purpose) {
	case ZEND_PROP_PURPOSE_DEBUG:
	case ZEND_PROP_PURPOSE_ARRAY_CAST:
		decode = true;
		break;
	case ZEND_PROP_PURPOSE_SERIALIZE:
	case ZEND_PROP_PURPOSE_VAR_EXPORT:
		break;
	default:
		return zend_std_get_properties_for(obj, purpose);
	}
	/* While floatX can have custom properties for now, they are not dumped or restored. */
	props = zend_new_array(1);
	if (decode) {
		double value;
		if (obj->ce == CBOR_CE(float32)) {
			value = (double)base->v.binary32.f;
		} else {
			value = php_cbor_from_float16(base->v.binary16);
		}
		ZVAL_DOUBLE(&zv, value);
		zend_hash_str_add_new(props, ZEND_STRL("value"), &zv);
	} else {
		char bin[4];
		size_t len = php_cbor_floatx_get_value(obj, bin);
		ZVAL_STRINGL(&zv, bin, len);
		zend_hash_next_index_insert(props, &zv);
	}
	return props;
}

double php_cbor_from_float16(uint16_t value)
{
	/* Based on RFC 8949 code */
	binary64_alias bin64;
	uint16_t exp = (value >> 10) & 0x1f;  /* 0b11111 */
	uint16_t frac = value & 0x3ff;  /* 0b11_1111_1111 */
	if (exp == 0x00) {  /* 0b00000 */
		bin64.f = ldexp(frac, -24);
	} else if (exp != 0x1f) {
		bin64.f = ldexp(frac + 1024, exp - 14 - 11);
	} else if (frac == 0) {  /* exp = 0b11111 */
		bin64.f = INFINITY;
	} else {  /* NaN */
		bin64.i = 0
			| (0x7ffULL << 52)  /* exp, 0b111_1111_1111 */
			| ((uint64_t)frac << 42)  /* fraction, 0b11_1111_1111 */
		;
	}
	bin64.i |= (value & 0x8000ULL) << 48;  /* sign */
	return bin64.f;
}

uint16_t php_cbor_to_float16(float value)
{
	binary32_alias binary32;
	uint16_t binary16 = 0;
	binary32.f = (float)value;
	if (UNEXPECTED(CBOR_B32A_ISNAN(binary32))) {
		binary16 = 0x7e00  /* qNaN (or sNaN may become INF), 0b0_11111_10_0000_0000 */
			| (uint16_t)((binary32.i & 0x80000000) >> 16)  /* sign */
			| (uint16_t)((binary32.i & (0x03ff << 13)) >> 13);  /* high 10-bit fract, 0b11_1111_1111 << 13 */
	} else {
		uint32_t exp = ((binary32.i & (0xff << 23)) >> 23);  /* 0b0_1111_1111 << 23 */
		if (exp < 127 - 24) {  /* Too small */
			binary16 = (uint16_t)((binary32.i & 0x80000000) >> 16);  /* sign */
			/* For exp = 127 - 25, think this does round to even to avoid negative shift below */
		} else if (exp < 127 - 14) {  /* binary32: 8bits; binary16: 5bits, with rounding */
			uint32_t frac = binary32.i & 0x7fffff; /* 0b111_1111_1111_1111_1111_1111 */;
			binary16 = (uint16_t)(0
				| ((binary32.i & 0x80000000) >> 16)  /* sign */
				| (1 << (exp - 127 + 24))  /* exp to fract; << 0..9 */
				| (((frac >> (127 - exp - 2)) + 1) >> 1)  /* Round half away from zero for simplicity; >> 22..13 */
			);
		} else if (exp < 127 + 16) {
			uint32_t frac = binary32.i & 0x7fffff; /* 0b111_1111_1111_1111_1111_1111 */;
			binary16 = (uint16_t)(0
				| ((binary32.i & 0x80000000) >> 16)  /* sign */
				| ((exp - 112) << 10)  /* exp, 113..142 > 1..29 */
				| (((frac >> 12) + 1) >> 1)  /* Round half away from zero for simplicity */
			);
		} else {
			binary16 = (uint16_t)(0
				| ((binary32.i & 0x80000000) >> 16)  /* sign */
				| (0x1f << 10) /* exp, 0b11111 */
			);
		}
	}
	return binary16;
}

#undef THIS_PROP
#define THIS_PROP(prop_literal)  DEF_THIS_PROP(tag, prop_literal)

PHP_METHOD(Cbor_Tag, __construct)
{
	zend_long value;
	zval *content;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &value, &content) == FAILURE) {
		RETURN_THROWS();
	}
	zend_update_property_long(THIS_PROP("tag"), value);
	zend_update_property(THIS_PROP("content"), content);
}

#undef THIS_PROP
#define THIS_PROP(prop_literal)  DEF_THIS_PROP(shareable, prop_literal)

PHP_METHOD(Cbor_Shareable, __construct)
{
	zval *value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &value) == FAILURE) {
		RETURN_THROWS();
	}
	zend_update_property(THIS_PROP("value"), value);
}

PHP_METHOD(Cbor_Shareable, jsonSerialize)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	zval *value, zv;
	value = zend_read_property(obj->ce, obj, LIT_PROP("value"), false, &zv);
	RETURN_COPY(value);
}

#undef THIS_PROP

void php_cbor_minit_types()
{
	memcpy(&undef_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	undef_handlers.clone_obj = &undef_clone_handler;
	undef_handlers.read_property = &undef_read_property;
	undef_handlers.write_property = &undef_write_property;
	undef_handlers.get_property_ptr_ptr = &undef_get_property_ptr_ptr;
	undef_handlers.has_property = &undef_has_property;
	undef_handlers.unset_property = &undef_unset_property;
	undef_handlers.cast_object = &undef_cast_object_handler;

	/* Setting CBOR_CE(xstring)->create_object does not help. */
	CBOR_CE(byte)->create_object = &xstring_create_object_handler;
	CBOR_CE(text)->create_object = &xstring_create_object_handler;
	memcpy(&xstring_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	xstring_handlers.cast_object = &xstring_cast_object_handler;

	CBOR_CE(float16)->create_object = &php_cbor_floatx_create;
	CBOR_CE(float32)->create_object = &php_cbor_floatx_create;
	memcpy(&floatx_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	floatx_handlers.offset = XtOffsetOf(floatx_class, std);
	floatx_handlers.clone_obj = &floatx_clone_handler;
	floatx_handlers.read_property = &floatx_read_property;
	floatx_handlers.write_property = &floatx_write_property;
	floatx_handlers.get_property_ptr_ptr = &floatx_get_property_ptr_ptr;
	floatx_handlers.has_property = &floatx_has_property;
	floatx_handlers.unset_property = &floatx_unset_property;
	floatx_handlers.cast_object = &floatx_cast_object_handler;
	floatx_handlers.get_properties_for = &floatx_get_properties_for;
}
