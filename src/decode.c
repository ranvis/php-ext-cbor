/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"
#include "utf8.h"
#include "tags.h"

#define SIZE_INIT_LIMIT  4096

#define _CB_SET_ERROR(error)  do { \
		assert(error); \
		if (!ctx->cb_error) { \
			ctx->cb_error = (error); \
		} \
	} while (false)
#define RETURN_CB_ERROR_B(error)  RETURN_CB_ERROR_V(false, error)
#define RETURN_CB_ERROR_V(v, error)  do { \
		_CB_SET_ERROR(error); \
		return v; \
	} while (false)
#define RETURN_CB_ERROR(error)  do { \
		_CB_SET_ERROR(error); \
		return; \
	} while (false)
#define THROW_CB_ERROR(error)  do { \
		_CB_SET_ERROR(error); \
		goto FINALLY; \
	} while (false)
#define ASSERT_ERROR_SET()  assert(ctx->cb_error)

static struct cbor_callbacks callbacks;

typedef struct stack_item_t stack_item;
typedef struct srns_item_t srns_item_t;

typedef struct {
	php_cbor_decode_args args;
	php_cbor_error cb_error;
	size_t length;
	size_t offset;
	size_t limit;  /* 0:unknown */
	cbor_data data;
	zval root;
	stack_item *stack_top;
	uint32_t stack_depth;
	srns_item_t *srns; /* string ref namespace */
	HashTable refs; /* shared ref */
} dec_context;

typedef enum {
	SI_TYPE_BYTE = 1,
	SI_TYPE_TEXT,
	SI_TYPE_ARRAY,
	SI_TYPE_MAP,
	SI_TYPE_TAG,
	SI_TYPE_TAG_HANDLED,
} si_type_t;

typedef enum {
	DATA_TYPE_STRING = 1,
} data_type_t;

typedef bool (*tag_handler_enter_t)(dec_context *ctx, stack_item *item);
typedef void (*tag_handler_data_t)(dec_context *ctx, stack_item *item, data_type_t type, zval *value);
typedef void (*tag_handler_child_t)(dec_context *ctx, stack_item *item, stack_item *parent_item);
typedef zval *(*tag_handler_exit_t)(dec_context *ctx, zval *value, stack_item *item, zval *storage);
typedef void (*tag_handler_free_t)(stack_item *item);

struct stack_item_t {
	stack_item *next_item;
	si_type_t si_type;
	uint32_t count;
	tag_handler_data_t tag_handler_data;
	tag_handler_child_t tag_handler_child;
	tag_handler_exit_t tag_handler_exit;
	tag_handler_free_t tag_handler_free;
	union stack_item_value_t {
		zval value;
		smart_str str;
		struct stack_item_value_map_t {
			zval dest;
			zval key;
		} map;
		zval tag_id;
		struct stack_item_value_shareable_t {
			zval value;
			zval tmp;
		} shareable;
	} v;
};

struct srns_item_t {  /* srns: string ref namespace */
	srns_item_t *prev_item;
	HashTable str_table;
};

static php_cbor_error dec_zval(dec_context *ctx);
static void set_callbacks();
static void free_srns_item(srns_item_t *srns);

void php_cbor_minit_decode()
{
	set_callbacks();
}

static void stack_free_item(stack_item *item)
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
	} else if (item->si_type == SI_TYPE_TAG_HANDLED) {
		if (item->tag_handler_free) {
			(*item->tag_handler_free)(item);
		}
	} else if (item->si_type) {
		zval_ptr_dtor(&item->v.value);
	}
	assert(item->next_item == NULL);
	efree(item);
}

static stack_item *stack_pop_item(dec_context *ctx)
{
	stack_item *item = ctx->stack_top;
	if (item) {
		ctx->stack_top = item->next_item;
		item->next_item = NULL;
		ctx->stack_depth--;
	}
	return item;
}

static void stack_push_item(dec_context *ctx, stack_item *item)
{
	if (ctx->stack_depth >= ctx->args.max_depth) {
		stack_free_item(item);
		RETURN_CB_ERROR(PHP_CBOR_ERROR_DEPTH);
	}
	ctx->stack_depth++;
	item->next_item = ctx->stack_top;
	ctx->stack_top = item;
}

static stack_item *stack_new_item(dec_context *ctx, si_type_t si_type, uint32_t count)
{
	stack_item *item = (stack_item *)emalloc(sizeof(stack_item));
	stack_item *parent_item = ctx->stack_top;
	memset(item, 0, sizeof *item);
	item->si_type = si_type;
	item->count = count;
	if (parent_item) {
		if (parent_item->tag_handler_child) {
			(*parent_item->tag_handler_child)(ctx, item, parent_item);
		}
	}
	return item;
}

static void stack_push_counted(dec_context *ctx, si_type_t si_type, zval *value, uint32_t count)
{
	stack_item *item = stack_new_item(ctx, si_type, count);
	ZVAL_COPY_VALUE(&item->v.value, value);
	stack_push_item(ctx, item);
}

static void stack_push_map(dec_context *ctx, si_type_t si_type, zval *value, uint32_t count)
{
	stack_item *item = stack_new_item(ctx, si_type, count);
	ZVAL_COPY_VALUE(&item->v.map.dest, value);
	ZVAL_UNDEF(&item->v.map.key);
	stack_push_item(ctx, item);
}

static bool do_tag_enter(dec_context *ctx, zend_long tag_id);

static void stack_push_tag(dec_context *ctx, zend_long tag_id)
{
	stack_item *item;
	if (do_tag_enter(ctx, tag_id)) {
		return;
	}
	item = stack_new_item(ctx, SI_TYPE_TAG, 1);
	ZVAL_LONG(&item->v.tag_id, tag_id);
	stack_push_item(ctx, item);
}

static void stack_push_xstring(dec_context *ctx, si_type_t si_type)
{
	stack_item *item = stack_new_item(ctx, si_type, 0);
	stack_push_item(ctx, item);
}

#define FREE_LINKED_LIST(type_t, head, free_f)  do { \
		for (type_t *item = head; item != NULL; ) { \
			type_t *item_tmp = item->prev_item; \
			free_f(item); \
			item = item_tmp; \
		} \
	} while (0)

static void free_ctx_content(dec_context *ctx)
{
	while (ctx->stack_top) {
		stack_item *item = stack_pop_item(ctx);
		stack_free_item(item);
	}
	FREE_LINKED_LIST(srns_item_t, ctx->srns, free_srns_item);
	if (ctx->args.shared_ref) {
		zend_hash_destroy(&ctx->refs);
	}
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
	ctx.stack_top = NULL;
	ctx.stack_depth = 0;
	ctx.offset = 0;
	if (!(args->flags & PHP_CBOR_SELF_DESCRIBE) && length >= 3
			&& !memcmp(ptr, PHP_CBOR_SELF_DESCRIBE_DATA, sizeof PHP_CBOR_SELF_DESCRIBE_DATA - 1)) {
		ctx.offset = 3;
	}
	ZVAL_UNDEF(&ctx.root);
	ctx.data = ptr;
	ctx.limit = ctx.length = length;
	ctx.srns = NULL;
	if (args->shared_ref) {
		zend_hash_init(&ctx.refs, HT_MIN_SIZE, NULL, ZVAL_PTR_DTOR, false);
	}
	ctx.args = *args;
	error = dec_zval(&ctx);
	free_ctx_content(&ctx);
	if (!error && ctx.offset != ctx.length) {
		error = PHP_CBOR_ERROR_EXTRANEOUS_DATA;
		ctx.args.error_arg = ctx.offset;
	}
	if (!error) {
		if (EXPECTED(Z_TYPE(ctx.root) != IS_REFERENCE)) {
			ZVAL_COPY_VALUE(value, &ctx.root);
		} else {
			zval *tmp = &ctx.root;
			ZVAL_DEREF(tmp);
			ZVAL_COPY_VALUE(value, tmp);
			Z_ADDREF_P(value);
			zval_ptr_dtor(&ctx.root);
		}
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
	} while (ctx->stack_depth);
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
		assert(ctx->stack_top == item);
		stack_pop_item(ctx);
		result = append_item(ctx, &item->v.value);
		if (result) {
			Z_TRY_ADDREF(item->v.value);
		}
		stack_free_item(item);
		return result;
	}
	return true;
}

static bool append_item_to_map(dec_context *ctx, zval *value, stack_item *item)
{
	if (Z_ISUNDEF(item->v.map.key)) {
		switch (Z_TYPE_P(value)) {
		case IS_LONG:
			if (!(ctx->args.flags & PHP_CBOR_INT_KEY)) {
				RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
			}
			if (Z_LVAL_P(value) < 0) {
				RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_VALUE);
			}
			ZVAL_COPY_VALUE(&item->v.map.key, value);
			break;
		case IS_STRING:
			ZVAL_COPY_VALUE(&item->v.map.key, value);
			break;
		default:
			RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
		return true;
	}
	if (Z_TYPE(item->v.map.dest) == IS_OBJECT) {
		if (Z_TYPE(item->v.map.key) == IS_LONG) {
			char num_str[ZEND_LTOA_BUF_LEN];
			ZEND_LTOA(Z_LVAL(item->v.map.key), num_str, sizeof num_str);
			ZVAL_STRINGL(&item->v.map.key, num_str, strlen(num_str));
		}
		if (UNEXPECTED(Z_TYPE_P(value) == IS_REFERENCE)) {
			if (Z_TYPE_P(Z_REFVAL_P(value)) == IS_OBJECT) {
				ZVAL_DEREF(value);
			}
		}
		if (EXPECTED(Z_TYPE_P(value) != IS_REFERENCE)) {
			zend_std_write_property(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), value, NULL);
		} else {
			zval *dest = zend_std_get_property_ptr_ptr(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), BP_VAR_W, NULL);
			if (dest == NULL) {
				RETURN_CB_ERROR_B(PHP_CBOR_ERROR_INTERNAL);
			}
			ZVAL_COPY_VALUE(dest, value);
		}
	} else {
		if (Z_TYPE(item->v.map.key) == IS_LONG) {
			assert(Z_LVAL(item->v.map.key) >= 0);
			add_index_zval(&item->v.map.dest, (zend_ulong)Z_LVAL(item->v.map.key), value);
		} else {
			add_assoc_zval_ex(&item->v.map.dest, Z_STRVAL(item->v.map.key), Z_STRLEN(item->v.map.key), value);
		}
		Z_TRY_ADDREF_P(value);
	}
	zval_ptr_dtor(&item->v.map.key);
	ZVAL_UNDEF(&item->v.map.key);
	if (item->count && --item->count == 0) {
		bool result;
		assert(ctx->stack_top == item);
		stack_pop_item(ctx);
		result = append_item(ctx, &item->v.map.dest);
		if (result) {
			Z_TRY_ADDREF(item->v.map.dest);
		}
		stack_free_item(item);
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
	zend_call_known_instance_method_with_2_params(CBOR_CE(tag)->constructor, Z_OBJ(container), NULL, &item->v.tag_id, value);
	assert(ctx->stack_top == item);
	stack_pop_item(ctx);
	assert(Z_TYPE(item->v.tag_id) == IS_LONG);
	zval_ptr_dtor(value);
	stack_free_item(item);
	if (!append_item(ctx, &container)) {
		ASSERT_ERROR_SET();
		zval_ptr_dtor(&container);
		return false;
	}
	return true;
}

static bool append_item_to_tag_handled(dec_context *ctx, zval *value, stack_item *item)
{
	zval storage, *orig_value = value;
	assert(ctx->stack_top == item);
	stack_pop_item(ctx);
	if (item->tag_handler_exit)  {
		value = (*item->tag_handler_exit)(ctx, value, item, &storage);
	}
	stack_free_item(item);
	if (ctx->cb_error || !append_item(ctx, value)) {
		ASSERT_ERROR_SET();
		/* while caller must free passed value when failed, exit handler may have changed adding value */
		if (orig_value != value) {
			Z_ADDREF_P(orig_value);
			zval_ptr_dtor(value);
		}
		return false;
	}
	return true;
}

static bool append_item(dec_context *ctx, zval *value)
{
	stack_item *item = ctx->stack_top;
	if (item == NULL) {
		ZVAL_COPY_VALUE(&ctx->root, value);
		return true;
	}
	switch (item->si_type) {
	case SI_TYPE_ARRAY:
		return append_item_to_array(ctx, value, item);
	case SI_TYPE_MAP:
		return append_item_to_map(ctx, value, item);
	case SI_TYPE_TAG:
		return append_item_to_tag(ctx, value, item);
	case SI_TYPE_TAG_HANDLED:
		return append_item_to_tag_handled(ctx, value, item);
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

static bool append_string_item(dec_context *ctx, zval *value, bool is_text, bool is_indef)
{
	zval container;
	int type_flag = is_text ? PHP_CBOR_TEXT : PHP_CBOR_BYTE;
	zend_class_entry *string_ce = is_text ? CBOR_CE(text) : CBOR_CE(byte);
	stack_item *item = ctx->stack_top;
	if (item && item->si_type == SI_TYPE_MAP && Z_ISUNDEF(item->v.map.key)) {  /* is map key */
		bool is_valid_type = is_text ? (ctx->args.flags & PHP_CBOR_KEY_TEXT) : (ctx->args.flags & PHP_CBOR_KEY_BYTE);
		if (!is_valid_type) {
			RETURN_CB_ERROR_B(PHP_CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
	} else if (ctx->args.flags & type_flag) {  /* to PHP string */
		/* do nothing*/
	} else {
		if (!create_value_object(&container, value, string_ce)) {
			RETURN_CB_ERROR_B(PHP_CBOR_ERROR_INTERNAL);
		}
		value = &container;
	}
	if (!is_indef && item && item->tag_handler_data) {
		(*item->tag_handler_data)(ctx, item, DATA_TYPE_STRING, value);
	}
	return append_item(ctx, value);
}

static void do_xstring(dec_context *ctx, cbor_data val, uint64_t length, bool is_text)
{
	zval value;
	stack_item *item = ctx->stack_top;
	if (is_text && !(ctx->args.flags & PHP_CBOR_UNSAFE_TEXT)
			&& !is_utf8((uint8_t *)val, length)) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UTF8);
	}
	if (item != NULL) {
		/* test indefinite-length string */
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
		/* not inside indefinite-length string */
	}
	ZVAL_STRINGL_FAST(&value, (const char *)val, length);
	if (!append_string_item(ctx, &value, is_text, false)) {
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
	if (count > ctx->args.max_size) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->limit && ctx->offset + count > ctx->limit) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_TRUNCATED_DATA);
	}
	array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
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
	if (count > ctx->args.max_size) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->limit && ctx->offset + count > ctx->limit) {
		RETURN_CB_ERROR(PHP_CBOR_ERROR_TRUNCATED_DATA);
	}
	if (ctx->args.flags & PHP_CBOR_MAP_AS_ARRAY) {
		array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
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
	stack_item *item = stack_pop_item(ctx);
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
		if (!append_string_item(ctx, &value, is_text, true)) {
			ASSERT_ERROR_SET();
			zval_ptr_dtor(&value);
		}
	} else {  /* SI_TYPE_ARRAY, SI_TYPE_MAP, SI_TYPE_TAG, SI_TYPE_TAG_HANDLED */
		if (UNEXPECTED(item->count != 0)  /* definite-length */
				|| (item->si_type == SI_TYPE_MAP && UNEXPECTED(!Z_ISUNDEF(item->v.map.key)))) {  /* value is expected */
			THROW_CB_ERROR(PHP_CBOR_ERROR_SYNTAX);
		}
		assert(item->si_type == SI_TYPE_ARRAY || item->si_type == SI_TYPE_MAP);
		if (append_item(ctx, &item->v.value)) {
			Z_TRY_ADDREF(item->v.value);
		} else {
			ASSERT_ERROR_SET();
		}
	}
FINALLY:
	stack_free_item(item);
}

bool php_cbor_is_len_string_ref(size_t str_len, uint32_t next_index)
{
	size_t threshold;
	if (next_index <= 23) {
		threshold = 3;
	} else if (next_index <= 0xff) {
		threshold = 4;
	} else if (next_index <= 0xffff) {
		threshold = 5;
	} else if (next_index <= 0xffffffff) {
		threshold = 7;
	} else {
		threshold = 11;
	}
	return str_len >= threshold;
}

static void tag_handler_str_ref_ns_data(dec_context *ctx, stack_item *item, data_type_t type, zval *value)
{
	if (type == DATA_TYPE_STRING) {
		HashTable *str_table = &ctx->srns->str_table;
		// ctx->srns is not NULL because this handler is called
		size_t str_len;
		if (Z_TYPE_P(value) == IS_STRING) {
			str_len = Z_STRLEN_P(value);
		} else if (EXPECTED(Z_TYPE_P(value) == IS_OBJECT)) {
			str_len = ZSTR_LEN(php_cbor_get_xstring_value(value));
		} else {
			RETURN_CB_ERROR(PHP_CBOR_ERROR_INTERNAL);
		}
		if (php_cbor_is_len_string_ref(str_len, zend_hash_num_elements(str_table))) {
			if (zend_hash_next_index_insert(str_table, value) == NULL) {
				RETURN_CB_ERROR(PHP_CBOR_ERROR_INTERNAL);
			}
		}
	}
}

static void tag_handler_str_ref_ns_child(dec_context *ctx, stack_item *item, stack_item *parent_item)
{
	if (item->si_type == SI_TYPE_BYTE || item->si_type == SI_TYPE_TEXT) {
		/* chunks of indefinite length string */
		return;
	}
	/* this may have to be list if another inner handler comes, also prevent from pushing multiple _ns handlers */
	item->tag_handler_child = &tag_handler_str_ref_ns_child;
	item->tag_handler_data = &tag_handler_str_ref_ns_data;
}

static zval *tag_handler_str_ref_ns_exit(dec_context *ctx, zval *value, stack_item *item, zval *storage)
{
	srns_item_t *srns = ctx->srns;
	ctx->srns = srns->prev_item;
	free_srns_item(srns);
	return value;
}

static void free_srns_item(srns_item_t *srns)
{
	zend_hash_destroy(&srns->str_table);
	efree(srns);
}

static void tag_handler_str_ref_ns_free(stack_item *item)
{
}

static bool tag_handler_str_ref_ns_enter(dec_context *ctx, stack_item *item)
{
	srns_item_t *srns = emalloc(sizeof *srns);
	item->tag_handler_data = &tag_handler_str_ref_ns_data;
	item->tag_handler_child = &tag_handler_str_ref_ns_child;
	item->tag_handler_exit = &tag_handler_str_ref_ns_exit;
	item->tag_handler_free = &tag_handler_str_ref_ns_free;
	zend_hash_init(&srns->str_table, 16, NULL, NULL, false);
	srns->prev_item = ctx->srns;
	ctx->srns = srns;
	return true;
}

static zval *tag_handler_str_ref_exit(dec_context *ctx, zval *value, stack_item *item, zval *storage)
{
	zend_long index;
	zval *str;
	if (Z_TYPE_P(value) != IS_LONG) {
		RETURN_CB_ERROR_V(value, PHP_CBOR_ERROR_TAG_TYPE);
	}
	index = Z_LVAL_P(value);
	if (index < 0) {
		RETURN_CB_ERROR_V(value, PHP_CBOR_ERROR_TAG_VALUE);
	}
	if ((str = zend_hash_index_find(&ctx->srns->str_table, index)) == NULL) {
		RETURN_CB_ERROR_V(value, PHP_CBOR_ERROR_TAG_VALUE);
	}
	assert(Z_TYPE_P(value) == IS_LONG);	/* zval_ptr_dtor(value); */
	Z_ADDREF_P(str);
	return str;
}

static bool tag_handler_str_ref_enter(dec_context *ctx, stack_item *item)
{
	item->tag_handler_exit = &tag_handler_str_ref_exit;
	if (!ctx->srns) {
		/* outer stringref-namespace is expected */
		RETURN_CB_ERROR_B(PHP_CBOR_ERROR_TAG_SYNTAX);
	}
	return true;
}

static zval *tag_handler_shareable_exit(dec_context *ctx, zval *value, stack_item *item, zval *storage)
{
	zval *entity;
	ZVAL_COPY_VALUE(storage, &item->v.shareable.value);
	Z_ADDREF_P(storage);  /* returning */
	entity = storage;
	ZVAL_DEREF(entity);
	ZVAL_COPY_VALUE(entity, value);  /* move into ref content */
	return storage;
}

static bool tag_handler_shareable_enter(dec_context *ctx, stack_item *item)
{
	item->tag_handler_exit = &tag_handler_shareable_exit;
	ZVAL_UNDEF(&item->v.shareable.tmp);
	ZVAL_NEW_REF(&item->v.shareable.value, &item->v.shareable.tmp);
	if (zend_hash_next_index_insert(&ctx->refs, &item->v.shareable.value) == NULL) {
		zval_ptr_dtor(&item->v.shareable.value);
		RETURN_CB_ERROR_B(PHP_CBOR_ERROR_INTERNAL);
	}
	return true;
}

static zval *tag_handler_shared_ref_exit(dec_context *ctx, zval *value, stack_item *item, zval *storage)
{
	zend_long index;
	zval *ref_value;
	if (Z_TYPE_P(value) != IS_LONG) {
		RETURN_CB_ERROR_V(value, PHP_CBOR_ERROR_TAG_TYPE);
	}
	index = Z_LVAL_P(value);
	if (index < 0) {
		RETURN_CB_ERROR_V(value, PHP_CBOR_ERROR_TAG_VALUE);
	}
	if ((ref_value = zend_hash_index_find(&ctx->refs, index)) == NULL) {
		/* the spec doesn't prohibit referencing future shareable, but it is unlikely */
		RETURN_CB_ERROR_V(value, PHP_CBOR_ERROR_TAG_VALUE);
	}
	assert(Z_TYPE_P(value) == IS_LONG);	/* zval_ptr_dtor(value); */
	Z_ADDREF_P(ref_value);
	return ref_value;
}

static bool tag_handler_shared_ref_enter(dec_context *ctx, stack_item *item)
{
	item->tag_handler_exit = &tag_handler_shared_ref_exit;
	return true;
}

static bool do_tag_enter(dec_context *ctx, zend_long tag_id)
{
	/* must return true if pushed to stack or an error occurred */
	tag_handler_enter_t handler = NULL;
	if (tag_id == PHP_CBOR_TAG_STRING_REF_NS && ctx->args.string_ref) {
		handler = &tag_handler_str_ref_ns_enter;
	} else if (tag_id == PHP_CBOR_TAG_STRING_REF && ctx->args.string_ref) {
		handler = &tag_handler_str_ref_enter;
	} else if (tag_id == PHP_CBOR_TAG_SHAREABLE && ctx->args.shared_ref) {
		handler = &tag_handler_shareable_enter;
	} else if (tag_id == PHP_CBOR_TAG_SHARED_REF && ctx->args.shared_ref) {
		handler = &tag_handler_shared_ref_enter;
	}
	if (handler) {
		stack_item *item = stack_new_item(ctx, SI_TYPE_TAG_HANDLED, 1);
		if (!(*handler)(ctx, item)) {
			ASSERT_ERROR_SET();
			stack_free_item(item);
			return true;
		}
		stack_push_item(ctx, item);
		return true;
	}
	return false;
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
