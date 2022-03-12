/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"

#define LIT_PROP(prop_literal)  prop_literal, sizeof prop_literal - 1
#define DEF_THIS_PROP(name, prop_literal)  CBOR_CE(name), Z_OBJ_P(self), LIT_PROP(prop_literal)

static zend_object_handlers undef_handlers;
static zend_object_handlers xstring_handlers;
static zend_object_handlers floatx_handlers;

zend_object *undef_clone_handler(zend_object *object);
int undef_cast_object_handler(zend_object *readobj, zval *retval, int type);

zend_object *xstring_create_object_handler(zend_class_entry *class_type);
int xstring_cast_object_handler(zend_object *readobj, zval *retval, int type);

zend_object *floatx_create_object_handler(zend_class_entry *class_type);
int floatx_cast_object_handler(zend_object *readobj, zval *retval, int type);

void php_cbor_minit_types()
{
	memcpy(&undef_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	undef_handlers.clone_obj = &undef_clone_handler;
	undef_handlers.cast_object = &undef_cast_object_handler;

	/* Setting CBOR_CE(xstring)->create_object does not help. */
	CBOR_CE(byte)->create_object = &xstring_create_object_handler;
	CBOR_CE(text)->create_object = &xstring_create_object_handler;
	memcpy(&xstring_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	xstring_handlers.cast_object = &xstring_cast_object_handler;

	CBOR_CE(float16)->create_object = &floatx_create_object_handler;
	CBOR_CE(float32)->create_object = &floatx_create_object_handler;
	memcpy(&floatx_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	floatx_handlers.cast_object = &floatx_cast_object_handler;
}

/* PHP has IS_UNDEF type, but it is semantically different from CBOR's "undefined" value. */

zend_object *undef_clone_handler(zend_object *object)
{
	GC_ADDREF(object);
	return object;
}

int undef_cast_object_handler(zend_object *readobj, zval *retval, int type)
{
	if (type != _IS_BOOL) {
		return FAILURE;
	}
	ZVAL_FALSE(retval);
	return SUCCESS;
}

PHP_METHOD(Cbor_Undefined, __construct)
{
	zend_throw_error(NULL, "You cannot instantiate Cbor\\Undefined.");
	RETURN_THROWS();
}

zend_object *php_cbor_get_undef()
{
	if (!CBOR_G(undef_ins)) {
		zend_object *obj = zend_objects_new(CBOR_CE(undefined));
		obj->handlers = &undef_handlers;
		CBOR_G(undef_ins) = obj;
	}
	return CBOR_G(undef_ins);
}

PHP_METHOD(Cbor_Undefined, __set_state)
{
	RETVAL_OBJ(php_cbor_get_undef());
	Z_ADDREF_P(return_value);
}

PHP_METHOD(Cbor_Undefined, get)
{
	RETVAL_OBJ(php_cbor_get_undef());
	Z_ADDREF_P(return_value);
}

zend_object *xstring_create_object_handler(zend_class_entry *class_type)
{
	zend_object *obj = zend_objects_new(class_type);
	object_properties_init(obj, class_type);
	obj->handlers = &xstring_handlers;
	return obj;
}

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(xstring, prop_literal)

int xstring_cast_object_handler(zend_object *readobj, zval *retval, int type)
{
	zval *value, zv;
	if (type != IS_STRING) {
		return FAILURE;
	}
	value = zend_read_property(readobj->ce, readobj, LIT_PROP("value"), false, &zv);
	Z_ADDREF_P(value);
	ZVAL_COPY_VALUE(retval, value);
	return SUCCESS;
}

PHP_METHOD(Cbor_XString, __construct)
{
	zval *self;
	zend_string *value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &value) == FAILURE) {
		RETURN_THROWS();
	}
	self = getThis();
	zend_update_property_str(THIS_PROP("value"), value);
}

zend_string *php_cbor_get_xstring_value(zval *ins)
{
	zval *value, zv;
	value = zend_read_property(Z_OBJ_P(ins)->ce, Z_OBJ_P(ins), LIT_PROP("value"), false, &zv);
	Z_ADDREF_P(value);
	return Z_STR_P(value);
}

#undef THIS_PROP

zend_object *floatx_create_object_handler(zend_class_entry *class_type)
{
	zend_object *obj = zend_objects_new(class_type);
	object_properties_init(obj, class_type);
	obj->handlers = &floatx_handlers;
	return obj;
}

#define THIS_PROP(prop_literal)  DEF_THIS_PROP(floatx, prop_literal)

int floatx_cast_object_handler(zend_object *readobj, zval *retval, int type)
{
	zval *value, zv;
	if (type != IS_DOUBLE) {
		return FAILURE;
	}
	value = zend_read_property(readobj->ce, readobj, LIT_PROP("value"), false, &zv);
	ZVAL_DOUBLE(retval, Z_DVAL_P(value));
	return SUCCESS;
}

PHP_METHOD(Cbor_FloatX, __construct)
{
	zval *self;
	double value;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "d", &value) == FAILURE) {
		RETURN_THROWS();
	}
	self = getThis();
	zend_update_property_double(THIS_PROP("value"), value);
}

#undef THIS_PROP
#define THIS_PROP(prop_literal)  DEF_THIS_PROP(tag, prop_literal)

PHP_METHOD(Cbor_Tag, __construct)
{
	zval *self;
	zend_long value;
	zval *content;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &value, &content) == FAILURE) {
		RETURN_THROWS();
	}
	self = getThis();
	zend_update_property_long(THIS_PROP("tag"), value);
	zend_update_property(THIS_PROP("content"), content);
}

#undef THIS_PROP
