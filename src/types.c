/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "cbor_globals.h"
#include "cpu_id.h"
#include "types.h"
#include "type_float_cast.h"
#include "compatibility.h"
#include <math.h>

#define DEF_THIS(name, prop_literal)  CBOR_CE(name), Z_OBJ_P(ZEND_THIS)

typedef struct {
	zend_string *str;
	zend_object std;
} xstring_class;

typedef struct {
	union floatx_class_v {
		binary32_alias binary32;
		cbor_fp16i binary16;
	} v;
	zend_object std;
} floatx_class;

static zend_object_handlers undef_handlers;
static zend_object_handlers xstring_handlers;
static zend_object_handlers floatx_handlers;

static void cbor_floatx_set_fp64(zend_object *obj, double value);

#define THIS()  DEF_THIS(tag, prop_literal)

PHP_METHOD(Cbor_EncodeParams, __construct)
{
	zval *value, *params;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "za", &value, &params) != SUCCESS) {
		RETURN_THROWS();
	}
	zend_update_property_ex(THIS(), ZSTR_KNOWN(ZEND_STR_VALUE), value);
	zend_update_property(THIS(), ZEND_STRL("params"), params);
}

#undef THIS

static zval *no_get_property_ptr_ptr(zend_object *obj, zend_string *member, int type, void **cache_slot)
{
	return NULL;
}

static void no_unset_property(zend_object *obj, zend_string *member, void **cache_slot)
{
	zend_throw_error(NULL, "%s cannot unset properties.", ZSTR_VAL(obj->ce->name));
}

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

static void undef_dtor(zend_object *obj)
{
	if (obj == CBOR_G(undef_ins)) {
		CBOR_G(undef_ins) = NULL;
	}
	zend_objects_destroy_object(obj);
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

static int undef_has_property(zend_object *obj, zend_string *member, int has_set_exists, void **cache_slot)
{
	return 0;
}

static zend_result_82 undef_cast(zend_object *obj, zval *retval, int type)
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

#define THIS()  DEF_THIS(xstring, prop_literal)

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

static zend_result_82 xstring_cast(zend_object *obj, zval *retval, int type)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
	if (type != IS_STRING) {
		return FAILURE;
	}
	ZVAL_STR_COPY(retval, base->str);
	return SUCCESS;
}

static int xstring_compare(zval *val1, zval *val2)
{
	if (Z_TYPE_P(val1) != Z_TYPE_P(val2)) {
		return zend_std_compare_objects(val1, val2);
	}
	zend_object *obj1 = Z_OBJ_P(val1);
	zend_object *obj2 = Z_OBJ_P(val2);
	if (obj1 == obj2) {
		return 0;
	}
	if (obj1->ce != obj2->ce) {
		return ZEND_UNCOMPARABLE;
	}
	xstring_class *base1 = CUSTOM_OBJ(xstring_class, obj1);
	xstring_class *base2 = CUSTOM_OBJ(xstring_class, obj2);
	return zend_binary_strcmp(ZSTR_VAL(base1->str), ZSTR_LEN(base1->str), ZSTR_VAL(base2->str), ZSTR_LEN(base2->str));
}

static bool xstring_copy_properties(zend_object *obj, zend_prop_purpose purpose, zend_array *props)
{
	xstring_class *base = CUSTOM_OBJ(xstring_class, obj);
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
		return false;
	}
	ZVAL_STR_COPY(&value, base->str);
	if (view) {
		zend_hash_update(props, ZSTR_KNOWN(ZEND_STR_VALUE), &value);
	} else {
		zend_hash_index_update(props, 0, &value);
	}
	return true;
}

static zend_array *xstring_get_properties_for(zend_object *obj, zend_prop_purpose purpose)
{
	zend_array *props = zend_new_array(1);
	if (!xstring_copy_properties(obj, purpose, props)) {
		zend_array_destroy(props);
		props = NULL;
	}
	return props;
}

static HashTable *xstring_get_properties(zend_object *obj)
{
	if (!obj->properties) {
		obj->properties = zend_new_array(1);
	}
	xstring_copy_properties(obj, ZEND_PROP_PURPOSE_ARRAY_CAST, obj->properties);
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
		zval_ptr_dtor(&value);
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

#undef THIS

zend_object *cbor_floatx_create(zend_class_entry *ce)
{
	floatx_class *base = zend_object_alloc(sizeof(floatx_class), ce);
	zend_object_std_init(&base->std, ce);
	base->std.handlers = &floatx_handlers;
	return &base->std;
}

#define THIS()  DEF_THIS(floatx, prop_literal)

static double floatx_to_fp64(zend_object *obj)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	double value;
	if (obj->ce == CBOR_CE(float32)) {
		value = cbor_from_fp32(base->v.binary32.f);
	} else {
		value = cbor_from_fp16i(base->v.binary16);
	}
	return value;
}

static zend_result_82 floatx_cast(zend_object *obj, zval *retval, int type)
{
	if (type != IS_DOUBLE) {
		// IS_LONG cast is not appropriate (implicit conversion may happen.)
		// IS_STRING cast may not be necessarily desirable.
		return FAILURE;
	}
	ZVAL_DOUBLE(retval, floatx_to_fp64(obj));
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
	cbor_floatx_set_fp64(obj, num);
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
	RETURN_DOUBLE(floatx_to_fp64(obj));
}

PHP_METHOD(Cbor_FloatX, toBinary)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	zend_parse_parameters_none();
	TEST_FLOATX_CLASS(obj->ce);
	char buf[4];
	size_t len = cbor_floatx_get_value_be(obj, buf);
	RETURN_STRINGL(buf, len);
}

static void cbor_floatx_to_floatx(INTERNAL_FUNCTION_PARAMETERS)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	zend_parse_parameters_none();
	TEST_FLOATX_CLASS(obj->ce);
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	double value;
	zend_class_entry *r_ce;
	if (obj->ce == CBOR_CE(float16)) {
		value = cbor_from_fp16i(base->v.binary16);
		r_ce = CBOR_CE(float32);
	} else {
		assert(obj->ce == CBOR_CE(float32));
		value = cbor_from_fp32(base->v.binary32.f);
		r_ce = CBOR_CE(float16);
	}
	zend_object *r_obj = cbor_floatx_create(r_ce);
	cbor_floatx_set_fp64(r_obj, value);
	RETURN_OBJ(r_obj);
}

PHP_METHOD(Cbor_Float16, toFloat32)
{
	cbor_floatx_to_floatx(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

PHP_METHOD(Cbor_Float32, toFloat16)
{
	cbor_floatx_to_floatx(INTERNAL_FUNCTION_PARAM_PASSTHRU);
}

static void cbor_floatx_set_fp64(zend_object *obj, double value)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	if (base->std.ce == CBOR_CE(float32)) {
		base->v.binary32.f = cbor_to_fp32(value);
	} else {
		base->v.binary16 = cbor_float_64_to_16(value);
	}
}

bool cbor_floatx_set_value(zend_object *obj, zval *value, uint32_t raw)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	if (value) {
		int type = Z_TYPE_P(value);
		if (type == IS_DOUBLE || type == IS_LONG) {
			// IS_LONG if called from write_property.
			double d_value = (type == IS_LONG) ? Z_LVAL_P(value) : Z_DVAL_P(value);
			cbor_floatx_set_fp64(obj, d_value);
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
				raw = ((cbor_fp16i)ptr[0] << 8) | ptr[1];
			}
		} else {
			zend_type_error("Invalid value type.");
			return false;
		}
	}
	if (base->std.ce == CBOR_CE(float32)) {
		base->v.binary32.i = raw;
	} else {
		base->v.binary16 = (cbor_fp16i)raw;
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
		ZVAL_DOUBLE(rv, floatx_to_fp64(obj));
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

size_t cbor_floatx_get_value_be(zend_object *obj, char *out)
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

float cbor_float32_get_value(zend_object *obj)
{
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
	if (obj->ce == CBOR_CE(float32)) {
		return base->v.binary32.f;
	}
	assert(obj->ce == CBOR_CE(float32));
	return 0;
}

typedef enum {
	// wider value must be defined later
	FOT_INT,
	FOT_FP16,
	FOT_FP16_32, // intermediate type for FP16
	FOT_FP32,
	FOT_FP64,
} float_operand_type;

typedef struct {
	float_operand_type type;
	union {
		zend_long i;
		cbor_fp16i f16;
		float f32;
		double f64;
	} v;
} float_operand;

static bool floatx_load_operand(float_operand *ope, zval *val)
{
	if (Z_TYPE_P(val) == IS_OBJECT) {
		zend_object *obj = Z_OBJ_P(val);
		floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
		if (obj->ce == CBOR_CE(float16)) {
			ope->type = FOT_FP16;
			ope->v.f16 = base->v.binary16;
		} else if (obj->ce == CBOR_CE(float32)) {
			ope->type = FOT_FP32;
			ope->v.f32 = base->v.binary32.f;
		} else {
			return false;
		}
		return true;
	} else if (Z_TYPE_P(val) == IS_LONG) {
		ope->type = FOT_INT;
		ope->v.i = Z_LVAL_P(val);
		return true;
	}
	return false;
}

static bool floatx_cast_operands(float_operand *ope1, float_operand *ope2)
{
	if (ope1->type == ope2->type && ope1->type != FOT_FP16) {
		return true;
	}
	if (ope1->type < ope2->type) {
		return floatx_cast_operands(ope2, ope1);
	}
	if (ope1->type == FOT_FP16) {
		ope1->v.f32 = (float)cbor_from_fp16i(ope1->v.f16);
		ope1->type = FOT_FP16_32;
		if (ope2->type == FOT_INT) {
			ope2->v.f32 = (float)ope2->v.i;
			ope2->type = FOT_FP16_32;
		} else {
			assert(ope2->type == FOT_FP16);
			ope2->v.f32 = (float)cbor_from_fp16i(ope2->v.f16);
			ope2->type = FOT_FP16_32;
		}
	} else if (ope1->type == FOT_FP32) {
		if (ope2->type == FOT_INT) {
			ope2->v.f32 = (float)ope2->v.i;
		} else {
			assert(ope2->type == FOT_FP16);
			ope2->v.f32 = (float)cbor_from_fp16i(ope2->v.f16);
		}
		ope2->type = FOT_FP32;
	} else if (ope1->type == FOT_FP64) {
		if (ope2->type == FOT_INT) {
			ope2->v.f64 = (double)ope2->v.i;
		} else if (ope2->type == FOT_FP16) {
			ope2->v.f64 = cbor_from_fp16i(ope2->v.f16);
		} else {
			assert(ope2->type == FOT_FP32);
			ope2->v.f64 = cbor_from_fp32(ope2->v.f32);
		}
		ope2->type = FOT_FP64;
	} else {
		return false;
	}
	return true;
}

static int floatx_compare(zval *val1, zval *val2)
{
	if (Z_TYPE_P(val1) != Z_TYPE_P(val2)) {
		return zend_std_compare_objects(val1, val2);
	}
	zend_object *obj1 = Z_OBJ_P(val1);
	zend_object *obj2 = Z_OBJ_P(val2);
	if (obj1 == obj2) {
		return 0;
	}
	float_operand ope1, ope2;
	if (!floatx_load_operand(&ope1, val1) || !floatx_load_operand(&ope2, val2)) {
		return ZEND_UNCOMPARABLE;
	}
	if (!floatx_cast_operands(&ope1, &ope2)) {
		return ZEND_UNCOMPARABLE;
	}
	assert(ope1.type == ope2.type);
	switch (ope1.type) {
	case FOT_FP16_32:
	case FOT_FP32:
		if (isnan(ope1.v.f32) || isnan(ope2.v.f32)) {
			break;
		}
		return (ope1.v.f32 == ope2.v.f32) ? 0 : (ope1.v.f32 < ope2.v.f32) ? -1 : 1;
	case FOT_FP64:
		if (isnan(ope1.v.f64) || isnan(ope2.v.f64)) {
			break;
		}
		return (ope1.v.f64 == ope2.v.f64) ? 0 : (ope1.v.f64 < ope2.v.f64) ? -1 : 1;
	}
	return ZEND_UNCOMPARABLE;
}

static bool floatx_copy_properties(zend_object *obj, zend_prop_purpose purpose, zend_array *props)
{
	/* While floatX can have custom properties for now, they are not dumped or restored. */
	floatx_class *base = CUSTOM_OBJ(floatx_class, obj);
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
		return false;
	}
	if (decode) {
		double value;
		if (obj->ce == CBOR_CE(float32)) {
			value = cbor_from_fp32(base->v.binary32.f);
		} else {
			value = cbor_from_fp16i(base->v.binary16);
		}
		ZVAL_DOUBLE(&zv, value);
		zend_hash_update(props, ZSTR_KNOWN(ZEND_STR_VALUE), &zv);
	} else {
		char bin[4];
		size_t len = cbor_floatx_get_value_be(obj, bin);
		ZVAL_STRINGL(&zv, bin, len);
		zend_hash_index_update(props, 0, &zv);
	}
	return true;
}

static zend_array *floatx_get_properties_for(zend_object *obj, zend_prop_purpose purpose)
{
	zend_array *props = zend_new_array(1);
	if (!floatx_copy_properties(obj, purpose, props)) {
		zend_array_destroy(props);
		props = zend_std_get_properties_for(obj, purpose);
	}
	return props;
}

static HashTable *floatx_get_properties(zend_object *obj)
{
	if (!obj->properties) {
		obj->properties = zend_new_array(1);
	}
	floatx_copy_properties(obj, ZEND_PROP_PURPOSE_ARRAY_CAST, obj->properties);
	return obj->properties;
}

double cbor_from_fp16i(cbor_fp16i value)
{
	if (has_f16c()) {
		if ((value & 0x7e00) != 0x7c00) { // not sNaN-ish
			return fp16_to_fp32_i(value);
		}
	}
	/* Based on RFC 8949 code */
	binary64_alias bin64;
	cbor_fp16i exp = (value >> 10) & 0x1f;  /* 0b11111 */
	cbor_fp16i frac = value & 0x3ff;  /* 0b11_1111_1111 */
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

double cbor_from_fp32(float value)
{
	if (EXPECTED(!isnan(value))) {
		return (double)value;
	}
	// Cast but preserve sNaN when possible
	binary32_alias f_value;
	f_value.f = value;
	binary64_alias d_value;
	d_value.i = F64_EXP_FILL << F64_FRAC_BITS;
	F64_UINT_TYPE fraction = ((F64_UINT_TYPE)(f_value.i & F32_FRAC_MASK)) << (F64_FRAC_BITS - F32_FRAC_BITS);
	d_value.i |= fraction;
	if (!fraction) {
		d_value.i |= 1ull << (F64_FRAC_BITS - 1); // INF => qNaN
	}
	return d_value.f;
}

float cbor_to_fp32(double value)
{
	if (EXPECTED(!isnan(value))) {
		return (float)value;
	}
	binary64_alias d_value;
	d_value.f = value;
	binary32_alias f_value;
	f_value.i = F32_EXP_FILL << F32_FRAC_BITS;
	F32_UINT_TYPE fraction = (F32_UINT_TYPE)((d_value.i & ((F64_UINT_TYPE)F32_FRAC_MASK << (F64_FRAC_BITS - F32_FRAC_BITS))) >> (F64_FRAC_BITS - F32_FRAC_BITS));
	f_value.i |= fraction;
	if (!fraction) {
		f_value.i |= 1 << (F32_FRAC_BITS - 1); // INF => qNaN
	}
	return f_value.f;
}

#undef THIS
#define THIS()  DEF_THIS(tag, prop_literal)

PHP_METHOD(Cbor_Tag, __construct)
{
	zend_long value;
	zval *content;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &value, &content) != SUCCESS) {
		RETURN_THROWS();
	}
	zend_update_property_long(THIS(), ZEND_STRL("tag"), value);
	zend_update_property(THIS(), ZEND_STRL("content"), content);
}

#undef THIS
#define THIS()  DEF_THIS(shareable, prop_literal)

PHP_METHOD(Cbor_Shareable, __construct)
{
	zval *value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &value) != SUCCESS) {
		RETURN_THROWS();
	}
	zend_update_property_ex(THIS(), ZSTR_KNOWN(ZEND_STR_VALUE), value);
}

PHP_METHOD(Cbor_Shareable, jsonSerialize)
{
	zend_object *obj = Z_OBJ_P(ZEND_THIS);
	zval *value, zv;
	zend_parse_parameters_none();
	value = zend_read_property_ex(obj->ce, obj, ZSTR_KNOWN(ZEND_STR_VALUE), false, &zv);
	RETURN_COPY(value);
}

#undef THIS

void php_cbor_minit_types()
{
	CBOR_CE(undefined)->serialize = undef_serialize;
	CBOR_CE(undefined)->unserialize = undef_unserialize;
	memcpy(&undef_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	undef_handlers.dtor_obj = &undef_dtor;
	undef_handlers.clone_obj = &undef_clone;
	undef_handlers.read_property = &undef_read_property;
	undef_handlers.write_property = &undef_write_property;
	undef_handlers.get_property_ptr_ptr = &no_get_property_ptr_ptr;
	undef_handlers.has_property = &undef_has_property;
	undef_handlers.unset_property = &no_unset_property;
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
	xstring_handlers.get_property_ptr_ptr = &no_get_property_ptr_ptr;
	xstring_handlers.has_property = &xstring_has_property;
	xstring_handlers.unset_property = &no_unset_property;
	xstring_handlers.cast_object = &xstring_cast;
	xstring_handlers.compare = &xstring_compare;
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
	floatx_handlers.compare = &floatx_compare;
	floatx_handlers.get_properties = &floatx_get_properties;
	floatx_handlers.get_properties_for = &floatx_get_properties_for;

	php_cbor_minit_types_float_cast();
	php_cbor_minit_decoder();
}
