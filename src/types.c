/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "types.h"

#define DEF_THIS_PROP(name, prop_literal)  CBOR_CE(name), Z_OBJ_P(ZEND_THIS), ZEND_STRL(prop_literal)

typedef struct {
	zend_string *str;
	zend_object std;
} xstring_class;

typedef struct {
	union floatx_class_v {
		binary32_alias binary32;
		uint16_t binary16;
	} v;
	zend_object std;
} floatx_class;

static zend_object_handlers undef_handlers;
static zend_object_handlers xstring_handlers;
static zend_object_handlers floatx_handlers;

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(tag, prop_literal)

PHP_METHOD(Cbor_EncodeParams, __construct)
{
	zval *value, *params;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "za", &value, &params) != SUCCESS) {
		RETURN_THROWS();
	}
	zend_update_property(THIS_PROP("value"), value);
	zend_update_property(THIS_PROP("params"), params);
}

#undef THIS_PROP

/* PHP has IS_UNDEF type, but it is semantically different from CBOR's "undefined" value. */

static int undef_serialize(zval *obj, unsigned char **buffer, size_t *buf_len, zend_serialize_data *data)
{
	*buffer = emalloc(0);  /* allocate just in case */
	*buf_len = 0;
	return SUCCESS;
}

static int undef_unserialize(zval *obj, zend_class_entry *ce, const unsigned char *buf, size_t buf_len, zend_unserialize_data *data)
{
	if (buf_len != 0) {
		zend_throw_error(NULL, "Unexpected data for %s", ZSTR_VAL(ce->name));
		return FAILURE;
	}
	ZVAL_OBJ(obj, cbor_get_undef());
	return SUCCESS;
}

static zend_object *undef_clone(zend_object *obj)
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

static int undef_cast(zend_object *obj, zval *retval, int type)
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

zend_object *cbor_get_undef()
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
	RETVAL_OBJ(cbor_get_undef());
}

PHP_METHOD(Cbor_Undefined, get)
{
	zend_parse_parameters_none();
	RETVAL_OBJ(cbor_get_undef());
}

PHP_METHOD(Cbor_Undefined, jsonSerialize)
{
	zend_parse_parameters_none();
	RETURN_NULL();
}

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(xstring, prop_literal)

zend_object *cbor_xstring_create(zend_class_entry *ce)
{
	xstring_class *base = zend_object_alloc(sizeof(xstring_class), ce);
	zend_object_std_init(&base->std, ce);
	base->str = zend_empty_string;
	base->std.handlers = &xstring_handlers;
	return &base->std;
}

void cbor_xstring_set_value(zend_object *obj, zend_string *value)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	zend_string_release(base->str);
	base->str = value;
	zend_string_addref(base->str);
}

static void xstring_free(zend_object *obj)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	zend_string_release(base->str);
	zend_object_std_dtor(obj);
}

static zend_object *xstring_clone(zend_object *obj)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	zend_object *new_obj = cbor_xstring_create(obj->ce);
	xstring_class *new_base = CUSTOM_OBJ(xstring_class, new_obj);
	new_base->str = base->str;
	zend_string_addref(new_base->str);
	return new_obj;
}

static zval *xstring_read_property(zend_object *obj, zend_string *member, int type, void **cache_slot, zval *rv)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	if (zend_string_equals_literal(member, "value")) {
		ZVAL_STR_COPY(rv, base->str);
	} else {
		zend_throw_error(NULL, "The custom property cannot be used.");
		ZVAL_ERROR(rv);
	}
	return rv;
}

static zval *xstring_write_property(zend_object *obj, zend_string *member, zval *value, void **cache_slot)
{
	if (zend_string_equals_literal(member, "value")) {
		if (Z_TYPE_P(value) != IS_STRING) {
			zend_type_error("The value property only accepts string.");
			ZVAL_ERROR(value);
		} else {
			cbor_xstring_set_value(obj, Z_STR_P(value));
		}
	} else {
		zend_throw_error(NULL, "The custom property cannot be used.");
		ZVAL_ERROR(value);
	}
	return value;
}

static zval *xstring_get_property_ptr_ptr(zend_object *obj, zend_string *member, int type, void **cache_slot)
{
	return NULL;
}

static int xstring_has_property(zend_object *obj, zend_string *member, int has_set_exists, void **cache_slot)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	if (zend_string_equals_literal(member, "value")) {
		if (has_set_exists == ZEND_PROPERTY_NOT_EMPTY) {
			zval value;
			ZVAL_STR(&value, base->str);
			return zend_is_true(&value);
		}
		return 1; /* ZEND_PROPERTY_ISSET, ZEND_PROPERTY_EXISTS */
	}
	return 0;
}

static void xstring_unset_property(zend_object *obj, zend_string *member, void **cache_slot)
{
	zend_throw_error(NULL, "The property cannot be unset.");
}

static int xstring_cast(zend_object *obj, zval *retval, int type)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	if (type != IS_STRING) {
		return FAILURE;
	}
	ZVAL_STR_COPY(retval, base->str);
	return SUCCESS;
}

static zend_array *xstring_get_properties_for(zend_object *obj, zend_prop_purpose purpose)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	zend_array *props;
	zval value;
	bool view = false;
	switch (purpose) {
	case ZEND_PROP_PURPOSE_DEBUG:
	case ZEND_PROP_PURPOSE_ARRAY_CAST:
		view = true;
		break;
	case ZEND_PROP_PURPOSE_SERIALIZE:
	case ZEND_PROP_PURPOSE_VAR_EXPORT:
		break;
	default:
		return NULL;
	}
	props = zend_new_array(1);
	ZVAL_STR_COPY(&value, base->str);
	if (view) {
		zend_hash_str_add_new(props, ZEND_STRL("value"), &value);
	} else {
		zend_hash_next_index_insert(props, &value);
	}
	return props;
}

static HashTable *xstring_get_properties(zend_object *obj)
{
	if (obj->properties) {
		zend_array_release(obj->properties);
	}
	obj->properties = xstring_get_properties_for(obj, ZEND_PROP_PURPOSE_ARRAY_CAST);
	return obj->properties;
}

PHP_METHOD(Cbor_XString, __construct)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, Z_OBJ_P(ZEND_THIS));
	zend_string *value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &value) != SUCCESS) {
		RETURN_THROWS();
	}
	zend_string_addref(value);
	base->str = value;
}

static bool xstring_restore(zend_object *obj, HashTable *ht)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	zval *value;
	value = zend_hash_index_find(ht, 0);
	if (!value || Z_TYPE_P(value) != IS_STRING) {
		zend_throw_error(NULL, "Unable to restore %s.", ZSTR_VAL(obj->ce->name));
		return false;
	}
	zend_string_release(base->str);
	base->str = Z_STR_P(value);
	zend_string_addref(base->str);
	return true;
}

PHP_METHOD(Cbor_XString, __set_state)
{
	zend_class_entry *ctx_ce = zend_get_called_scope(execute_data);
	HashTable *ht;
	zval value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &ht) != SUCCESS) {
		RETURN_THROWS();
	}
	if (object_init_ex(&value, ctx_ce) != SUCCESS) {
		if (!EG(exception)) {
			zend_throw_error(NULL, "Unable to create an object.");
		}
		RETURN_THROWS();
	}
	if (!xstring_restore(Z_OBJ(value), ht)) {
		RETURN_THROWS();
	}
	RETURN_COPY_VALUE(&value);
}

PHP_METHOD(Cbor_XString, __unserialize)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	HashTable *ht;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &ht) != SUCCESS) {
		RETURN_THROWS();
	}
	if (!xstring_restore(obj, ht)) {
		RETURN_THROWS();
	}
}

zend_string *cbor_get_xstring_value(zval *ins)
{
	zend_object *obj = Z_OBJ_P(ins);
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	zval value;
	ZVAL_STR_COPY(&value, base->str);
	return Z_STR(value);
}

PHP_METHOD(Cbor_XString, jsonSerialize)
{
	zend_parse_parameters_none();
	RETURN_STR(cbor_get_xstring_value(ZEND_THIS));
}

#undef THIS_PROP

zend_object *cbor_floatx_create(zend_class_entry *ce)
{
	floatx_class *base = zend_object_alloc(sizeof(floatx_class), ce);
	zend_object_std_init(&base->std, ce);
	base->std.handlers = &floatx_handlers;
	return &base->std;
}

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(floatx, prop_literal)

static double floatx_to_double(zend_object *obj)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	double value;
	if (obj->ce == CBOR_CE(float32)) {
		value = (double)base->v.binary32.f;
	} else {
		value = cbor_from_float16(base->v.binary16);
	}
	return value;
}

static int floatx_cast(zend_object *obj, zval *retval, int type)
{
	if (type != IS_DOUBLE) {
		/* IS_STRING cast may not be necessarily desirable. */
		return FAILURE;
	}
	ZVAL_DOUBLE(retval, floatx_to_double(obj));
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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "d", &num) != SUCCESS) {
		RETURN_THROWS();
	}
	ZVAL_DOUBLE(&value, num);
	if (!cbor_floatx_set_value(obj, &value, 0)) {
		RETURN_THROWS();
	}
}

PHP_METHOD(Cbor_FloatX, fromBinary)
{
	zend_class_entry *ctx_ce = zend_get_called_scope(execute_data);
	zend_object *obj;
	zend_string *str;
	zval value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &str) != SUCCESS) {
		RETURN_THROWS();
	}
	TEST_FLOATX_CLASS(ctx_ce);
	obj = cbor_floatx_create(ctx_ce);
	ZVAL_STR(&value, str);
	if (!cbor_floatx_set_value(obj, &value, 0)) {
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
			|| !cbor_floatx_set_value(obj, value, 0)) {
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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &ht) != SUCCESS) {
		RETURN_THROWS();
	}
	TEST_FLOATX_CLASS(ctx_ce);
	obj = cbor_floatx_create(ctx_ce);
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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "h", &ht) != SUCCESS) {
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
	zend_parse_parameters_none();
	TEST_FLOATX_CLASS(obj->ce);
	RETURN_DOUBLE(floatx_to_double(obj));
}

bool cbor_floatx_set_value(zend_object *obj, zval *value, uint32_t raw)
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
				base->v.binary16 = cbor_to_float16(f_value);
			}
			return true;
		} else if (type == IS_STRING) {
			size_t req_len;
			const uint8_t *ptr = (const uint8_t *)Z_STRVAL_P(value);
			req_len = (base->std.ce == CBOR_CE(float32)) ? 4 : 2;
			if (Z_STRLEN_P(value) != req_len) {
				zend_value_error("Unexpected value length.");
				return false;
			}
			if (base->std.ce == CBOR_CE(float32)) {
				raw = ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) | ((uint32_t)ptr[2] << 8) | ptr[3];
			} else {
				raw = ((uint16_t)ptr[0] << 8) | ptr[1];
			}
		} else {
			zend_type_error("Invalid value type.");
			return false;
		}
	}
	if (base->std.ce == CBOR_CE(float32)) {
		base->v.binary32.i = raw;
	} else {
		base->v.binary16 = (uint16_t)raw;
	}
	return true;
}

static zend_object *floatx_clone(zend_object *obj)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	zend_object *new_obj = cbor_floatx_create(obj->ce);
	floatx_class *new_base = CUSTOM_OBJ(floatx_class, new_obj);
	new_base->v = base->v;
	zend_objects_clone_members(new_obj, obj);
	return new_obj;
}

static zval *floatx_read_property(zend_object *obj, zend_string *member, int type, void **cache_slot, zval *rv)
{
	if (zend_string_equals_literal(member, "value")) {
		ZVAL_DOUBLE(rv, floatx_to_double(obj));
	} else {
		rv = zend_std_read_property(obj, member, type, cache_slot, rv);
	}
	return rv;
}

static zval *floatx_write_property(zend_object *obj, zend_string *member, zval *value, void **cache_slot)
{
	if (zend_string_equals_literal(member, "value")) {
		cbor_floatx_set_value(obj, value, 0);
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

size_t cbor_floatx_get_value(zend_object *obj, char *out)
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
			value = cbor_from_float16(base->v.binary16);
		}
		ZVAL_DOUBLE(&zv, value);
		zend_hash_str_add_new(props, ZEND_STRL("value"), &zv);
	} else {
		char bin[4];
		size_t len = cbor_floatx_get_value(obj, bin);
		ZVAL_STRINGL(&zv, bin, len);
		zend_hash_next_index_insert(props, &zv);
	}
	return props;
}

static HashTable *floatx_get_properties(zend_object *obj)
{
	if (obj->properties) {
		zend_array_release(obj->properties);
	}
	obj->properties = floatx_get_properties_for(obj, ZEND_PROP_PURPOSE_ARRAY_CAST);
	return obj->properties;
}

double cbor_from_float16(uint16_t value)
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

uint16_t cbor_to_float16(float value)
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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &value, &content) != SUCCESS) {
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
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &value) != SUCCESS) {
		RETURN_THROWS();
	}
	zend_update_property(THIS_PROP("value"), value);
}

PHP_METHOD(Cbor_Shareable, jsonSerialize)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	zval *value, zv;
	zend_parse_parameters_none();
	value = zend_read_property(obj->ce, obj, ZEND_STRL("value"), false, &zv);
	RETURN_COPY(value);
}

#undef THIS_PROP

void php_cbor_minit_types()
{
	CBOR_CE(undefined)->serialize = undef_serialize;
	CBOR_CE(undefined)->unserialize = undef_unserialize;
	memcpy(&undef_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	undef_handlers.clone_obj = &undef_clone;
	undef_handlers.read_property = &undef_read_property;
	undef_handlers.write_property = &undef_write_property;
	undef_handlers.get_property_ptr_ptr = &undef_get_property_ptr_ptr;
	undef_handlers.has_property = &undef_has_property;
	undef_handlers.unset_property = &undef_unset_property;
	undef_handlers.cast_object = &undef_cast;

	CBOR_CE(xstring)->create_object = &cbor_xstring_create;
	CBOR_CE(byte)->create_object = &cbor_xstring_create;
	CBOR_CE(text)->create_object = &cbor_xstring_create;
	memcpy(&xstring_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	xstring_handlers.offset = XtOffsetOf(xstring_class, std);
	xstring_handlers.free_obj = &xstring_free;
	xstring_handlers.clone_obj = &xstring_clone;
	xstring_handlers.read_property = &xstring_read_property;
	xstring_handlers.write_property = &xstring_write_property;
	xstring_handlers.get_property_ptr_ptr = &xstring_get_property_ptr_ptr;
	xstring_handlers.has_property = &xstring_has_property;
	xstring_handlers.unset_property = &xstring_unset_property;
	xstring_handlers.cast_object = &xstring_cast;
	xstring_handlers.get_properties = &xstring_get_properties;
	xstring_handlers.get_properties_for = &xstring_get_properties_for;

	CBOR_CE(float16)->create_object = &cbor_floatx_create;
	CBOR_CE(float32)->create_object = &cbor_floatx_create;
	memcpy(&floatx_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	floatx_handlers.offset = XtOffsetOf(floatx_class, std);
	floatx_handlers.clone_obj = &floatx_clone;
	floatx_handlers.read_property = &floatx_read_property;
	floatx_handlers.write_property = &floatx_write_property;
	floatx_handlers.get_property_ptr_ptr = &floatx_get_property_ptr_ptr;
	floatx_handlers.has_property = &floatx_has_property;
	floatx_handlers.unset_property = &floatx_unset_property;
	floatx_handlers.cast_object = &floatx_cast;
	floatx_handlers.get_properties = &floatx_get_properties;
	floatx_handlers.get_properties_for = &floatx_get_properties_for;

	php_cbor_minit_decoder();
}
