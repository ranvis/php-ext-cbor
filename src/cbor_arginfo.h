/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 731e37372f72a62a02f67691eb68875decbb0aaa */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_cbor_encode, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "CBOR_BYTE | CBOR_KEY_BYTE")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 1, "[\'max_depth\' => 64, \'string_ref\' => false]")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_cbor_decode, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, data, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "CBOR_BYTE | CBOR_KEY_BYTE")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 1, "[\'max_depth\' => 64, \'max_size\' => 65536, \'string_ref\' => true, \'shared_ref\' => true]")
ZEND_END_ARG_INFO()


ZEND_FUNCTION(cbor_encode);
ZEND_FUNCTION(cbor_decode);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(cbor_encode, arginfo_cbor_encode)
	ZEND_FE(cbor_decode, arginfo_cbor_decode)
	ZEND_FE_END
};
