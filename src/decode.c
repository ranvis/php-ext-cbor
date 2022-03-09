/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "php_cbor.h"
#include "utf8.h"

#define RETURN_CB_ERROR_B(error)  do { \
		assert(error); \
		ctx->cb_error = (error); \
		return false; \
	} while (false)
#define RETURN_CB_ERROR(error)  do { \
		assert(error); \
		ctx->cb_error = (error); \
		return; \
	} while (false)
#define THROW_CB_ERROR(error)  do { \
		ctx->cb_error = (error); \
		goto FINALLY; \
	} while (false)
#define ASSERT_ERROR_SET()  assert(ctx->cb_error)

#define STACK_ITEM_TOP(s)  (stack_item *)((s)->top ? zend_ptr_stack_top(s) : NULL)
#define STACK_ITEM_POP(s)  (stack_item *)((s)->top ? zend_ptr_stack_pop(s) : NULL)
#define STACK_NUM_ELEMENTS(s)  ((s)->top)

static struct cbor_callbacks callbacks;

typedef struct {
	php_cbor_decode_args args;
	php_cbor_error cb_error;
	size_t length;
	size_t offset;
	cbor_data data;
	zval root;
	zend_ptr_stack stack;  /* ptr stack is suitable to retain popped item */
} dec_context;

typedef enum {
	SI_TYPE_BYTE = 1,
	SI_TYPE_TEXT,
	SI_TYPE_ARRAY,
	SI_TYPE_MAP,
	SI_TYPE_TAG,
} si_type_t;

typedef struct {
	si_type_t si_type;
	uint32_t count;
	union stack_item_value_t {
		zval value;
		smart_str str;
		struct stack_item_value_map_t {
			zval dest;
			zval key;
		} map;
	} v;
} stack_item;

static php_cbor_error dec_zval(dec_context *ctx);
static void set_callbacks();

void php_cbor_minit_decode()
{
	set_callbacks();
}

static void stack_item_free(stack_item *item)
{
	if (item == NULL) {
		return;
	}
	if (item->si_type == SI_TYPE_BYTE || item->si_type == SI_TYPE_TEXT) {
		smart_str_free(&item->v.str);
	} else if (item->si_type == SI_TYPE_MAP) {
		zval_ptr_dtor(&item->v.map.dest);
		zval_ptr_dtor(&item->v.map.key);
	} else if (item->si_type == SI_TYPE_TAG) {
		/* nothing */
	} else if (item->si_type) {
		zval_ptr_dtor(&item->v.value);
	}
	efree(item);
}

static void free_stack_element(void *vp_item)
{
	stack_item *item = (stack_item *)vp_item;
	stack_item_free(item);
}

static void stack_push_item(dec_context *ctx, stack_item *item)
{
	int cur_depth = STACK_NUM_ELEMENTS(&ctx->stack);
	if (cur_depth > ctx->args.max_depth) {
		stack_item_free(item);
		RETURN_CB_ERROR(PHP_CBOR_ERROR_DEPTH);
	}
	zend_ptr_stack_push(&ctx->stack, item);
}

static stack_item *stack_new_item(si_type_t si_type, uint32_t count)
{
	stack_item *item = (stack_item *)emalloc(sizeof(stack_item));
	item->si_type = si_type;
	item->count = count;
	memset(&item->v, 0, sizeof item->v);
	return item;
}

static void stack_push_counted(dec_context *ctx, si_type_t si_type, zval *value, uint32_t count)
{
	stack_item *item = stack_new_item(si_type, count);
	item->v.value = *value;
	stack_push_item(ctx, item);
}

static void stack_push_map(dec_context *ctx, si_type_t si_type, zval *value, uint32_t count)
{
	stack_item *item = stack_new_item(si_type, count);
	item->v.map.dest = *value;
	ZVAL_UNDEF(&item->v.map.key);
	stack_push_item(ctx, item);
}

static void stack_push_tag(dec_context *ctx, zend_long tag)
{
	stack_item *item = stack_new_item(SI_TYPE_TAG, 1);
	ZVAL_LONG(&item->v.value, tag);
	stack_push_item(ctx, item);
}

static void stack_push_xstring(dec_context *ctx, si_type_t si_type)
{
	stack_item *item = stack_new_item(si_type, 0);
	stack_push_item(ctx, item);
}

php_cbor_error php_cbor_decode(zend_string *data, zval *value, php_cbor_decode_args *args)
{
	php_cbor_error error;
	dec_context ctx;
	cbor_data ptr = (cbor_data)ZSTR_VAL(data);
	size_t length = ZSTR_LEN(data);
	if ((args->flags & PHP_CBOR_FLOAT16 && args->flags & PHP_CBOR_FLOAT32)) {
		return PHP_CBOR_ERROR_INVALID_FLAGS;
	}
	if (args->max_depth <= 0 || args->max_depth > 10000) {
		return PHP_CBOR_ERROR_INVALID_FLAGS;
	}
	zend_ptr_stack_init(&ctx.stack);
	ctx.offset = 0;
	if (!(args->flags & PHP_CBOR_SELF_DESCRIBE) && length >= 3
			&& !memcmp(ptr, PHP_CBOR_SELF_DESCRIBE_DATA, sizeof PHP_CBOR_SELF_DESCRIBE_DATA - 1)) {
		ctx.offset = 3;
	}
	ZVAL_UNDEF(&ctx.root);
	ctx.data = ptr;
	ctx.length = length;
	ctx.args = *args;
	error = dec_zval(&ctx);
	zend_ptr_stack_apply(&ctx.stack, &free_stack_element);
	zend_ptr_stack_destroy(&ctx.stack);
	if (!error && ctx.offset != ctx.length) {
		error = PHP_CBOR_ERROR_EXTRANEOUS_DATA;
		ctx.args.error_arg = ctx.offset;
	}
	if (!error) {
		*value = ctx.root;
	} else {
		args->error_arg = ctx.args.error_arg;
	}
	return error;
}

static php_cbor_error dec_zval(dec_context *ctx)
{
	struct cbor_decoder_result result;
	ctx->cb_error = 0;
	do {
		if (ctx->offset >= ctx->length) {
			ctx->args.error_arg = ctx->offset;
			return PHP_CBOR_ERROR_TRUNCATED_DATA;
		}
		result = cbor_stream_decode(ctx->data + ctx->offset, ctx->length - ctx->offset, &callbacks, ctx);
		switch (result.status) {
		case CBOR_DECODER_FINISHED:
			ctx->offset += result.read;
			break;
		case CBOR_DECODER_NEDATA:
			ctx->args.error_arg = ctx->offset;
			return PHP_CBOR_ERROR_TRUNCATED_DATA;
		case CBOR_DECODER_ERROR:
			ctx->args.error_arg = ctx->offset;
			return PHP_CBOR_ERROR_MALFORMED_DATA;
		}
		if (ctx->cb_error) {
			ctx->args.error_arg = ctx->offset;
			return ctx->cb_error;
		}
	} while (STACK_NUM_ELEMENTS(&ctx->stack));
	return 0;
}

static bool append_item(dec_context *ctx, zval *value);

static bool append_item_to_array(dec_context *ctx, zval *value, stack_item *item)
{
	if (add_next_index_zval(&item->v.value, value) != SUCCESS) {
		RETURN_CB_ERROR_B(PHP_CBOR_ERROR_INTERNAL);
	}
	if (item->count && --item->count == 0) {
		bool result;
		assert(STACK_ITEM_TOP(&ctx->stack) == item);
		STACK_ITEM_POP(&ctx->stack);
		result = append_item(ctx, &item->v.value);
		if (result) {
			Z_TRY_ADDREF(item->v.value);
		}
		stack_item_free(item);
		return result;
	}
	return true;
}

static bool append_item_to_map(dec_context *ctx, zval *value, stack_item *item)
{
	char num_str[ZEND_LTOA_BUF_LEN];
	if (Z_ISUNDEF(item->v.map.key)) {
		switch (Z_TYPE_P(value)) {
		case IS_LONG:
			if (!(ctx->args.flags & PHP_CBOR_INT_KEY)) {
				RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
			}
			if (Z_LVAL_P(value) < 0) {
				RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_VALUE);
			}
			ZEND_LTOA(Z_LVAL_P(value), num_str, sizeof num_str);
			ZVAL_STRINGL(&item->v.map.key, num_str, strlen(num_str));
			break;
		case IS_STRING:
			item->v.map.key = *value;
			break;
		default:
			RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
		return true;
	}
	if (Z_TYPE(item->v.map.dest) == IS_OBJECT) {
		zend_std_write_property(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), value, NULL);
	} else {
		add_assoc_zval_ex(&item->v.map.dest, Z_STRVAL(item->v.map.key), Z_STRLEN(item->v.map.key), value);
		Z_TRY_ADDREF_P(value);
	}
	zval_ptr_dtor_str(&item->v.map.key);
	ZVAL_UNDEF(&item->v.map.key);
	if (item->count && --item->count == 0) {
		bool result;
		assert(STACK_ITEM_TOP(&ctx->stack) == item);
		STACK_ITEM_POP(&ctx->stack);
		result = append_item(ctx, &item->v.map.dest);
		if (result) {
			Z_TRY_ADDREF(item->v.map.dest);
		}
		stack_item_free(item);
		return result;
	}
	return true;
}

static bool append_item_to_tag(dec_context *ctx, zval *value, stack_item *item)
{
	zval container;
	if (object_init_ex(&container, CBOR_CE(tag)) != SUCCESS) {
		RETURN_CB_ERROR_B(PHP_CBOR_ERROR_INTERNAL);
	}
	zend_call_known_instance_method_with_2_params(CBOR_CE(tag)->constructor, Z_OBJ(container), NULL, &item->v.value, value);
	assert(STACK_ITEM_TOP(&ctx->stack) == item);
	STACK_ITEM_POP(&ctx->stack);
	zval_ptr_dtor(&item->v.value);
	zval_ptr_dtor(value);
	stack_item_free(item);
	if (!append_item(ctx, &container)) {
		ASSERT_ERROR_SET();
		zval_ptr_dtor(&container);
		return false;
	}
	return true;
}

static bool append_item(dec_context *ctx, zval *value)
{
	stack_item *item = STACK_ITEM_TOP(&ctx->stack);
	if (item == NULL) {
		ctx->root = *value;
		return true;
	}
	switch (item->si_type) {
	case SI_TYPE_ARRAY:
		return append_item_to_array(ctx, value, item);
	case SI_TYPE_MAP:
		return append_item_to_map(ctx, value, item);
	case SI_TYPE_TAG:
		return append_item_to_tag(ctx, value, item);
	}
	RETURN_CB_ERROR_B(PHP_CBOR_ERROR_SYNTAX);
}

#if SIZEOF_ZEND_LONG == 4
#define TEST_OVERFLOW_XINT32(val)  ((val) > ZEND_LONG_MAX)
#define TEST_OVERFLOW_XINT64(val)  true
#elif SIZEOF_ZEND_LONG == 8
#define TEST_OVERFLOW_XINT32(val)  false
#define TEST_OVERFLOW_XINT64(val)  ((val) > ZEND_LONG_MAX)
#else
#error unimplemented
#endif

static void cb_uint8(void *vp_ctx, uint8_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_LONG(&value, val);
	append_item(ctx, &value);
}

static void cb_uint16(void *vp_ctx, uint16_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_LONG(&value, val);
	append_item(ctx, &value);
}

static void cb_uint32(void *vp_ctx, uint32_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (TEST_OVERFLOW_XINT32(val)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	ZVAL_LONG(&value, val);
	append_item(ctx, &value);
}

static void cb_uint64(void *vp_ctx, uint64_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (TEST_OVERFLOW_XINT64(val)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	ZVAL_LONG(&value, val);
	append_item(ctx, &value);
}

static void cb_negint8(void *vp_ctx, uint8_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_LONG(&value, -(zend_long)val - 1);
	append_item(ctx, &value);
}

static void cb_negint16(void *vp_ctx, uint16_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_LONG(&value, -(zend_long)val - 1);
	append_item(ctx, &value);
}

static void cb_negint32(void *vp_ctx, uint32_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (TEST_OVERFLOW_XINT32(val)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	ZVAL_LONG(&value, -(zend_long)val - 1);
	append_item(ctx, &value);
}

static void cb_negint64(void *vp_ctx, uint64_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (TEST_OVERFLOW_XINT64(val)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	ZVAL_LONG(&value, -(zend_long)val - 1);
	append_item(ctx, &value);
}

static bool create_value_object(zval *container, zval *value, zend_class_entry *ce)
{
	if (object_init_ex(container, ce) != SUCCESS) {
		return false;
	}
	zend_call_known_instance_method_with_1_params(ce->constructor, Z_OBJ_P(container), NULL, value);
	zval_ptr_dtor(value);
	return true;
}

static bool append_string_item(dec_context *ctx, zval *value, bool is_text)
{
	zval container;
	int type_flag = is_text ? PHP_CBOR_TEXT : PHP_CBOR_BYTE;
	zend_class_entry *string_ce = is_text ? CBOR_CE(text) : CBOR_CE(byte);
	stack_item *item = STACK_ITEM_TOP(&ctx->stack);
	if (item && item->si_type == SI_TYPE_MAP && Z_ISUNDEF(item->v.map.key)) {  /* is map key */
		bool is_valid_type = is_text ? (ctx->args.flags & PHP_CBOR_KEY_TEXT) : (ctx->args.flags & PHP_CBOR_KEY_BYTE);
		if (!is_valid_type) {
			RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
		return append_item(ctx, value);
	}
	if (ctx->args.flags & type_flag) {  /* to PHP string */
		return append_item(ctx, value);
	}
	if (!create_value_object(&container, value, string_ce)) {
		RETURN_CB_ERROR_B(PHP_CBOR_ERROR_INTERNAL);
	}
	return append_item(ctx, &container);
}

static void do_xstring(dec_context *ctx, cbor_data val, uint64_t length, bool is_text)
{
	zval value;
	stack_item *item = STACK_ITEM_TOP(&ctx->stack);
	if (is_text && !(ctx->args.flags & PHP_CBOR_UNSAFE_TEXT)
			&& !is_utf8((uint8_t *)val, length)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UTF8);
	}
	if (item != NULL) {
		/* indefinite-length string */
		si_type_t str_si_type = is_text ? SI_TYPE_TEXT : SI_TYPE_BYTE;
		si_type_t inv_si_type = is_text ? SI_TYPE_BYTE : SI_TYPE_TEXT;
		if (item->si_type == str_si_type) {
			if (length) {
				smart_str_appendl(&item->v.str, (const char *)val, length);
			}
			return;
		}
		if (item->si_type == inv_si_type) {
			RETURN_CB_ERROR(PHP_CBOR_ERROR_SYNTAX);
		}
	}
	if (!length) {
		ZVAL_EMPTY_STRING(&value);
	} else if (length == 1) {
		ZVAL_CHAR(&value, *val);
	} else {
		ZVAL_STRINGL(&value, (const char *)val, length);
	}
	if (!append_string_item(ctx, &value, is_text)) {
		zval_ptr_dtor(&value);
	}
}

static void cb_text_string(void *vp_ctx, cbor_data val, uint64_t length)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	do_xstring(ctx, val, length, true);
}

static void cb_text_string_start(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	stack_push_xstring(ctx, SI_TYPE_TEXT);
}

static void cb_byte_string(void *vp_ctx, cbor_data val, uint64_t length)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	do_xstring(ctx, val, length, false);
}

static void cb_byte_string_start(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	stack_push_xstring(ctx, SI_TYPE_BYTE);
}

static void cb_array_start(void *vp_ctx, uint64_t count)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (count > 0xffffffff) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	array_init_size(&value, (uint32_t)count);
	if (count) {
		stack_push_counted(ctx, SI_TYPE_ARRAY, &value, (uint32_t)count);
	} else {
		if (!append_item(ctx, &value)) {
			zval_ptr_dtor(&value);
		}
	}
}

static void cb_indef_array_start(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	array_init(&value);
	stack_push_counted(ctx, SI_TYPE_ARRAY, &value, 0);
}

static void cb_map_start(void *vp_ctx, uint64_t count)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (count > 0xffffffff) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->args.flags & PHP_CBOR_MAP_AS_ARRAY) {
		array_init_size(&value, (uint32_t)count);
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	if (count) {
		stack_push_map(ctx, SI_TYPE_MAP, &value, (uint32_t)count);
	} else {
		if (!append_item(ctx, &value)) {
			zval_ptr_dtor(&value);
		}
	}
}

static void cb_indef_map_start(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	if (ctx->args.flags & PHP_CBOR_MAP_AS_ARRAY) {
		array_init(&value);
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	stack_push_map(ctx, SI_TYPE_MAP, &value, 0);
}

static void cb_tag(void *vp_ctx, uint64_t val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	if (val > ZEND_LONG_MAX) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	stack_push_tag(ctx, (zend_long)val);
}

static void do_floatx(dec_context *ctx, float val, int bits)
{
	zval container, value;
	ZVAL_DOUBLE(&value, (double)val);
	int type_flag = bits == 32 ? PHP_CBOR_FLOAT32 : PHP_CBOR_FLOAT16;
	zend_class_entry *float_ce = bits == 32 ? CBOR_CE(float32) : CBOR_CE(float16);
	if (ctx->args.flags & type_flag) {
		append_item(ctx, &value);
		return;
	}
	if (!create_value_object(&container, &value, float_ce)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_INTERNAL);
	}
	if (!append_item(ctx, &container)) {
		zval_ptr_dtor(&container);
	}
}

static void cb_float2(void *vp_ctx, float val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	do_floatx(ctx, val, 16);
}

static void cb_float4(void *vp_ctx, float val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	do_floatx(ctx, val, 32);
}

static void cb_float8(void *vp_ctx, double val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_DOUBLE(&value, val);
	append_item(ctx, &value);
}

static void cb_null(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_NULL(&value);
	append_item(ctx, &value);
}

static void cb_undefined(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_OBJ(&value, php_cbor_get_undef());
	Z_ADDREF(value);
	append_item(ctx, &value);
}

static void cb_boolean(void *vp_ctx, bool val)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	zval value;
	ZVAL_BOOL(&value, val);
	append_item(ctx, &value);
}

static void cb_indef_break(void *vp_ctx)
{
	dec_context *ctx = (dec_context *)vp_ctx;
	stack_item *item = STACK_ITEM_POP(&ctx->stack);
	if (UNEXPECTED(item == NULL)) {
		THROW_CB_ERROR(PHP_CBOR_ERROR_SYNTAX);
	}
	if (UNEXPECTED(!item->si_type)) {
		THROW_CB_ERROR(PHP_CBOR_ERROR_SYNTAX);
	}
	if (item->si_type == SI_TYPE_BYTE || item->si_type == SI_TYPE_TEXT) {
		bool is_text = item->si_type == SI_TYPE_TEXT;
		zval value;
		ZVAL_STR(&value, smart_str_extract(&item->v.str));
		if (!append_string_item(ctx, &value, is_text)) {
			ASSERT_ERROR_SET();
			zval_ptr_dtor(&value);
		}
	} else {  /* SI_TYPE_ARRAY, SI_TYPE_MAP, SI_TYPE_TAG */
		if (UNEXPECTED(item->count != 0)) {  /* definite-length */
			THROW_CB_ERROR(PHP_CBOR_ERROR_SYNTAX);
		}
		if (append_item(ctx, &item->v.value)) {
			Z_TRY_ADDREF(item->v.value);
		} else {
			ASSERT_ERROR_SET();
		}
	}
FINALLY:
	stack_item_free(item);
}

static void set_callbacks()
{
	callbacks.uint8 = &cb_uint8;
	callbacks.uint16 = &cb_uint16;
	callbacks.uint32 = &cb_uint32;
	callbacks.uint64 = &cb_uint64;

	callbacks.negint64 = &cb_negint64;
	callbacks.negint32 = &cb_negint32;
	callbacks.negint16 = &cb_negint16;
	callbacks.negint8 = &cb_negint8;

	callbacks.byte_string_start = &cb_byte_string_start;
	callbacks.byte_string = &cb_byte_string;

	callbacks.string = &cb_text_string;
	callbacks.string_start = &cb_text_string_start;

	callbacks.indef_array_start = &cb_indef_array_start;
	callbacks.array_start = &cb_array_start;

	callbacks.indef_map_start = &cb_indef_map_start;
	callbacks.map_start = &cb_map_start;

	callbacks.tag = &cb_tag;

	callbacks.float2 = &cb_float2;
	callbacks.float4 = &cb_float4;
	callbacks.float8 = &cb_float8;
	callbacks.undefined = &cb_undefined;
	callbacks.null = &cb_null;
	callbacks.boolean = &cb_boolean;

	callbacks.indef_break = &cb_indef_break;
}
