/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: b9e77f905bf317829c3ea3653cd12a4ccbc57ee4 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Encodable_cborEncode, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_Undefined___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Undefined___set_state, 0, 1, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, properties, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Cbor_Undefined_get, 0, 0, Cbor\\Undefined, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_XString___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_FloatX___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_Tag___construct, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, tag, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, content, IS_MIXED, 0)
ZEND_END_ARG_INFO()


ZEND_METHOD(Cbor_Undefined, __construct);
ZEND_METHOD(Cbor_Undefined, __set_state);
ZEND_METHOD(Cbor_Undefined, get);
ZEND_METHOD(Cbor_XString, __construct);
ZEND_METHOD(Cbor_FloatX, __construct);
ZEND_METHOD(Cbor_Tag, __construct);


static const zend_function_entry class_Cbor_Exception_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Encodable_methods[] = {
	ZEND_ABSTRACT_ME_WITH_FLAGS(Cbor_Encodable, cborEncode, arginfo_class_Cbor_Encodable_cborEncode, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Undefined_methods[] = {
	ZEND_ME(Cbor_Undefined, __construct, arginfo_class_Cbor_Undefined___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(Cbor_Undefined, __set_state, arginfo_class_Cbor_Undefined___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Cbor_Undefined, get, arginfo_class_Cbor_Undefined_get, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_XString_methods[] = {
	ZEND_ME(Cbor_XString, __construct, arginfo_class_Cbor_XString___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Byte_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Text_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_FloatX_methods[] = {
	ZEND_ME(Cbor_FloatX, __construct, arginfo_class_Cbor_FloatX___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Float16_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Float32_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Tag_methods[] = {
	ZEND_ME(Cbor_Tag, __construct, arginfo_class_Cbor_Tag___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_Cbor_Exception(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Exception", class_Cbor_Exception_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Encodable(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Encodable", class_Cbor_Encodable_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Undefined(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Undefined", class_Cbor_Undefined_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_Cbor_XString(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "XString", class_Cbor_XString_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_ABSTRACT;

	zval property_value_default_value;
	ZVAL_UNDEF(&property_value_default_value);
	zend_string *property_value_name = zend_string_init("value", sizeof("value") - 1, 1);
	zend_declare_typed_property(class_entry, property_value_name, &property_value_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release(property_value_name);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Byte(zend_class_entry *class_entry_Cbor_XString)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Byte", class_Cbor_Byte_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Cbor_XString);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Text(zend_class_entry *class_entry_Cbor_XString)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Text", class_Cbor_Text_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Cbor_XString);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_Cbor_FloatX(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "FloatX", class_Cbor_FloatX_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_ABSTRACT;

	zval property_value_default_value;
	ZVAL_UNDEF(&property_value_default_value);
	zend_string *property_value_name = zend_string_init("value", sizeof("value") - 1, 1);
	zend_declare_typed_property(class_entry, property_value_name, &property_value_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_DOUBLE));
	zend_string_release(property_value_name);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Float16(zend_class_entry *class_entry_Cbor_FloatX)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Float16", class_Cbor_Float16_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Cbor_FloatX);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Float32(zend_class_entry *class_entry_Cbor_FloatX)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Float32", class_Cbor_Float32_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Cbor_FloatX);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Tag(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Tag", class_Cbor_Tag_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	zval property_tag_default_value;
	ZVAL_UNDEF(&property_tag_default_value);
	zend_string *property_tag_name = zend_string_init("tag", sizeof("tag") - 1, 1);
	zend_declare_typed_property(class_entry, property_tag_name, &property_tag_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_LONG));
	zend_string_release(property_tag_name);

	zval property_content_default_value;
	ZVAL_UNDEF(&property_content_default_value);
	zend_string *property_content_name = zend_string_init("content", sizeof("content") - 1, 1);
	zend_declare_typed_property(class_entry, property_content_name, &property_content_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ANY));
	zend_string_release(property_content_name);

	return class_entry;
}
