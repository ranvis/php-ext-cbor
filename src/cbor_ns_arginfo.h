/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 45601d4aa3cb8cefdfa2b3163185ba088bcd02c9 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Serializable_cborSerialize, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_EncodeParams___construct, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, params, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_Undefined___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_Undefined___destruct arginfo_class_Cbor_Undefined___construct

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Undefined___set_state, 0, 1, IS_OBJECT, 0)
	ZEND_ARG_TYPE_INFO(0, properties, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Cbor_Undefined_get, 0, 0, Cbor\\Undefined, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_Undefined_jsonSerialize arginfo_class_Cbor_Serializable_cborSerialize

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_XString___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Cbor_XString___set_state, 0, 1, Cbor\\XString, 0)
	ZEND_ARG_TYPE_INFO(0, properties, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_XString___unserialize, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_XString_jsonSerialize arginfo_class_Cbor_Serializable_cborSerialize

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_FloatX___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Cbor_FloatX_fromBinary, 0, 1, Cbor\\FloatX, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_Cbor_FloatX___set_state, 0, 1, Cbor\\FloatX, 0)
	ZEND_ARG_TYPE_INFO(0, properties, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_FloatX___unserialize arginfo_class_Cbor_XString___unserialize

#define arginfo_class_Cbor_FloatX_jsonSerialize arginfo_class_Cbor_Serializable_cborSerialize

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_Tag___construct, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, tag, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, content, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_Shareable___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_Shareable_jsonSerialize arginfo_class_Cbor_Serializable_cborSerialize

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Cbor_Decoder___construct, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "CBOR_BYTE | CBOR_KEY_BYTE")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Decoder_decode, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Decoder_add, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Decoder_process, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Decoder_reset, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_Decoder_hasValue arginfo_class_Cbor_Decoder_process

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Decoder_getValue, 0, 0, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, clear, _IS_BOOL, 0, "true")
ZEND_END_ARG_INFO()

#define arginfo_class_Cbor_Decoder_isPartial arginfo_class_Cbor_Decoder_process

#define arginfo_class_Cbor_Decoder_isProcessing arginfo_class_Cbor_Decoder_process

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Cbor_Decoder_getBuffer, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()


ZEND_METHOD(Cbor_EncodeParams, __construct);
ZEND_METHOD(Cbor_Undefined, __construct);
ZEND_METHOD(Cbor_Undefined, __destruct);
ZEND_METHOD(Cbor_Undefined, __set_state);
ZEND_METHOD(Cbor_Undefined, get);
ZEND_METHOD(Cbor_Undefined, jsonSerialize);
ZEND_METHOD(Cbor_XString, __construct);
ZEND_METHOD(Cbor_XString, __set_state);
ZEND_METHOD(Cbor_XString, __unserialize);
ZEND_METHOD(Cbor_XString, jsonSerialize);
ZEND_METHOD(Cbor_FloatX, __construct);
ZEND_METHOD(Cbor_FloatX, fromBinary);
ZEND_METHOD(Cbor_FloatX, __set_state);
ZEND_METHOD(Cbor_FloatX, __unserialize);
ZEND_METHOD(Cbor_FloatX, jsonSerialize);
ZEND_METHOD(Cbor_Tag, __construct);
ZEND_METHOD(Cbor_Shareable, __construct);
ZEND_METHOD(Cbor_Shareable, jsonSerialize);
ZEND_METHOD(Cbor_Decoder, __construct);
ZEND_METHOD(Cbor_Decoder, decode);
ZEND_METHOD(Cbor_Decoder, add);
ZEND_METHOD(Cbor_Decoder, process);
ZEND_METHOD(Cbor_Decoder, reset);
ZEND_METHOD(Cbor_Decoder, hasValue);
ZEND_METHOD(Cbor_Decoder, getValue);
ZEND_METHOD(Cbor_Decoder, isPartial);
ZEND_METHOD(Cbor_Decoder, isProcessing);
ZEND_METHOD(Cbor_Decoder, getBuffer);


static const zend_function_entry class_Cbor_Exception_methods[] = {
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Serializable_methods[] = {
	ZEND_ABSTRACT_ME_WITH_FLAGS(Cbor_Serializable, cborSerialize, arginfo_class_Cbor_Serializable_cborSerialize, ZEND_ACC_PUBLIC|ZEND_ACC_ABSTRACT)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_EncodeParams_methods[] = {
	ZEND_ME(Cbor_EncodeParams, __construct, arginfo_class_Cbor_EncodeParams___construct, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Undefined_methods[] = {
	ZEND_ME(Cbor_Undefined, __construct, arginfo_class_Cbor_Undefined___construct, ZEND_ACC_PRIVATE)
	ZEND_ME(Cbor_Undefined, __destruct, arginfo_class_Cbor_Undefined___destruct, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Undefined, __set_state, arginfo_class_Cbor_Undefined___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Cbor_Undefined, get, arginfo_class_Cbor_Undefined_get, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Cbor_Undefined, jsonSerialize, arginfo_class_Cbor_Undefined_jsonSerialize, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_XString_methods[] = {
	ZEND_ME(Cbor_XString, __construct, arginfo_class_Cbor_XString___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_XString, __set_state, arginfo_class_Cbor_XString___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Cbor_XString, __unserialize, arginfo_class_Cbor_XString___unserialize, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_XString, jsonSerialize, arginfo_class_Cbor_XString_jsonSerialize, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Cbor_FloatX, fromBinary, arginfo_class_Cbor_FloatX_fromBinary, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Cbor_FloatX, __set_state, arginfo_class_Cbor_FloatX___set_state, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(Cbor_FloatX, __unserialize, arginfo_class_Cbor_FloatX___unserialize, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_FloatX, jsonSerialize, arginfo_class_Cbor_FloatX_jsonSerialize, ZEND_ACC_PUBLIC)
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


static const zend_function_entry class_Cbor_Shareable_methods[] = {
	ZEND_ME(Cbor_Shareable, __construct, arginfo_class_Cbor_Shareable___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Shareable, jsonSerialize, arginfo_class_Cbor_Shareable_jsonSerialize, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_Cbor_Decoder_methods[] = {
	ZEND_ME(Cbor_Decoder, __construct, arginfo_class_Cbor_Decoder___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, decode, arginfo_class_Cbor_Decoder_decode, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, add, arginfo_class_Cbor_Decoder_add, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, process, arginfo_class_Cbor_Decoder_process, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, reset, arginfo_class_Cbor_Decoder_reset, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, hasValue, arginfo_class_Cbor_Decoder_hasValue, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, getValue, arginfo_class_Cbor_Decoder_getValue, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, isPartial, arginfo_class_Cbor_Decoder_isPartial, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, isProcessing, arginfo_class_Cbor_Decoder_isProcessing, ZEND_ACC_PUBLIC)
	ZEND_ME(Cbor_Decoder, getBuffer, arginfo_class_Cbor_Decoder_getBuffer, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_Cbor_Exception(zend_class_entry *class_entry_Exception)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Exception", class_Cbor_Exception_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_Exception);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Serializable(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Serializable", class_Cbor_Serializable_methods);
	class_entry = zend_register_internal_interface(&ce);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_EncodeParams(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "EncodeParams", class_Cbor_EncodeParams_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;

	zval property_value_default_value;
	ZVAL_UNDEF(&property_value_default_value);
	zend_string *property_value_name = zend_string_init("value", sizeof("value") - 1, 1);
	zend_declare_typed_property(class_entry, property_value_name, &property_value_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ANY));
	zend_string_release(property_value_name);

	zval property_params_default_value;
	ZVAL_UNDEF(&property_params_default_value);
	zend_string *property_params_name = zend_string_init("params", sizeof("params") - 1, 1);
	zend_declare_typed_property(class_entry, property_params_name, &property_params_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ARRAY));
	zend_string_release(property_params_name);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Undefined(zend_class_entry *class_entry_JsonSerializable)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Undefined", class_Cbor_Undefined_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;
	zend_class_implements(class_entry, 1, class_entry_JsonSerializable);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_XString(zend_class_entry *class_entry_JsonSerializable)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "XString", class_Cbor_XString_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_ABSTRACT;
	zend_class_implements(class_entry, 1, class_entry_JsonSerializable);

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

static zend_class_entry *register_class_Cbor_FloatX(zend_class_entry *class_entry_JsonSerializable)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "FloatX", class_Cbor_FloatX_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_ABSTRACT;
	zend_class_implements(class_entry, 1, class_entry_JsonSerializable);

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

static zend_class_entry *register_class_Cbor_Shareable(zend_class_entry *class_entry_JsonSerializable)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Shareable", class_Cbor_Shareable_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_FINAL;
	zend_class_implements(class_entry, 1, class_entry_JsonSerializable);

	zval property_value_default_value;
	ZVAL_UNDEF(&property_value_default_value);
	zend_string *property_value_name = zend_string_init("value", sizeof("value") - 1, 1);
	zend_declare_typed_property(class_entry, property_value_name, &property_value_default_value, ZEND_ACC_PUBLIC, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_ANY));
	zend_string_release(property_value_name);

	return class_entry;
}

static zend_class_entry *register_class_Cbor_Decoder(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "Cbor", "Decoder", class_Cbor_Decoder_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
	class_entry->ce_flags |= ZEND_ACC_NOT_SERIALIZABLE;

	return class_entry;
}
