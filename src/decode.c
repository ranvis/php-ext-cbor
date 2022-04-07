/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "di_decoder.h"
#include "codec.h"
#include "tags.h"
#include "types.h"
#include "utf8.h"
#include "xzval.h"

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

typedef struct stack_item stack_item;
typedef struct srns_item srns_item;

typedef struct {
	cbor_decode_args args;
	cbor_error cb_error;
	size_t length;
	size_t offset;
	size_t limit;  /* 0:unknown */
	const uint8_t *data;
	zval root;
	stack_item *stack_top;
	uint32_t stack_depth;
	srns_item *srns; /* string ref namespace */
	HashTable *refs; /* shared ref */
} dec_context;

typedef enum {
	SI_TYPE_STRING_MASK = 0x10,
	SI_TYPE_BYTE = (0 | SI_TYPE_STRING_MASK),
	SI_TYPE_TEXT = (1 | SI_TYPE_STRING_MASK),
	SI_TYPE_ARRAY = 2,
	SI_TYPE_MAP,
	SI_TYPE_TAG,
	SI_TYPE_TAG_HANDLED,
} si_type_t;

typedef enum {
	DATA_TYPE_STRING = 1,
} data_type_t;

typedef bool (tag_handler_enter_t)(dec_context *ctx, stack_item *item);
typedef void (tag_handler_data_t)(dec_context *ctx, stack_item *item, data_type_t type, zval *value);
typedef void (tag_handler_child_t)(dec_context *ctx, stack_item *item, stack_item *self);
typedef xzval *(tag_handler_exit_t)(dec_context *ctx, xzval *value, stack_item *item, zval *tmp_v);
typedef void (tag_handler_free_t)(stack_item *item);

struct stack_item {
	stack_item *next_item;
	si_type_t si_type;
	uint32_t count;
	tag_handler_data_t *tag_handler_data;
	tag_handler_child_t *tag_handler_child[2];
	tag_handler_exit_t *tag_handler_exit;
	tag_handler_free_t *tag_handler_free;
	union si_value_t {
		zval value;
		smart_str str;
		struct si_value_map_t {
			zval dest;
			zval key;
		} map;
		zval tag_id;
		struct si_value_tag_handled_t {
			zend_ulong tag_id;
			union si_value_tag_h_value_t{
				srns_item *srns_detached;
				struct si_tag_shareable_t {
					zval value;
					zend_long index;
				} shareable;
			} v;
		} tag_h;
	} v;
};

struct srns_item {  /* srns: string ref namespace */
	srns_item *prev_item;
	HashTable *str_table;
};

static cbor_error dec_zval(dec_context *ctx);
static cbor_error decode_cbor_data_item(const uint8_t *data, size_t len, cbor_di_decoded *out, dec_context *ctx);
static void free_srns_item(srns_item *srns);

void php_cbor_minit_decode()
{
}

/* just in case, defined as a macro, as function pointer is theoretically incompatible with data pointer */
#define DECLARE_SI_SET_HANDLER_VEC(member)  \
	static bool register_handler_vec_##member(stack_item *item, member##_t *handler) \
	{ \
		int i, count = sizeof item->member / sizeof item->member[0]; \
		for (i = 0; i < count && item->member[i]; i++) { \
			if (item->member[i] == handler) { \
				return true; \
			} \
		} \
		assert(i < count); \
		if (i >= count) { \
			return false; \
		} \
		item->member[i] = handler; \
		return true; \
	}
#define SI_SET_HANDLER_VEC(member, si, handler)  do { \
		if (!register_handler_vec_##member(si, handler) && !ctx->cb_error) { \
			ctx->cb_error = CBOR_ERROR_INTERNAL; \
		} \
	} while (0)
#define SI_SET_HANDLER(member, si, handler)  si->member = handler
#define SI_CALL_HANDLER(si, member, ...)  (*si->member)(__VA_ARGS__)
#define SI_CALL_HANDLER_VEC(member, si, ...)  do { \
		for (int i = 0; i < sizeof si->member / sizeof si->member[0] && si->member[i]; i++) { \
			(*si->member[i])(__VA_ARGS__); \
		} \
	} while (0)

DECLARE_SI_SET_HANDLER_VEC(tag_handler_child)

#define SI_SET_DATA_HANDLER(si, handler)  SI_SET_HANDLER(tag_handler_data, si, handler)
#define SI_SET_CHILD_HANDLER(si, handler)  SI_SET_HANDLER_VEC(tag_handler_child, si, handler)
#define SI_SET_EXIT_HANDLER(si, handler)  SI_SET_HANDLER(tag_handler_exit, si, handler)
#define SI_SET_FREE_HANDLER(si, handler)  SI_SET_HANDLER(tag_handler_free, si, handler)

#define SI_CALL_CHILD_HANDLER(si, ...)  SI_CALL_HANDLER_VEC(tag_handler_child, si, __VA_ARGS__)

static void stack_free_item(stack_item *item)
{
	if (item == NULL) {
		return;
	}
	if (item->si_type & SI_TYPE_STRING_MASK) {
		smart_str_free(&item->v.str);
	} else if (item->si_type == SI_TYPE_MAP) {
		zval_ptr_dtor(&item->v.map.dest);
		XZVAL_PURIFY(&item->v.map.key);
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
	stack_item *parent_item = ctx->stack_top;
	if (ctx->stack_depth >= ctx->args.max_depth) {
		stack_free_item(item);
		RETURN_CB_ERROR(CBOR_ERROR_DEPTH);
	}
	if (parent_item) {
		SI_CALL_CHILD_HANDLER(parent_item, ctx, item, parent_item);
	}
	ctx->stack_depth++;
	item->next_item = ctx->stack_top;
	ctx->stack_top = item;
}

static stack_item *stack_new_item(dec_context *ctx, si_type_t si_type, uint32_t count)
{
	stack_item *item = (stack_item *)emalloc(sizeof(stack_item));
	memset(item, 0, sizeof *item);
	item->si_type = si_type;
	item->count = count;
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
	FREE_LINKED_LIST(srns_item, ctx->srns, free_srns_item);
	if (ctx->args.shared_ref) {
		zend_array_destroy(ctx->refs);
	}
}

cbor_error php_cbor_decode(zend_string *data, zval *value, cbor_decode_args *args)
{
	cbor_error error;
	dec_context ctx;
	const uint8_t *ptr = (const uint8_t *)ZSTR_VAL(data);
	size_t length = ZSTR_LEN(data);
	if ((args->flags & CBOR_FLOAT16 && args->flags & CBOR_FLOAT32)) {
		return CBOR_ERROR_INVALID_FLAGS;
	}
	ctx.stack_top = NULL;
	ctx.stack_depth = 0;
	ctx.offset = 0;
	if (!(args->flags & CBOR_SELF_DESCRIBE) && length >= 3
			&& !memcmp(ptr, CBOR_SELF_DESCRIBE_DATA, sizeof CBOR_SELF_DESCRIBE_DATA - 1)) {
		ctx.offset = 3;
	}
	ZVAL_UNDEF(&ctx.root);
	ctx.data = ptr;
	ctx.limit = ctx.length = length;
	ctx.srns = NULL;
	if (args->shared_ref) {
		ctx.refs = zend_new_array(0);
	}
	ctx.args = *args;
	error = dec_zval(&ctx);
	free_ctx_content(&ctx);
	if (!error && ctx.offset != ctx.length) {
		error = CBOR_ERROR_EXTRANEOUS_DATA;
		ctx.args.error_arg = ctx.offset;
	}
	if (!error) {
		if (EXPECTED(Z_TYPE(ctx.root) != IS_REFERENCE)) {
			ZVAL_COPY_VALUE(value, &ctx.root);
		} else {
			zval *tmp = &ctx.root;
			ZVAL_DEREF(tmp);
			ZVAL_COPY_VALUE(value, tmp);
			Z_TRY_ADDREF_P(value);
			zval_ptr_dtor(&ctx.root);
		}
	} else {
		args->error_arg = ctx.args.error_arg;
	}
	return error;
}

static cbor_error dec_zval(dec_context *ctx)
{
	cbor_error error;
	cbor_di_decoded out_data;
	ctx->cb_error = 0;
	do {
		if (ctx->offset >= ctx->length) {
			ctx->args.error_arg = ctx->offset;
			return CBOR_ERROR_TRUNCATED_DATA;
		}
		error = decode_cbor_data_item(ctx->data + ctx->offset, ctx->length - ctx->offset, &out_data, ctx);
		ctx->offset += out_data.read_len;
		if (error) {
			ctx->args.error_arg = ctx->offset;
			return error;
		}
		if (ctx->cb_error) {
			ctx->args.error_arg = ctx->offset;
			return ctx->cb_error;
		}
	} while (ctx->stack_depth);
	return 0;
}

static bool convert_xz_xint_to_string(xzval *value)
{
	char buf[24];  /* ceil(log10(2)*64) = 20 */
	size_t len;
	assert(XZ_ISXXINT_P(value));
	if (Z_TYPE_P(value) == IS_X_UINT) {
		len = snprintf(buf, sizeof buf, "%" PRIu64, XZ_XINT_P(value));
	} else {
		uint64_t n_value = XZ_XINT_P(value);
		bool fix;
		n_value++;
		if ((fix = !n_value) != 0) {
			n_value--;
		}
		len = snprintf(buf, sizeof buf, "-%" PRIu64, n_value);
		assert(len >= 1 && len < sizeof buf);
		if (fix) {
			buf[len - 1]++;  /* "-18446744073709551615" => "...6" */
		}
	}
	ZVAL_STRINGL(value, buf, len);
	return true;
}

static bool append_item(dec_context *ctx, xzval *value);

static bool append_item_to_array(dec_context *ctx, xzval *value, stack_item *item)
{
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	if (add_next_index_zval(&item->v.value, value) != SUCCESS) {
		RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
	}
	Z_TRY_ADDREF_P(value);
	if (item->count && --item->count == 0) {
		bool result;
		assert(ctx->stack_top == item);
		stack_pop_item(ctx);
		result = append_item(ctx, &item->v.value);
		stack_free_item(item);
		return result;
	}
	return true;
}

static bool append_item_to_map(dec_context *ctx, xzval *value, stack_item *item)
{
	if (Z_ISUNDEF(item->v.map.key)) {
		switch (Z_TYPE_P(value)) {
		case IS_LONG:
		case IS_X_UINT:
		case IS_X_NINT:
			if (!(ctx->args.flags & CBOR_INT_KEY)) {
				RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
			}
			ZVAL_COPY_VALUE(&item->v.map.key, value);
			break;
		case IS_STRING:
			ZVAL_COPY_VALUE(&item->v.map.key, value);
			Z_TRY_ADDREF_P(value);
			break;
		default:
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
		return true;
	}
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	if (XZ_ISXXINT(item->v.map.key)) {
		if (!convert_xz_xint_to_string(&item->v.map.key)) {
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
	}
	if (Z_TYPE(item->v.map.dest) == IS_OBJECT) {
		if (Z_TYPE(item->v.map.key) == IS_LONG) {
			char num_str[ZEND_LTOA_BUF_LEN];
			ZEND_LTOA(Z_LVAL(item->v.map.key), num_str, sizeof num_str);
			ZVAL_STRINGL(&item->v.map.key, num_str, strlen(num_str));
		}
		assert(Z_TYPE_P(value) != IS_REFERENCE || Z_TYPE_P(Z_REFVAL_P(value)) != IS_OBJECT);
		if (Z_STRLEN(item->v.map.key) >= 1 && Z_STRVAL(item->v.map.key)[0] == '\0') {
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_KEY_VALUE);
		}
		if (EXPECTED(Z_TYPE_P(value) != IS_REFERENCE)) {
			zend_std_write_property(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), value, NULL);
		} else {
			zval *dest = zend_std_get_property_ptr_ptr(Z_OBJ(item->v.map.dest), Z_STR(item->v.map.key), BP_VAR_W, NULL);
			if (dest == NULL) {
				RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
			}
			Z_TRY_ADDREF_P(value);
			zval_ptr_dtor(dest);
			ZVAL_COPY_VALUE(dest, value);
		}
	} else {  /* IS_ARRAY */
		if (Z_TYPE(item->v.map.key) == IS_LONG) {
			/* The argument accepts zend_ulong as underlying hash table does, but the actual index visible for PHP script is still zend_long. */
			add_index_zval(&item->v.map.dest, (zend_ulong)Z_LVAL(item->v.map.key), value);
		} else {
			add_assoc_zval_ex(&item->v.map.dest, Z_STRVAL(item->v.map.key), Z_STRLEN(item->v.map.key), value);
		}
		Z_TRY_ADDREF_P(value);
	}
	XZVAL_PURIFY(&item->v.map.key);
	zval_ptr_dtor(&item->v.map.key);
	ZVAL_UNDEF(&item->v.map.key);
	if (item->count && --item->count == 0) {
		bool result;
		assert(ctx->stack_top == item);
		stack_pop_item(ctx);
		result = append_item(ctx, &item->v.map.dest);
		stack_free_item(item);
		return result;
	}
	return true;
}

static bool append_item_to_tag(dec_context *ctx, xzval *value, stack_item *item)
{
	zval container;
	bool result;
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	if (object_init_ex(&container, CBOR_CE(tag)) != SUCCESS) {
		RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
	}
	zend_call_known_instance_method_with_2_params(CBOR_CE(tag)->constructor, Z_OBJ(container), NULL, &item->v.tag_id, value);
	assert(ctx->stack_top == item);
	stack_pop_item(ctx);
	assert(Z_TYPE(item->v.tag_id) == IS_LONG);
	stack_free_item(item);
	result = append_item(ctx, &container);
	zval_ptr_dtor(&container);
	return result;
}

static bool append_item_to_tag_handled(dec_context *ctx, xzval *value, stack_item *item)
{
	zval tmp_v;
	xzval *orig_v = value;
	bool result;
	assert(ctx->stack_top == item);
	stack_pop_item(ctx);
	if (item->tag_handler_exit)  {
		assert(!ctx->cb_error);
		value = (*item->tag_handler_exit)(ctx, value, item, &tmp_v);
		/* exit handler may return the new value, that caller frees */
	}
	result = !ctx->cb_error && append_item(ctx, value);
	if (value != orig_v) {
		zval_ptr_dtor(value);
	}
	stack_free_item(item);
	return result;
}

static bool append_item(dec_context *ctx, xzval *value)
{
	stack_item *item = ctx->stack_top;
	if (item == NULL) {
		if (XZ_ISXXINT_P(value)) {
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_VALUE);
		}
		Z_TRY_ADDREF_P(value);
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

static void proc_uint32(dec_context *ctx, uint32_t val)
{
	xzval value;
	if (TEST_OVERFLOW_XINT32(val)) {
		XZVAL_UINT(&value, (uint64_t)val);
	} else {
		ZVAL_LONG(&value, val);
	}
	append_item(ctx, &value);
}

static void proc_uint64(dec_context *ctx, uint64_t val)
{
	xzval value;
	if (TEST_OVERFLOW_XINT64(val)) {
		XZVAL_UINT(&value, val);
	} else {
		ZVAL_LONG(&value, (zend_long)val);
	}
	append_item(ctx, &value);
}

static void proc_nint32(dec_context *ctx, uint32_t val)
{
	xzval value;
	if (TEST_OVERFLOW_XINT32(val)) {
		XZVAL_NINT(&value, (uint64_t)val);
	} else {
		ZVAL_LONG(&value, -(zend_long)val - 1);
	}
	append_item(ctx, &value);
}

static void proc_nint64(dec_context *ctx, uint64_t val)
{
	zval value;
	if (TEST_OVERFLOW_XINT64(val)) {
		XZVAL_NINT(&value, val);
	} else {
		ZVAL_LONG(&value, -(zend_long)val - 1);
	}
	append_item(ctx, &value);
}

static bool create_value_object(zval *container, zval *value, zend_class_entry *ce)
{
	if (object_init_ex(container, ce) != SUCCESS) {
		return false;
	}
	zend_call_known_instance_method_with_1_params(ce->constructor, Z_OBJ_P(container), NULL, value);
	return true;
}

static bool append_string_item(dec_context *ctx, zval *value, bool is_text, bool is_indef)
{
	zval container;
	int type_flag = is_text ? CBOR_TEXT : CBOR_BYTE;
	zend_class_entry *string_ce = is_text ? CBOR_CE(text) : CBOR_CE(byte);
	stack_item *item = ctx->stack_top;
	bool result;
	ZVAL_NULL(&container);
	if (item && item->si_type == SI_TYPE_MAP && Z_ISUNDEF(item->v.map.key)) {  /* is map key */
		bool is_valid_type = is_text ? (ctx->args.flags & CBOR_KEY_TEXT) : (ctx->args.flags & CBOR_KEY_BYTE);
		if (!is_valid_type) {
			RETURN_CB_ERROR_B(CBOR_ERROR_UNSUPPORTED_KEY_TYPE);
		}
	} else if (ctx->args.flags & type_flag) {  /* to PHP string */
		/* do nothing*/
	} else {
		zend_object *obj = php_cbor_xstring_create(string_ce);
		php_cbor_xstring_set_value(obj, Z_STR_P(value));
		ZVAL_OBJ(&container, obj);
		value = &container;
	}
	if (!is_indef && item && item->tag_handler_data) {
		(*item->tag_handler_data)(ctx, item, DATA_TYPE_STRING, value);
	}
	result = append_item(ctx, value);
	if (!Z_ISNULL(container)) {
		zval_ptr_dtor(&container);
	}
	return result;
}

static void do_xstring(dec_context *ctx, const char *val, uint64_t length, bool is_text)
{
	zval value;
	stack_item *item = ctx->stack_top;
#if UINT64_MAX > SIZE_MAX
	if (length > SIZE_MAX) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
#endif
	if (is_text && !(ctx->args.flags & CBOR_UNSAFE_TEXT)
			&& !is_utf8((uint8_t *)val, (size_t)length)) {
		RETURN_CB_ERROR(CBOR_ERROR_UTF8);
	}
	if (item != NULL && item->si_type & SI_TYPE_STRING_MASK) {
		/* indefinite-length string */
		si_type_t str_si_type = is_text ? SI_TYPE_TEXT : SI_TYPE_BYTE;
		if (item->si_type == str_si_type) {
			if (length) {
				if (UNEXPECTED(length > SIZE_MAX - smart_str_get_len(&item->v.str))) {
					RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
				}
				smart_str_appendl(&item->v.str, (const char *)val, (size_t)length);
			}
			return;
		}
		RETURN_CB_ERROR(CBOR_ERROR_SYNTAX);
	}
	ZVAL_STRINGL_FAST(&value, (const char *)val, (size_t)length);
	append_string_item(ctx, &value, is_text, false);
	zval_ptr_dtor(&value);
}

static void proc_text_string(dec_context *ctx, const char *val, uint64_t length)
{
	do_xstring(ctx, val, length, true);
}

static void proc_text_string_start(dec_context *ctx)
{
	stack_push_xstring(ctx, SI_TYPE_TEXT);
}

static void proc_byte_string(dec_context *ctx, const char *val, uint64_t length)
{
	do_xstring(ctx, val, length, false);
}

static void proc_byte_string_start(dec_context *ctx)
{
	stack_push_xstring(ctx, SI_TYPE_BYTE);
}

static void proc_array_start(dec_context *ctx, uint32_t count)
{
	zval value;
	if (count > ctx->args.max_size) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->limit && ctx->offset + count > ctx->limit) {
		RETURN_CB_ERROR(CBOR_ERROR_TRUNCATED_DATA);
	}
	array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
	if (count) {
		stack_push_counted(ctx, SI_TYPE_ARRAY, &value, (uint32_t)count);
	} else {
		append_item(ctx, &value);
		zval_ptr_dtor(&value);
	}
}

static void proc_indef_array_start(dec_context *ctx)
{
	zval value;
	array_init(&value);
	stack_push_counted(ctx, SI_TYPE_ARRAY, &value, 0);
}

static void proc_map_start(dec_context *ctx, uint32_t count)
{
	zval value;
	if (count > ctx->args.max_size) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
	if (ctx->limit && ctx->offset + count > ctx->limit) {
		RETURN_CB_ERROR(CBOR_ERROR_TRUNCATED_DATA);
	}
	if (ctx->args.flags & CBOR_MAP_AS_ARRAY) {
		array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	if (count) {
		stack_push_map(ctx, SI_TYPE_MAP, &value, (uint32_t)count);
	} else {
		append_item(ctx, &value);
		zval_ptr_dtor(&value);
	}
}

static void proc_indef_map_start(dec_context *ctx)
{
	zval value;
	if (ctx->args.flags & CBOR_MAP_AS_ARRAY) {
		array_init(&value);
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	stack_push_map(ctx, SI_TYPE_MAP, &value, 0);
}

static void proc_tag(dec_context *ctx, uint64_t val)
{
	if (val > ZEND_LONG_MAX) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	stack_push_tag(ctx, (zend_long)val);
}

static void do_floatx(dec_context *ctx, uint32_t raw, int bits)
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
			ZVAL_DOUBLE(&value, php_cbor_from_float16((uint16_t)raw));
		}
		append_item(ctx, &value);
		return;
	}
	obj = php_cbor_floatx_create(float_ce);
	php_cbor_floatx_set_value(obj, NULL, raw);  /* always succeeds */
	ZVAL_OBJ(&container, obj);
	append_item(ctx, &container);
	zval_ptr_dtor(&container);
}

static void proc_float16(dec_context *ctx, uint16_t val)
{
	do_floatx(ctx, val, 16);
}

static void proc_float32(dec_context *ctx, uint32_t val)
{
	do_floatx(ctx, val, 32);
}

static void proc_float64(dec_context *ctx, double val)
{
	zval value;
	ZVAL_DOUBLE(&value, val);
	append_item(ctx, &value);
}

static void proc_null(dec_context *ctx)
{
	zval value;
	ZVAL_NULL(&value);
	append_item(ctx, &value);
}

static void proc_undefined(dec_context *ctx)
{
	zval value;
	ZVAL_OBJ(&value, php_cbor_get_undef());
	Z_ADDREF(value);
	append_item(ctx, &value);
}

static void proc_boolean(dec_context *ctx, bool val)
{
	zval value;
	ZVAL_BOOL(&value, val);
	append_item(ctx, &value);
}

static void proc_indef_break(dec_context *ctx)
{
	stack_item *item = stack_pop_item(ctx);
	if (UNEXPECTED(item == NULL)) {
		THROW_CB_ERROR(CBOR_ERROR_SYNTAX);
	}
	if (UNEXPECTED(!item->si_type)) {
		THROW_CB_ERROR(CBOR_ERROR_SYNTAX);
	}
	if (item->si_type & SI_TYPE_STRING_MASK) {
		bool is_text = item->si_type == SI_TYPE_TEXT;
		zval value;
		ZVAL_STR(&value, smart_str_extract(&item->v.str));
		append_string_item(ctx, &value, is_text, true);
		zval_ptr_dtor(&value);
	} else {  /* SI_TYPE_ARRAY, SI_TYPE_MAP, SI_TYPE_TAG, SI_TYPE_TAG_HANDLED */
		if (UNEXPECTED(item->count != 0)  /* definite-length */
				|| (item->si_type == SI_TYPE_MAP && UNEXPECTED(!Z_ISUNDEF(item->v.map.key)))) {  /* value is expected */
			THROW_CB_ERROR(CBOR_ERROR_SYNTAX);
		}
		assert(item->si_type == SI_TYPE_ARRAY || item->si_type == SI_TYPE_MAP);
		append_item(ctx, &item->v.value);
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
		HashTable *str_table = ctx->srns->str_table;
		size_t str_len;
		if (Z_TYPE_P(value) == IS_STRING) {
			str_len = Z_STRLEN_P(value);
		} else if (EXPECTED(Z_TYPE_P(value) == IS_OBJECT)) {
			str_len = ZSTR_LEN(php_cbor_get_xstring_value(value));
		} else {
			RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
		}
		if (php_cbor_is_len_string_ref(str_len, zend_hash_num_elements(str_table))) {
			if (zend_hash_next_index_insert(str_table, value) == NULL) {
				RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
			}
			Z_ADDREF_P(value);
		}
	}
}

static void tag_handler_str_ref_ns_child(dec_context *ctx, stack_item *item, stack_item *self)
{
	if (item->si_type & SI_TYPE_STRING_MASK) {
		/* chunks of indefinite length string */
		return;
	}
	SI_SET_CHILD_HANDLER(item, &tag_handler_str_ref_ns_child);
	SI_SET_DATA_HANDLER(item, &tag_handler_str_ref_ns_data);
}

static xzval *tag_handler_str_ref_ns_exit(dec_context *ctx, xzval *value, stack_item *item, zval *tmp_v)
{
	srns_item *srns = ctx->srns;
	ctx->srns = srns->prev_item;
	item->v.tag_h.v.srns_detached = srns;
	return value;
}

static void free_srns_item(srns_item *srns)
{
	zend_array_destroy(srns->str_table);
	efree(srns);
}

static void tag_handler_str_ref_ns_free(stack_item *item)
{
	if (item->v.tag_h.v.srns_detached) {
		free_srns_item(item->v.tag_h.v.srns_detached);
	}
}

static bool tag_handler_str_ref_ns_enter(dec_context *ctx, stack_item *item)
{
	srns_item *srns = emalloc(sizeof *srns);
	SI_SET_DATA_HANDLER(item, &tag_handler_str_ref_ns_data);
	SI_SET_CHILD_HANDLER(item, &tag_handler_str_ref_ns_child);
	SI_SET_EXIT_HANDLER(item, &tag_handler_str_ref_ns_exit);
	SI_SET_FREE_HANDLER(item, &tag_handler_str_ref_ns_free);
	item->v.tag_h.v.srns_detached = NULL;
	srns->str_table = zend_new_array(0);
	srns->prev_item = ctx->srns;
	ctx->srns = srns;
	return true;
}

static xzval *tag_handler_str_ref_exit(dec_context *ctx, xzval *value, stack_item *item, zval *tmp_v)
{
	zend_long index;
	zval *str;
	if (Z_TYPE_P(value) != IS_LONG) {
		RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_TYPE);
	}
	index = Z_LVAL_P(value);
	if (index < 0) {
		RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_VALUE);
	}
	if ((str = zend_hash_index_find(ctx->srns->str_table, index)) == NULL) {
		RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_VALUE);
	}
	assert(Z_TYPE_P(value) == IS_LONG);
	Z_ADDREF_P(str);
	ZVAL_COPY_VALUE(tmp_v, str);
	return tmp_v;
}

static bool tag_handler_str_ref_enter(dec_context *ctx, stack_item *item)
{
	SI_SET_EXIT_HANDLER(item, &tag_handler_str_ref_exit);
	if (!ctx->srns) {
		/* outer stringref-namespace is expected */
		RETURN_CB_ERROR_B(CBOR_ERROR_TAG_SYNTAX);
	}
	return true;
}

static void tag_handler_shareable_child(dec_context *ctx, stack_item *item, stack_item *self)
{
	zval *real_v, tmp_v;
	assert(self->si_type == SI_TYPE_TAG_HANDLED);
	assert(Z_TYPE(self->v.tag_h.v.shareable.value) == IS_NULL);
	if (item->si_type == SI_TYPE_MAP && Z_TYPE(item->v.map.dest) == IS_OBJECT) {
		real_v = &item->v.map.dest;
		ZVAL_COPY_VALUE(&self->v.tag_h.v.shareable.value, real_v);
	} else {
		if (ctx->args.shared_ref == OPT_SHAREABLE) {
			ZVAL_TRUE(&self->v.tag_h.v.shareable.value);
			if (!create_value_object(&tmp_v, &self->v.tag_h.v.shareable.value, CBOR_CE(shareable))) {
				RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
			}
			real_v = &tmp_v;
		} else if (ctx->args.shared_ref == OPT_UNSAFE_REF) {
			ZVAL_UNDEF(&tmp_v);
			real_v = &self->v.tag_h.v.shareable.value;
			ZVAL_NEW_REF(real_v, &tmp_v);
		} else {
			RETURN_CB_ERROR(CBOR_ERROR_TAG_TYPE);
		}
	}
	if (zend_hash_index_update(ctx->refs, self->v.tag_h.v.shareable.index, real_v) == NULL) {
		zval_ptr_dtor(real_v);
		RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
	} else {
		Z_ADDREF_P(real_v);
	}
}

static xzval *tag_handler_shareable_exit(dec_context *ctx, xzval *value, stack_item *item, zval *tmp_v)
{
	zval *real_v;
	if (XZ_ISXXINT_P(value)) {
		RETURN_CB_ERROR_V(value, CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	if (Z_TYPE(item->v.tag_h.v.shareable.value) == IS_NULL) {
		/* child handler is not called */
		if (Z_TYPE_P(value) == IS_OBJECT || ctx->args.shared_ref == OPT_UNSAFE_REF) {
			if (Z_TYPE_P(value) != IS_OBJECT) {
				Z_TRY_ADDREF_P(value);
				real_v = tmp_v;
				ZVAL_COPY_VALUE(real_v, value);
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
			RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_TYPE);
		}
		if (zend_hash_index_update(ctx->refs, item->v.tag_h.v.shareable.index, real_v) == NULL) {
			if (real_v != value) {
				zval_ptr_dtor(real_v);
			}
			RETURN_CB_ERROR_V(value, CBOR_ERROR_INTERNAL);
		}
	} else {
		real_v = tmp_v;
		ZVAL_COPY_VALUE(real_v, &item->v.tag_h.v.shareable.value);
		if (Z_TYPE_P(real_v) == IS_TRUE) {
			zend_update_property(CBOR_CE(shareable), Z_OBJ_P(real_v), ZEND_STRL("value"), value);
		} else if (Z_TYPE_P(real_v) == IS_REFERENCE) {
			zval *shareable_ref = real_v;
			ZVAL_DEREF(shareable_ref);
			ZVAL_COPY_VALUE(shareable_ref, value);  /* move into ref content */
			Z_TRY_ADDREF_P(value);
		}
	}
	Z_ADDREF_P(real_v);  /* returning */
	return real_v;
}

static bool tag_handler_shareable_enter(dec_context *ctx, stack_item *item)
{
	if (ctx->stack_top && ctx->stack_top->si_type == SI_TYPE_TAG_HANDLED && ctx->stack_top->v.tag_h.tag_id == CBOR_TAG_SHAREABLE) {
		/* nested shareable */
		RETURN_CB_ERROR_B(CBOR_ERROR_TAG_SYNTAX);
	}
	SI_SET_CHILD_HANDLER(item, &tag_handler_shareable_child);
	SI_SET_EXIT_HANDLER(item, &tag_handler_shareable_exit);
	ZVAL_NULL(&item->v.tag_h.v.shareable.value); /* cannot be undef if adding to HashTable */
	item->v.tag_h.v.shareable.index = zend_hash_num_elements(ctx->refs);
	if (zend_hash_next_index_insert(ctx->refs, &item->v.tag_h.v.shareable.value) == NULL) {
		RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
	}
	return true;
}

static xzval *tag_handler_shared_ref_exit(dec_context *ctx, zval *value, stack_item *item, zval *tmp_v)
{
	zend_long index;
	zval *ref_v;
	if (Z_TYPE_P(value) != IS_LONG) {
		RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_TYPE);
	}
	index = Z_LVAL_P(value);
	if (index < 0) {
		RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_VALUE);
	}
	if ((ref_v = zend_hash_index_find(ctx->refs, index)) == NULL) {
		/* the spec doesn't prohibit referencing future shareable, but it is unlikely */
		RETURN_CB_ERROR_V(value, CBOR_ERROR_TAG_VALUE);
	}
	assert(Z_TYPE_P(value) == IS_LONG);	/* zval_ptr_dtor(value); */
	Z_ADDREF_P(ref_v);
	return ref_v;  /* returning hash structure */
}

static bool tag_handler_shared_ref_enter(dec_context *ctx, stack_item *item)
{
	SI_SET_EXIT_HANDLER(item, &tag_handler_shared_ref_exit);
	return true;
}

static bool do_tag_enter(dec_context *ctx, zend_long tag_id)
{
	/* must return true if pushed to stack or an error occurred */
	tag_handler_enter_t *handler = NULL;
	if (tag_id == CBOR_TAG_STRING_REF_NS && ctx->args.string_ref) {
		handler = &tag_handler_str_ref_ns_enter;
	} else if (tag_id == CBOR_TAG_STRING_REF && ctx->args.string_ref) {
		handler = &tag_handler_str_ref_enter;
	} else if (tag_id == CBOR_TAG_SHAREABLE && ctx->args.shared_ref) {
		handler = &tag_handler_shareable_enter;
	} else if (tag_id == CBOR_TAG_SHARED_REF && ctx->args.shared_ref) {
		handler = &tag_handler_shared_ref_enter;
	}
	if (handler) {
		stack_item *item = stack_new_item(ctx, SI_TYPE_TAG_HANDLED, 1);
		item->v.tag_h.tag_id = tag_id;
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

#define SET_READ_ERROR(result)  do { \
		if (!result) { \
			error = out->req_len ? CBOR_ERROR_TRUNCATED_DATA \
				: CBOR_ERROR_MALFORMED_DATA; \
			goto FINALLY; \
		} \
	} while (0)

static cbor_error decode_cbor_data_item(const uint8_t *data, size_t len, cbor_di_decoded *out, dec_context *ctx)
{
	cbor_error error = 0;
	uint8_t type = cbor_di_get_type(data, len);
	out->read_len = 1;
	out->req_len = 0;
	switch (type) {
	case DI_UINT:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		if (!DI_IS_DOUBLE(*out)) {
			proc_uint32(ctx, out->v.i32);
		} else {
			proc_uint64(ctx, out->v.i64);
		}
		break;
	case DI_NINT:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		if (!DI_IS_DOUBLE(*out)) {
			proc_nint32(ctx, out->v.i32);
		} else {
			proc_nint64(ctx, out->v.i64);
		}
		break;
	case DI_BSTR:
		SET_READ_ERROR(cbor_di_read_str(data, len, out));
		if (out->v.str.val) {
			proc_byte_string(ctx, out->v.str.val, out->v.str.len);
		} else {
			proc_byte_string_start(ctx);
		}
		break;
	case DI_TSTR:
		SET_READ_ERROR(cbor_di_read_str(data, len, out));
		if (out->v.str.val) {
			proc_text_string(ctx, out->v.str.val, out->v.str.len);
		} else {
			proc_text_string_start(ctx);
		}
		break;
	case DI_ARRAY:
		SET_READ_ERROR(cbor_di_read_list(data, len, out));
		if (!out->v.ext.is_indef) {
			if (out->v.i64 > 0xffffffff) {
				return CBOR_ERROR_UNSUPPORTED_SIZE;
			}
			proc_array_start(ctx, (uint32_t)out->v.i64);
		} else {
			proc_indef_array_start(ctx);
		}
		break;
	case DI_MAP:
		SET_READ_ERROR(cbor_di_read_list(data, len, out));
		if (!out->v.ext.is_indef) {
			if (out->v.i64 > 0xffffffff) {
				return CBOR_ERROR_UNSUPPORTED_SIZE;
			}
			proc_map_start(ctx, (uint32_t)out->v.i64);
		} else {
			proc_indef_map_start(ctx);
		}
		break;
	case DI_TAG:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		proc_tag(ctx, !DI_IS_DOUBLE(*out) ? out->v.i32 : out->v.i64);
		break;
	case DI_FALSE:
	case DI_TRUE:
		proc_boolean(ctx, type == DI_TRUE);
		break;
	case DI_NULL:
		proc_null(ctx);
		break;
	case DI_UNDEF:
		proc_undefined(ctx);
		break;
	case DI_SIMPLE:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		if (out->v.i32 <= DI_INFO_MAX) {
			/* not-well-formed range is not used (RFC 8949 3.3) */
			return CBOR_ERROR_MALFORMED_DATA;
		}
		return CBOR_ERROR_UNSUPPORTED_TYPE;
	case DI_FLOAT16:
		SET_READ_ERROR(cbor_di_read_float(data, len, out));
		proc_float16(ctx, out->v.f16);
		break;
	case DI_FLOAT32:
		SET_READ_ERROR(cbor_di_read_float(data, len, out));
		proc_float32(ctx, out->v.i32);
		break;
	case DI_FLOAT64:
		SET_READ_ERROR(cbor_di_read_float(data, len, out));
		proc_float64(ctx, out->v.f64);
		break;
	case DI_BREAK:
		proc_indef_break(ctx);
		break;
	default:
		out->read_len = 0;
		return CBOR_ERROR_MALFORMED_DATA;
	}
FINALLY:
	return error;
}
