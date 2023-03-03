/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

static void convert_xz_xint_to_string(xzval *value)
{
	assert(XZ_ISXXINT_P(value));
	char buf[CBOR_INT_BUF_SIZE];
	size_t len = cbor_int_to_str(buf, XZ_XINT_P(value), Z_TYPE_P(value) != IS_X_UINT);
	ZVAL_STRINGL(value, buf, len);
}

static void zv_ctx_init(dec_context *ctx)
{
	ZVAL_UNDEF(&ctx->u.zv.root);
	ctx->u.zv.srns = NULL;
	if (ctx->args.shared_ref) {
		ctx->u.zv.refs = zend_new_array(0);
	}
}

static void zv_ctx_free(dec_context *ctx)
{
	FREE_LINKED_LIST(srns_item, ctx->u.zv.srns, free_srns_item);
	if (ctx->args.shared_ref) {
		zend_array_destroy(ctx->u.zv.refs);
	}
	zval_ptr_dtor(&ctx->u.zv.root);
}

static void zv_si_free(stack_item *item_)
{
	stack_item_zv *item = (stack_item_zv *)item_;
	if (item->base.si_type & SI_TYPE_STRING_MASK) {
		smart_str_free(&item->v.str);
	} else if (item->base.si_type == SI_TYPE_MAP) {
		zval_ptr_dtor(&item->v.map.dest);
		XZVAL_PURIFY(&item->v.map.key);
		zval_ptr_dtor(&item->v.map.key);
	} else if (item->base.si_type == SI_TYPE_TAG) {
		/* nothing */
	} else if (item->base.si_type == SI_TYPE_TAG_HANDLED) {
		if (tag_handlers[item->v.tag_h.thi].h_free) {
			(*tag_handlers[item->v.tag_h.thi].h_free)(item);
		}
	} else if (item->base.si_type) {
		zval_ptr_dtor(&item->v.value);
	}
}

static void zv_si_push(dec_context *ctx, stack_item *item, stack_item *parent_item)
{
	if (parent_item) {
		SI_CALL_CHILD_HANDLER((stack_item_zv *)parent_item, ctx, (stack_item_zv *)item, (stack_item_zv *)parent_item);
	}
}

static void zv_si_pop(dec_context *ctx, stack_item *item)
{
}

static cbor_error zv_dec_finish(dec_context *ctx, cbor_decode_args *args, cbor_error error, zval *value)
{
	if (!error) {
		ZVAL_COPY_VALUE(value, &ctx->u.zv.root);
		if (UNEXPECTED(Z_TYPE_P(value) == IS_REFERENCE)) {
			zend_unwrap_reference(value);
		}
	} else {
		args->error_arg = ctx->args.error_arg;
		zval_ptr_dtor(&ctx->u.zv.root);
	}
	ZVAL_UNDEF(&ctx->u.zv.root);
	return error;
}

static void zv_stack_push_counted(dec_context *ctx, si_type_code si_type, zval *value, uint32_t count)
{
	stack_item_zv *item = stack_new_item(ctx, si_type, count);
	ZVAL_COPY_VALUE(&item->v.value, value);
	stack_push_item(ctx, &item->base);
}

static void zv_stack_push_xstring(dec_context *ctx, si_type_code si_type)
{
	stack_item_zv *item = stack_new_item(ctx, si_type, 0);
	stack_push_item(ctx, &item->base);
}

static void zv_stack_push_map(dec_context *ctx, si_type_code si_type, zval *value, uint32_t count)
{
	stack_item_zv *item = stack_new_item(ctx, si_type, count);
	ZVAL_COPY_VALUE(&item->v.map.dest, value);
	ZVAL_UNDEF(&item->v.map.key);
	Z_EXTRA(item->v.map.dest) = 0;
	stack_push_item(ctx, &item->base);
}

static void zv_stack_push_tag(dec_context *ctx, zend_long tag_id)
{
	stack_item_zv *item = stack_new_item(ctx, SI_TYPE_TAG, 1);
	item->v.tag_id = tag_id;
	stack_push_item(ctx, &item->base);
}

static bool zv_append(dec_context *ctx, xzval *value);

static bool zv_append_counted(dec_context *ctx, zval *value)
{
	stack_item *item = stack_pop_item(ctx);
	bool result = zv_append(ctx, value);
	stack_free_item(ctx, item);
	return result;
}

static bool zv_append_to_array(dec_context *ctx, xzval *value, stack_item_zv *item)
{
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_VALUE, INT_RANGE));
	}
	if (!item->base.count && zend_hash_num_elements(Z_ARRVAL(item->v.value)) >= ctx->args.max_size) {
		/* max size of indefinite-length array is enforced on append */
		RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (add_next_index_zval(&item->v.value, value) != SUCCESS) {
		RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
	}
	Z_TRY_ADDREF_P(value);
	if (item->base.count && --item->base.count == 0) {
		ASSERT_STACK_ITEM_IS_TOP(item);
		return zv_append_counted(ctx, &item->v.value);
	}
	return true;
}

static bool zv_append_to_map(dec_context *ctx, xzval *value, stack_item_zv *item)
{
	if (Z_ISUNDEF(item->v.map.key)) {
		if (!item->base.count && ++Z_EXTRA(item->v.map.dest) > ctx->args.max_size) {
			/* max size of indefinite-length map is enforced on append */
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_SIZE);
		}
		switch (Z_TYPE_P(value)) {
		case IS_LONG:
		case IS_X_UINT:
		case IS_X_NINT:
			if (!(ctx->args.flags & CBOR_INT_KEY)) {
				RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, INT_KEY));
			}
			ZVAL_COPY_VALUE(&item->v.map.key, value);
			break;
		case IS_STRING:
			ZVAL_COPY(&item->v.map.key, value);
			break;
		case IS_NULL:
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, NULL));
		case IS_FALSE:
		case IS_TRUE:
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, BOOL));
		case IS_DOUBLE:
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, FLOAT));
		case IS_ARRAY:
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, ARRAY));
		case IS_OBJECT: {
			zend_class_entry *ce = Z_OBJ_P(value)->ce;
			if (ce == CBOR_CE(undefined)) {
				RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, UNDEF));
			} else if (ce == CBOR_CE(float16) || ce == CBOR_CE(float32)) {
				RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, FLOAT));
			} else if (ce == CBOR_CE(tag) || ce == CBOR_CE(shareable)) {
				RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, TAG));
			}
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, OBJECT));
		}
		default:
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
		return true;
	}
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_VALUE, INT_RANGE));
	}
	if (XZ_ISXXINT(item->v.map.key)) {
		convert_xz_xint_to_string(&item->v.map.key);
	}
	if (Z_TYPE(item->v.map.dest) == IS_OBJECT) {
		if (Z_TYPE(item->v.map.key) == IS_LONG) {
			char num_str[ZEND_LTOA_BUF_LEN];
			ZEND_LTOA(Z_LVAL(item->v.map.key), num_str, sizeof num_str);
			ZVAL_STRINGL(&item->v.map.key, num_str, strlen(num_str));
		}
		assert(Z_TYPE_P(value) != IS_REFERENCE || Z_TYPE_P(Z_REFVAL_P(value)) != IS_OBJECT);
		if (Z_STRLEN(item->v.map.key) >= 1 && Z_STRVAL(item->v.map.key)[0] == '\0') {
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_VALUE, RESERVED_PROP_NAME));
		}
		if (ctx->args.flags & CBOR_MAP_NO_DUP_KEY
				&& zend_std_has_property(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), ZEND_PROPERTY_EXISTS, NULL)) {
			RETURN_CB_ERROR_B(CBOR_ERROR_DUPLICATE_KEY);
		}
		zend_std_write_property(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), value, NULL);
	} else {  /* IS_ARRAY */
		if (Z_TYPE(item->v.map.key) == IS_LONG) {
			zend_ulong index = (zend_ulong)Z_LVAL(item->v.map.key);
			if (ctx->args.flags & CBOR_MAP_NO_DUP_KEY
					&& zend_hash_index_exists(Z_ARRVAL(item->v.map.dest), index)) {
				RETURN_CB_ERROR_B(CBOR_ERROR_DUPLICATE_KEY);
			}
			/* The argument accepts zend_ulong as underlying hash table does, but the actual index visible for PHP script is still zend_long. */
			add_index_zval(&item->v.map.dest, index, value);
		} else {
			if (ctx->args.flags & CBOR_MAP_NO_DUP_KEY
					&& zend_symtable_exists(Z_ARRVAL(item->v.map.dest), Z_STR(item->v.map.key))) {
				RETURN_CB_ERROR_B(CBOR_ERROR_DUPLICATE_KEY);
			}
			add_assoc_zval_ex(&item->v.map.dest, Z_STRVAL(item->v.map.key), Z_STRLEN(item->v.map.key), value);
		}
		Z_TRY_ADDREF_P(value);
	}
	XZVAL_PURIFY(&item->v.map.key);
	zval_ptr_dtor(&item->v.map.key);
	ZVAL_UNDEF(&item->v.map.key);
	if (item->base.count && --item->base.count == 0) {
		ASSERT_STACK_ITEM_IS_TOP(item);
		return zv_append_counted(ctx, &item->v.map.dest);
	}
	return true;
}

static bool zv_append_to_tag(dec_context *ctx, xzval *value, stack_item_zv *item)
{
	zval container;
	bool result;
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_VALUE, INT_RANGE));
	}
	if (object_init_ex(&container, CBOR_CE(tag)) != SUCCESS) {
		RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
	}
	zval tag_id;
	ZVAL_LONG(&tag_id, item->v.tag_id);
	zend_call_known_instance_method_with_2_params(CBOR_CE(tag)->constructor, Z_OBJ(container), NULL, &tag_id, value);
	ASSERT_STACK_ITEM_IS_TOP(item);
	stack_pop_item(ctx);
	stack_free_item(ctx, &item->base);
	result = zv_append(ctx, &container);
	zval_ptr_dtor_nogc(&container);
	return result;
}

static bool zv_append_to_tag_handled(dec_context *ctx, xzval *value, stack_item_zv *item)
{
	zval tmp_v;
	xzval *orig_v = value;
	bool result;
	ASSERT_STACK_ITEM_IS_TOP(item);
	stack_pop_item(ctx);
	if (tag_handlers[item->v.tag_h.thi].h_exit)  {
		assert(!ctx->cb_error);
		value = (*tag_handlers[item->v.tag_h.thi].h_exit)(ctx, value, item, &tmp_v);
		/* exit handler may return the new value, that caller frees */
	}
	result = !ctx->cb_error && zv_append(ctx, value);
	if (value != orig_v) {
		zval_ptr_dtor(value);
	}
	stack_free_item(ctx, &item->base);
	return result;
}

static bool zv_append(dec_context *ctx, xzval *value)
{
	stack_item_zv *item = (stack_item_zv *)ctx->stack_top;
	if (item == NULL) {
		if (XZ_ISXXINT_P(value)) {
			RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_UNSUPPORTED_VALUE, INT_RANGE));
		}
		ZVAL_COPY(&ctx->u.zv.root, value);
		return true;
	}
	switch (item->base.si_type) {
	case SI_TYPE_ARRAY:
		return zv_append_to_array(ctx, value, item);
	case SI_TYPE_MAP:
		return zv_append_to_map(ctx, value, item);
	case SI_TYPE_TAG:
		return zv_append_to_tag(ctx, value, item);
	case SI_TYPE_TAG_HANDLED:
		return zv_append_to_tag_handled(ctx, value, item);
	}
	if (item->base.si_type & SI_TYPE_STRING_MASK) {
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_SYNTAX, INDEF_STRING_CHUNK_TYPE));
	}
	RETURN_CB_ERROR_B(CBOR_ERROR_SYNTAX);
}

#if -(ZEND_LONG_MIN + 1) != ZEND_LONG_MAX
#error "abs(ZEND_LONG_MIN) must be (ZEND_LONG_MAX + 1)"
#endif
#if SIZEOF_ZEND_LONG == 4
#define TEST_OVERFLOW_XINT32(val)  ((val) > ZEND_LONG_MAX)
#define TEST_OVERFLOW_XINT64(val)  true
#elif SIZEOF_ZEND_LONG == 8
#define TEST_OVERFLOW_XINT32(val)  false
#define TEST_OVERFLOW_XINT64(val)  ((val) > ZEND_LONG_MAX)
#else
#error unimplemented
#endif

static void zv_proc_uint32(dec_context *ctx, uint32_t val)
{
	xzval value;
	if (TEST_OVERFLOW_XINT32(val)) {
		XZVAL_UINT(&value, (uint64_t)val);
	} else {
		ZVAL_LONG(&value, val);
	}
	zv_append(ctx, &value);
}

static void zv_proc_uint64(dec_context *ctx, uint64_t val)
{
	xzval value;
	if (TEST_OVERFLOW_XINT64(val)) {
		XZVAL_UINT(&value, val);
	} else {
		ZVAL_LONG(&value, (zend_long)val);
	}
	zv_append(ctx, &value);
}

static void zv_proc_nint32(dec_context *ctx, uint32_t val)
{
	xzval value;
	if (TEST_OVERFLOW_XINT32(val)) {
		XZVAL_NINT(&value, (uint64_t)val);
	} else {
		ZVAL_LONG(&value, -(zend_long)val - 1);
	}
	zv_append(ctx, &value);
}

static void zv_proc_nint64(dec_context *ctx, uint64_t val)
{
	zval value;
	if (TEST_OVERFLOW_XINT64(val)) {
		XZVAL_NINT(&value, val);
	} else {
		ZVAL_LONG(&value, -(zend_long)val - 1);
	}
	zv_append(ctx, &value);
}

static bool create_value_object(zval *container, zval *value, zend_class_entry *ce)
{
	if (object_init_ex(container, ce) != SUCCESS) {
		return false;
	}
	zend_call_known_instance_method_with_1_params(ce->constructor, Z_OBJ_P(container), NULL, value);
	return true;
}

static bool zv_append_string_item(dec_context *ctx, zval *value, bool is_text, bool is_indef)
{
	zval container;
	int type_flag = is_text ? CBOR_TEXT : CBOR_BYTE;
	zend_class_entry *string_ce = is_text ? CBOR_CE(text) : CBOR_CE(byte);
	stack_item_zv *item = (stack_item_zv *)ctx->stack_top;
	bool result;
	ZVAL_NULL(&container);
	if (item && item->base.si_type == SI_TYPE_MAP && Z_ISUNDEF(item->v.map.key)) {  /* is map key */
		bool is_valid_type = is_text ? (ctx->args.flags & CBOR_KEY_TEXT) : (ctx->args.flags & CBOR_KEY_BYTE);
		if (!is_valid_type) {
			cbor_error error = is_text ? E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, TEXT) : E_DESC(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, BYTE);
			RETURN_CB_ERROR_B(error);
		}
	} else if (ctx->args.flags & type_flag) {  /* to PHP string */
		/* do nothing */
	} else {
		zend_object *obj = cbor_xstring_create(string_ce);
		cbor_xstring_set_value(obj, Z_STR_P(value));
		ZVAL_OBJ(&container, obj);
		value = &container;
	}
	if (!is_indef && item && item->thi_data != THI_NONE) {
		(*tag_handlers[item->thi_data].h_data)(ctx, item, DATA_TYPE_STRING, value);
	}
	result = zv_append(ctx, value);
	if (!Z_ISNULL(container)) {
		zval_ptr_dtor_nogc(&container);
	}
	return result;
}

static void zv_do_xstring(dec_context *ctx, const char *val, uint64_t length, bool is_text)
{
	zval value;
	stack_item_zv *item = (stack_item_zv *)ctx->stack_top;
#if UINT64_MAX > SIZE_MAX
	if (length > SIZE_MAX) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
#endif
	if (is_text && !(ctx->args.flags & CBOR_UNSAFE_TEXT)
			&& !is_utf8((uint8_t *)val, (size_t)length)) {
		RETURN_CB_ERROR(CBOR_ERROR_UTF8);
	}
	if (item != NULL && item->base.si_type & SI_TYPE_STRING_MASK) {
		/* indefinite-length string */
		si_type_code str_si_type = is_text ? SI_TYPE_TEXT : SI_TYPE_BYTE;
		if (item->base.si_type == str_si_type) {
			if (length) {
				if (UNEXPECTED(length > SIZE_MAX - smart_str_get_len(&item->v.str))) {
					RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
				}
				smart_str_appendl(&item->v.str, (const char *)val, (size_t)length);
			}
			return;
		}
		RETURN_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, INCONSISTENT_STRING_TYPE));
	}
	ZVAL_STRINGL_FAST(&value, (const char *)val, (size_t)length);
	zv_append_string_item(ctx, &value, is_text, false);
	zval_ptr_dtor_str(&value);
}

static void zv_proc_text_string(dec_context *ctx, const char *val, uint64_t length)
{
	zv_do_xstring(ctx, val, length, true);
}

static void zv_proc_text_string_start(dec_context *ctx)
{
	zv_stack_push_xstring(ctx, SI_TYPE_TEXT);
}

static void zv_proc_byte_string(dec_context *ctx, const char *val, uint64_t length)
{
	zv_do_xstring(ctx, val, length, false);
}

static void zv_proc_byte_string_start(dec_context *ctx)
{
	zv_stack_push_xstring(ctx, SI_TYPE_BYTE);
}

static void zv_proc_array_start(dec_context *ctx, uint32_t count)
{
	zval value;
	if (count > ctx->args.max_size) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->mem->limit && ctx->mem->offset + count > ctx->mem->limit) {
		RETURN_CB_ERROR(CBOR_ERROR_TRUNCATED_DATA);
	}
	if (count) {
		array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
		zv_stack_push_counted(ctx, SI_TYPE_ARRAY, &value, (uint32_t)count);
	} else {
		ZVAL_EMPTY_ARRAY(&value);
		zv_append(ctx, &value);
		zval_ptr_dtor_nogc(&value);
	}
}

static void zv_proc_indef_array_start(dec_context *ctx)
{
	zval value;
	array_init(&value);
	zv_stack_push_counted(ctx, SI_TYPE_ARRAY, &value, 0);
}

static void zv_proc_map_start(dec_context *ctx, uint32_t count)
{
	zval value;
	if (count > ctx->args.max_size) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->mem->limit && ctx->mem->offset + count > ctx->mem->limit) {
		RETURN_CB_ERROR(CBOR_ERROR_TRUNCATED_DATA);
	}
	if (ctx->args.flags & CBOR_MAP_AS_ARRAY) {
		if (count) {
			array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
		} else {
			ZVAL_EMPTY_ARRAY(&value);
		}
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	if (count) {
		zv_stack_push_map(ctx, SI_TYPE_MAP, &value, (uint32_t)count);
	} else {
		zv_append(ctx, &value);
		zval_ptr_dtor_nogc(&value);
	}
}

static void zv_proc_indef_map_start(dec_context *ctx)
{
	zval value;
	if (ctx->args.flags & CBOR_MAP_AS_ARRAY) {
		array_init(&value);
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	zv_stack_push_map(ctx, SI_TYPE_MAP, &value, 0);
}

static bool zv_do_tag_enter(dec_context *ctx, zend_long tag_id);

static void zv_proc_tag(dec_context *ctx, uint64_t val)
{
	if (val > ZEND_LONG_MAX) {
		RETURN_CB_ERROR(E_DESC(CBOR_ERROR_UNSUPPORTED_VALUE, INT_RANGE));
	}
	if (!zv_do_tag_enter(ctx, (zend_long)val)) {
		zv_stack_push_tag(ctx, (zend_long)val);
	}
}

static void zv_do_floatx(dec_context *ctx, uint32_t raw, int bits)
{
	zval container, value;
	zend_object *obj;
	int type_flag = bits == 32 ? CBOR_FLOAT32 : CBOR_FLOAT16;
	zend_class_entry *float_ce = bits == 32 ? CBOR_CE(float32) : CBOR_CE(float16);
	if (ctx->args.flags & type_flag) {
		if (bits == 32) {
			binary32_alias binary32;
			binary32.i = raw;
			ZVAL_DOUBLE(&value, (double)binary32.f);
		} else {
			ZVAL_DOUBLE(&value, cbor_from_float16((uint16_t)raw));
		}
		zv_append(ctx, &value);
		return;
	}
	obj = cbor_floatx_create(float_ce);
	cbor_floatx_set_value(obj, NULL, raw);  /* always succeeds */
	ZVAL_OBJ(&container, obj);
	zv_append(ctx, &container);
	zval_ptr_dtor_nogc(&container);
}

static void zv_proc_float16(dec_context *ctx, uint16_t val)
{
	zv_do_floatx(ctx, val, 16);
}

static void zv_proc_float32(dec_context *ctx, uint32_t val)
{
	zv_do_floatx(ctx, val, 32);
}

static void zv_proc_float64(dec_context *ctx, double val)
{
	zval value;
	ZVAL_DOUBLE(&value, val);
	zv_append(ctx, &value);
}

static void zv_proc_null(dec_context *ctx)
{
	zval value;
	ZVAL_NULL(&value);
	zv_append(ctx, &value);
}

static void zv_proc_undefined(dec_context *ctx)
{
	zval value;
	ZVAL_OBJ(&value, cbor_get_undef());
	zv_append(ctx, &value);
	Z_DELREF(value);
}

static void zv_proc_simple(dec_context *ctx, uint32_t val)
{
	RETURN_CB_ERROR(E_DESC(CBOR_ERROR_UNSUPPORTED_TYPE, SIMPLE));
}

static void zv_proc_boolean(dec_context *ctx, bool val)
{
	zval value;
	ZVAL_BOOL(&value, val);
	zv_append(ctx, &value);
}

static void zv_proc_indef_break(dec_context *ctx, stack_item *item_)
{
	stack_item_zv *item = (stack_item_zv *)item_;
	if (item->base.si_type & SI_TYPE_STRING_MASK) {
		bool is_text = item->base.si_type == SI_TYPE_TEXT;
		zval value;
		ZVAL_STR(&value, smart_str_extract(&item->v.str));
		zv_append_string_item(ctx, &value, is_text, true);
		zval_ptr_dtor_str(&value);
	} else {  /* SI_TYPE_ARRAY, SI_TYPE_MAP, SI_TYPE_TAG, SI_TYPE_TAG_HANDLED */
		if (UNEXPECTED(item->base.count != 0)  /* definite-length */
				|| (item->base.si_type == SI_TYPE_MAP && UNEXPECTED(!Z_ISUNDEF(item->v.map.key)))) {  /* value is expected */
			THROW_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, BREAK_UNEXPECTED));
		}
		assert(item->base.si_type == SI_TYPE_ARRAY || item->base.si_type == SI_TYPE_MAP);
		zv_append(ctx, &item->v.value);
	}
FINALLY: ;
}

static void tag_handler_str_ref_ns_data(dec_context *ctx, stack_item_zv *item, data_type type, zval *value)
{
	if (type == DATA_TYPE_STRING) {
		HashTable *str_table = ctx->u.zv.srns->str_table;
		size_t str_len;
		if (Z_TYPE_P(value) == IS_STRING) {
			str_len = Z_STRLEN_P(value);
		} else if (EXPECTED(Z_TYPE_P(value) == IS_OBJECT)) {
			str_len = ZSTR_LEN(cbor_get_xstring_value(value));
		} else {
			RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
		}
		if (cbor_is_len_string_ref(str_len, zend_hash_num_elements(str_table))) {
			if (zend_hash_next_index_insert(str_table, value) == NULL) {
				RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
			}
			Z_ADDREF_P(value);
		}
	}
}

static void tag_handler_str_ref_ns_child(dec_context *ctx, stack_item_zv *item, stack_item_zv *self)
{
	if (item->base.si_type & SI_TYPE_STRING_MASK) {
		/* chunks of indefinite length string */
		return;
	}
	SI_SET_CHILD_HANDLER(item, THI_STR_REF_NS);
	SI_SET_DATA_HANDLER(item, THI_STR_REF_NS);
}

static xzval *tag_handler_str_ref_ns_exit(dec_context *ctx, xzval *value, stack_item_zv *item, zval *tmp_v)
{
	srns_item *srns = ctx->u.zv.srns;
	ctx->u.zv.srns = srns->prev_item;
	item->v.tag_h.v.srns_detached = srns;
	return value;
}

static void free_srns_item(srns_item *srns)
{
	zend_array_destroy(srns->str_table);
	efree(srns);
}

static void tag_handler_str_ref_ns_free(stack_item_zv *item)
{
	if (item->v.tag_h.v.srns_detached) {
		free_srns_item(item->v.tag_h.v.srns_detached);
	}
}

static bool tag_handler_str_ref_ns_enter(dec_context *ctx, stack_item_zv *item)
{
	srns_item *srns = emalloc(sizeof *srns);
	SI_SET_DATA_HANDLER(item, THI_STR_REF_NS);
	SI_SET_CHILD_HANDLER(item, THI_STR_REF_NS);
	item->v.tag_h.v.srns_detached = NULL;
	srns->str_table = zend_new_array(0);
	srns->prev_item = ctx->u.zv.srns;
	ctx->u.zv.srns = srns;
	return true;
}

static xzval *tag_handler_str_ref_exit(dec_context *ctx, xzval *value, stack_item_zv *item, zval *tmp_v)
{
	zend_long index;
	zval *str;
	if (Z_TYPE_P(value) != IS_LONG) {
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_TYPE, STR_REF_NOT_INT));
	}
	index = Z_LVAL_P(value);
	if (index < 0) {
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_VALUE, STR_REF_RANGE));
	}
	if ((str = zend_hash_index_find(ctx->u.zv.srns->str_table, index)) == NULL) {
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_VALUE, STR_REF_RANGE));
	}
	assert(Z_TYPE_P(value) == IS_LONG);
	ZVAL_COPY(tmp_v, str);
	return tmp_v;
}

static bool tag_handler_str_ref_enter(dec_context *ctx, stack_item_zv *item)
{
	if (!ctx->u.zv.srns) {
		/* outer stringref-namespace is expected */
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_TAG_SYNTAX, STR_REF_NO_NS));
	}
	return true;
}

static void tag_handler_shareable_child(dec_context *ctx, stack_item_zv *item, stack_item_zv *self)
{
	zval *real_v, tmp_v;
	assert(self->base.si_type == SI_TYPE_TAG_HANDLED);
	assert(Z_TYPE(self->v.tag_h.v.shareable.value) == IS_NULL);
	if (item->base.si_type == SI_TYPE_MAP && Z_TYPE(item->v.map.dest) == IS_OBJECT) {
		real_v = &item->v.map.dest;
		ZVAL_COPY(&self->v.tag_h.v.shareable.value, real_v);
	} else {
		if (ctx->args.shared_ref == OPT_SHAREABLE) {
			ZVAL_TRUE(&tmp_v);
			if (!create_value_object(&self->v.tag_h.v.shareable.value, &tmp_v, CBOR_CE(shareable))) {
				RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
			}
			real_v = &self->v.tag_h.v.shareable.value;
		} else if (ctx->args.shared_ref == OPT_UNSAFE_REF) {
			ZVAL_UNDEF(&tmp_v);
			real_v = &self->v.tag_h.v.shareable.value;
			ZVAL_NEW_REF(real_v, &tmp_v);
		} else {
			RETURN_CB_ERROR(E_DESC(CBOR_ERROR_TAG_TYPE, SHARE_INCOMPATIBLE));
		}
	}
	if (zend_hash_index_update(ctx->u.zv.refs, self->v.tag_h.v.shareable.index, real_v) == NULL) {
		zval_ptr_dtor(real_v);
		RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
	}
}

static xzval *tag_handler_shareable_exit(dec_context *ctx, xzval *value, stack_item_zv *item, zval *tmp_v)
{
	zval *real_v;
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_UNSUPPORTED_VALUE, INT_RANGE));
	}
	if (Z_TYPE(item->v.tag_h.v.shareable.value) == IS_NULL) {
		/* child handler is not called */
		if (Z_TYPE_P(value) == IS_OBJECT || ctx->args.shared_ref == OPT_UNSAFE_REF) {
			if (Z_TYPE_P(value) != IS_OBJECT) {
				real_v = tmp_v;
				ZVAL_COPY(real_v, value);
				ZVAL_MAKE_REF(real_v);
			} else {
				real_v = value;
			}
		} else if (ctx->args.shared_ref == OPT_SHAREABLE) {
			real_v = tmp_v;
			if (!create_value_object(real_v, value, CBOR_CE(shareable))) {
				RETURN_CB_ERROR_V(value, CBOR_ERROR_INTERNAL);
			}
		} else {
			RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_TYPE, SHARE_INCOMPATIBLE));
		}
		if (zend_hash_index_update(ctx->u.zv.refs, item->v.tag_h.v.shareable.index, real_v) == NULL) {
			if (real_v != value) {
				zval_ptr_dtor(real_v);
			}
			RETURN_CB_ERROR_V(value, CBOR_ERROR_INTERNAL);
		}
		Z_ADDREF_P(real_v);
	} else {
		real_v = tmp_v;
		ZVAL_COPY(real_v, &item->v.tag_h.v.shareable.value);
		if (ctx->args.shared_ref == OPT_SHAREABLE) {
			assert(Z_TYPE_P(real_v) == IS_OBJECT);
			zend_update_property_ex(CBOR_CE(shareable), Z_OBJ_P(real_v), ZSTR_KNOWN(ZEND_STR_VALUE), value);
		} else if (ctx->args.shared_ref == OPT_UNSAFE_REF) {
			assert(Z_TYPE_P(real_v) == IS_REFERENCE);
			zval *shareable_ref = real_v;
			ZVAL_DEREF(shareable_ref);
			ZVAL_COPY(shareable_ref, value);  /* move into ref content */
		}
	}
	return real_v;
}

static bool tag_handler_shareable_enter(dec_context *ctx, stack_item_zv *item)
{
	if (ctx->stack_top && ctx->stack_top->si_type == SI_TYPE_TAG_HANDLED && ((stack_item_zv *)ctx->stack_top)->v.tag_h.id == CBOR_TAG_SHAREABLE) {
		/* nested shareable */
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_TAG_SYNTAX, SHARE_NESTED));
	}
	SI_SET_CHILD_HANDLER(item, THI_SHAREABLE);
	ZVAL_NULL(&item->v.tag_h.v.shareable.value); /* cannot be undef if adding to HashTable */
	item->v.tag_h.v.shareable.index = zend_hash_num_elements(ctx->u.zv.refs);
	if (zend_hash_next_index_insert(ctx->u.zv.refs, &item->v.tag_h.v.shareable.value) == NULL) {
		RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
	}
	return true;
}

static xzval *tag_handler_shared_ref_exit(dec_context *ctx, zval *value, stack_item_zv *item, zval *tmp_v)
{
	zend_long index;
	zval *ref_v;
	if (Z_TYPE_P(value) != IS_LONG) {
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_TYPE, SHARE_NOT_INT));
	}
	index = Z_LVAL_P(value);
	if (index < 0) {
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_VALUE, SHARE_RANGE));
	}
	if ((ref_v = zend_hash_index_find(ctx->u.zv.refs, index)) == NULL) {
		/* the spec doesn't prohibit referencing future shareable, but it is unlikely */
		RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_VALUE, SHARE_RANGE));
	}
	assert(Z_TYPE_P(value) == IS_LONG);	/* zval_ptr_dtor(value); */
	Z_ADDREF_P(ref_v);
	return ref_v;  /* returning hash structure */
}

static bool tag_handler_shared_ref_enter(dec_context *ctx, stack_item_zv *item)
{
	return true;
}

static tag_handler_procs tag_handlers[THI_COUNT] = {
	{
		NULL,
	}, {
		&tag_handler_str_ref_ns_enter,
		&tag_handler_str_ref_ns_exit,
		&tag_handler_str_ref_ns_free,
		&tag_handler_str_ref_ns_data,
		&tag_handler_str_ref_ns_child,
	}, {
		&tag_handler_str_ref_enter,
		&tag_handler_str_ref_exit,
	}, {
		&tag_handler_shareable_enter,
		&tag_handler_shareable_exit,
		NULL,
		NULL,
		&tag_handler_shareable_child,
	}, {
		&tag_handler_shared_ref_enter,
		&tag_handler_shared_ref_exit,
	},
};

static bool zv_do_tag_enter(dec_context *ctx, zend_long tag_id)
{
	/* must return true if pushed to stack or an error occurred */
	tag_handler_index thi = THI_NONE;
	if (tag_id == CBOR_TAG_STRING_REF_NS && ctx->args.string_ref) {
		thi = THI_STR_REF_NS;
	} else if (tag_id == CBOR_TAG_STRING_REF && ctx->args.string_ref) {
		thi = THI_STR_REF;
	} else if (tag_id == CBOR_TAG_SHAREABLE && ctx->args.shared_ref) {
		thi = THI_SHAREABLE;
	} else if (tag_id == CBOR_TAG_SHARED_REF && ctx->args.shared_ref) {
		thi = THI_SHARED_REF;
	}
	if (thi != THI_NONE) {
		stack_item_zv *item = stack_new_item(ctx, SI_TYPE_TAG_HANDLED, 1);
		item->v.tag_h.id = tag_id;
		if (!(*tag_handlers[thi].h_enter)(ctx, item)) {
			ASSERT_ERROR_SET();
			stack_free_item(ctx, &item->base);
			return true;
		}
		item->v.tag_h.thi = thi;
		stack_push_item(ctx, &item->base);
		return true;
	}
	return false;
}

#define METHOD(name) zv_##name
#include "decode_base.h"
#undef METHOD
