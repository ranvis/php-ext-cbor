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
#define ASSERT_STACK_ITEM_IS_TOP(item)  assert(ctx->stack_top == (stack_item *)item)

typedef struct stack_item stack_item;
typedef struct srns_item srns_item;
typedef struct decode_vt decode_vt;

typedef struct cbor_decode_context {
	cbor_decode_args args;
	cbor_error cb_error;
	bool skip_self_desc;
	cbor_fragment *mem;
	stack_item *stack_top, *stack_pool;
	uint32_t stack_depth;
	const decode_vt *vt;
	union dec_ctx_vt_switch {
		struct {
			zval root;
			srns_item *srns; /* string ref namespace */
			HashTable *refs; /* shared ref */
		} zv;
		struct {
			smart_str str;
			int indent_level;
		} edn;
	} u;
} dec_context;

struct decode_vt {
	int si_size;
	void (*ctx_init)(dec_context *ctx);
	void (*ctx_free)(dec_context *ctx);
	cbor_error (*dec_item)(const uint8_t *data, size_t len, cbor_di_decoded *out, dec_context *ctx);
	cbor_error (*dec_finish)(dec_context *ctx, cbor_decode_args *args, cbor_error error, zval *value);
	void (*si_free)(stack_item *item);
	void (*si_push)(dec_context *ctx, stack_item *item, stack_item *parent_item);
	void (*si_pop)(dec_context *ctx, stack_item *item);
};

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

struct stack_item {
	stack_item *next_item;
	const decode_vt *vt;
	si_type_t si_type;
	uint32_t count;
	/* ... */
};

typedef struct stack_item_zv stack_item_zv;

typedef bool (tag_handler_enter_t)(dec_context *ctx, stack_item_zv *item);
typedef void (tag_handler_data_t)(dec_context *ctx, stack_item_zv *item, data_type_t type, zval *value);
typedef void (tag_handler_child_t)(dec_context *ctx, stack_item_zv *item, stack_item_zv *self);
typedef xzval *(tag_handler_exit_t)(dec_context *ctx, xzval *value, stack_item_zv *item, zval *tmp_v);
typedef void (tag_handler_free_t)(stack_item_zv *item);

struct stack_item_zv {
	stack_item base;
	tag_handler_data_t *tag_handler_data;
	tag_handler_child_t *tag_handler_child[2];
	tag_handler_exit_t *tag_handler_exit;
	tag_handler_free_t *tag_handler_free;
	union si_value_t {
		zval value;
		smart_str str;
		struct si_value_map_t {
			zval dest; /* extra: count of added elements for indefinite-length */
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

static void zv_ctx_init(dec_context *ctx);
static void zv_ctx_free(dec_context *ctx);
static cbor_error zv_dec_item(const uint8_t *data, size_t len, cbor_di_decoded *out, dec_context *ctx);
static cbor_error zv_dec_finish(dec_context *ctx, cbor_decode_args *args, cbor_error error, zval *value);
static void zv_si_free(stack_item *item);
static void zv_si_push(dec_context *ctx, stack_item *item, stack_item *parent_item);
static void zv_si_pop(dec_context *ctx, stack_item *item);

static const decode_vt zv_dec_vt = {
	sizeof(stack_item_zv),
	&zv_ctx_init,
	&zv_ctx_free,
	&zv_dec_item,
	&zv_dec_finish,
	&zv_si_free,
	&zv_si_push,
	&zv_si_pop,
};

static void edn_ctx_init(dec_context *ctx);
static void edn_ctx_free(dec_context *ctx);
static cbor_error edn_dec_item(const uint8_t *data, size_t len, cbor_di_decoded *out, dec_context *ctx);
static cbor_error edn_dec_finish(dec_context *ctx, cbor_decode_args *args, cbor_error error, zval *value);
static void edn_si_free(stack_item *item);
static void edn_si_push(dec_context *ctx, stack_item *item, stack_item *parent_item);
static void edn_si_pop(dec_context *ctx, stack_item *item);

static const decode_vt edn_dec_vt = {
	sizeof(stack_item_zv),
	&edn_ctx_init,
	&edn_ctx_free,
	&edn_dec_item,
	&edn_dec_finish,
	&edn_si_free,
	&edn_si_push,
	&edn_si_pop,
};

struct srns_item {  /* srns: string ref namespace */
	srns_item *prev_item;
	HashTable *str_table;
};

static cbor_error decode_nested(dec_context *ctx);

static void free_srns_item(srns_item *srns);

void php_cbor_minit_decode()
{
}

/* just in case, defined as a macro, as function pointer is theoretically incompatible with data pointer */
#define DECLARE_SI_SET_HANDLER_VEC(member)  \
	static bool register_handler_vec_##member(stack_item_zv *item, member##_t *handler) \
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
		for (int i = 0; i < sizeof (si)->member / sizeof (si)->member[0] && (si)->member[i]; i++) { \
			(*(si)->member[i])(__VA_ARGS__); \
		} \
	} while (0)

DECLARE_SI_SET_HANDLER_VEC(tag_handler_child)

#define SI_SET_DATA_HANDLER(si, handler)  SI_SET_HANDLER(tag_handler_data, si, handler)
#define SI_SET_CHILD_HANDLER(si, handler)  SI_SET_HANDLER_VEC(tag_handler_child, si, handler)
#define SI_SET_EXIT_HANDLER(si, handler)  SI_SET_HANDLER(tag_handler_exit, si, handler)
#define SI_SET_FREE_HANDLER(si, handler)  SI_SET_HANDLER(tag_handler_free, si, handler)

#define SI_CALL_CHILD_HANDLER(si, ...)  SI_CALL_HANDLER_VEC(tag_handler_child, si, __VA_ARGS__)

static void stack_free_item(dec_context *ctx, stack_item *item)
{
	if (item == NULL) {
		return;
	}
	item->vt->si_free(item);
	assert(item->next_item == NULL);
	item->next_item = ctx->stack_pool;
	ctx->stack_pool = item;
}

static stack_item *stack_pop_item(dec_context *ctx)
{
	stack_item *item = ctx->stack_top;
	if (item) {
		ctx->stack_top = item->next_item;
		item->next_item = NULL;
		ctx->stack_depth--;
		ctx->vt->si_pop(ctx, item);
	}
	return item;
}

static void stack_push_item(dec_context *ctx, stack_item *item)
{
	stack_item *parent_item = ctx->stack_top;
	if (ctx->stack_depth >= ctx->args.max_depth) {
		stack_free_item(ctx, item);
		RETURN_CB_ERROR(CBOR_ERROR_DEPTH);
	}
	ctx->vt->si_push(ctx, item, parent_item);
	ctx->stack_depth++;
	item->next_item = ctx->stack_top;
	ctx->stack_top = item;
}

static void *stack_new_item(dec_context *ctx, si_type_t si_type, uint32_t count)
{
	stack_item *item = ctx->stack_pool;
	if (item) {
		ctx->stack_pool = item->next_item;
	} else {
		item = emalloc(ctx->vt->si_size);
	}
	memset(item, 0, ctx->vt->si_size);
	item->vt = ctx->vt;
	item->si_type = si_type;
	item->count = count;
	return item;
}

#define FREE_LINKED_LIST(type_t, head, free_f)  do { \
		for (type_t *item = head; item != NULL; ) { \
			type_t *item_tmp = item->prev_item; \
			free_f(item); \
			item = item_tmp; \
		} \
	} while (0)

static void cbor_decode_free(dec_context *ctx)
{
	while (ctx->stack_top) {
		stack_item *item = stack_pop_item(ctx);
		stack_free_item(ctx, item);
	}
	stack_item *ptr = ctx->stack_pool;
	while (ptr) {
		stack_item *next = ptr->next_item;
		efree(ptr);
		ptr = next;
	}
	ctx->vt->ctx_free(ctx);
}

static void cbor_decode_init(dec_context *ctx, const cbor_decode_args *args, cbor_fragment *mem)
{
	ctx->skip_self_desc = !(args->flags & CBOR_SELF_DESCRIBE);
	ctx->stack_top = ctx->stack_pool = NULL;
	ctx->stack_depth = 0;
	ctx->args = *args;
	ctx->mem = mem;
	if (args->flags & CBOR_EDN) {
		ctx->vt = &edn_dec_vt;
	} else {
		ctx->vt = &zv_dec_vt;
	}
	ctx->vt->ctx_init(ctx);
}

dec_context *php_cbor_decode_new(const cbor_decode_args *args, cbor_fragment *mem)
{
	dec_context *ctx = emalloc(sizeof(dec_context));
	cbor_decode_init(ctx, args, mem);
	return ctx;
}

void php_cbor_decode_delete(dec_context *ctx)
{
	cbor_decode_free(ctx);
	efree(ctx);
}

cbor_error php_cbor_decode_process(dec_context *ctx)
{
	cbor_error error;
	if (ctx->skip_self_desc) {
		cbor_fragment *mem = ctx->mem;
		size_t rem_len = mem->length - mem->offset;
		if (!rem_len) {
			error = CBOR_ERROR_TRUNCATED_DATA;
			goto FINALLY;
		}
		assert(sizeof CBOR_SELF_DESCRIBE_DATA == 4 && (uint8_t)CBOR_SELF_DESCRIBE_DATA[0] == 0xd9);
		if (mem->ptr[mem->offset] == (uint8_t)CBOR_SELF_DESCRIBE_DATA[0]) {
			if (rem_len < sizeof CBOR_SELF_DESCRIBE_DATA - 1) {
				error = CBOR_ERROR_TRUNCATED_DATA;
				goto FINALLY;
			}
			if (!memcmp(&mem->ptr[mem->offset], CBOR_SELF_DESCRIBE_DATA, sizeof CBOR_SELF_DESCRIBE_DATA - 1)) {
				mem->offset += sizeof CBOR_SELF_DESCRIBE_DATA - 1;
			}
		}
		ctx->skip_self_desc = false;
	}
	error = decode_nested(ctx);
	if (ctx->skip_self_desc && error != CBOR_ERROR_TRUNCATED_DATA) {
		ctx->skip_self_desc = false;
	}
FINALLY:
	if (error) {
		cbor_fragment *mem = ctx->mem;
		ctx->args.error_arg = mem->base + mem->offset;
	}
	return error;
}

cbor_error php_cbor_decode_finish(dec_context *ctx, cbor_decode_args *args, cbor_error error, zval *value)
{
	return ctx->vt->dec_finish(ctx, args, error, value);
}

cbor_error php_cbor_decode(zend_string *data, zval *value, cbor_decode_args *args)
{
	cbor_error error = 0;
	dec_context ctx;
	cbor_fragment mem;
	mem.offset = args->offset;
	mem.length = ZSTR_LEN(data);
	if (mem.offset > mem.length) {
		error = CBOR_ERROR_TRUNCATED_DATA;
		args->error_arg = mem.length;
	} else if (args->length != LEN_DEFAULT) {
		if ((size_t)args->length > mem.length || mem.offset > mem.length - args->length) {
			error = CBOR_ERROR_TRUNCATED_DATA;
			args->error_arg = mem.length;
		}
		mem.length = mem.offset + args->length;
	}
	mem.base = 0;
	mem.limit = mem.length;
	mem.ptr = (const uint8_t *)ZSTR_VAL(data);
	if (!error) {
		cbor_decode_init(&ctx, args, &mem);
		error = php_cbor_decode_process(&ctx);
		if (!error && mem.offset != mem.length) {
			error = CBOR_ERROR_EXTRANEOUS_DATA;
			ctx.args.error_arg = mem.base + mem.offset;
		}
		error = php_cbor_decode_finish(&ctx, args, error, value);
		cbor_decode_free(&ctx);
	}
	return error;
}

static cbor_error decode_nested(dec_context *ctx)
{
	cbor_error error;
	cbor_fragment *mem = ctx->mem;
	cbor_di_decoded out_data;
	ctx->cb_error = 0;
	do {
		if (mem->offset >= mem->length) {
			return CBOR_ERROR_TRUNCATED_DATA;
		}
		error = ctx->vt->dec_item(mem->ptr + mem->offset, mem->length - mem->offset, &out_data, ctx);
		mem->offset += out_data.read_len;
		if (error) {
			return error;
		}
		if (ctx->cb_error) {
			return ctx->cb_error;
		}
	} while (ctx->stack_depth);
	return 0;
}

#define CBOR_INT_BUF_SIZE  24 /* ceil(log10(2)*64) = 20 */

static size_t cbor_int_to_str(char *buf, uint64_t value, bool is_negative)
{
	size_t len;
	if (!is_negative) {
		len = snprintf(buf, CBOR_INT_BUF_SIZE, "%" PRIu64, value);
	} else {
		uint64_t n_value = value;
		bool fix;
		n_value++;
		if ((fix = !n_value) != 0) {
			n_value--;
		}
		len = snprintf(buf, CBOR_INT_BUF_SIZE, "-%" PRIu64, n_value);
		assert(len >= 1 && len < CBOR_INT_BUF_SIZE);
		if (fix) {
			buf[len - 1]++;  /* "-18446744073709551615" => "...6" */
		}
	}
	return len;
}

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
		if (item->tag_handler_free) {
			(*item->tag_handler_free)(item);
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
		if (EXPECTED(Z_TYPE(ctx->u.zv.root) != IS_REFERENCE)) {
			ZVAL_COPY_VALUE(value, &ctx->u.zv.root);
		} else {
			zval *tmp = &ctx->u.zv.root;
			ZVAL_DEREF(tmp);
			ZVAL_COPY_VALUE(value, tmp);
			Z_TRY_ADDREF_P(value);
			zval_ptr_dtor(&ctx->u.zv.root);
		}
	} else {
		args->error_arg = ctx->args.error_arg;
		zval_ptr_dtor(&ctx->u.zv.root);
	}
	ZVAL_UNDEF(&ctx->u.zv.root);
	return error;
}

static void zv_stack_push_counted(dec_context *ctx, si_type_t si_type, zval *value, uint32_t count)
{
	stack_item_zv *item = stack_new_item(ctx, si_type, count);
	ZVAL_COPY_VALUE(&item->v.value, value);
	stack_push_item(ctx, &item->base);
}

static void zv_stack_push_xstring(dec_context *ctx, si_type_t si_type)
{
	stack_item_zv *item = stack_new_item(ctx, si_type, 0);
	stack_push_item(ctx, &item->base);
}

static void zv_stack_push_map(dec_context *ctx, si_type_t si_type, zval *value, uint32_t count)
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
	ZVAL_LONG(&item->v.tag_id, tag_id);
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
			ZVAL_COPY_VALUE(&item->v.map.key, value);
			Z_TRY_ADDREF_P(value);
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
	zend_call_known_instance_method_with_2_params(CBOR_CE(tag)->constructor, Z_OBJ(container), NULL, &item->v.tag_id, value);
	ASSERT_STACK_ITEM_IS_TOP(item);
	stack_pop_item(ctx);
	assert(Z_TYPE(item->v.tag_id) == IS_LONG);
	stack_free_item(ctx, &item->base);
	result = zv_append(ctx, &container);
	zval_ptr_dtor(&container);
	return result;
}

static bool zv_append_to_tag_handled(dec_context *ctx, xzval *value, stack_item_zv *item)
{
	zval tmp_v;
	xzval *orig_v = value;
	bool result;
	ASSERT_STACK_ITEM_IS_TOP(item);
	stack_pop_item(ctx);
	if (item->tag_handler_exit)  {
		assert(!ctx->cb_error);
		value = (*item->tag_handler_exit)(ctx, value, item, &tmp_v);
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
		Z_TRY_ADDREF_P(value);
		ZVAL_COPY_VALUE(&ctx->u.zv.root, value);
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
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_SYNTAX, NESTED_INDEF_STRING));
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
		/* do nothing*/
	} else {
		zend_object *obj = cbor_xstring_create(string_ce);
		cbor_xstring_set_value(obj, Z_STR_P(value));
		ZVAL_OBJ(&container, obj);
		value = &container;
	}
	if (!is_indef && item && item->tag_handler_data) {
		(*item->tag_handler_data)(ctx, item, DATA_TYPE_STRING, value);
	}
	result = zv_append(ctx, value);
	if (!Z_ISNULL(container)) {
		zval_ptr_dtor(&container);
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
		si_type_t str_si_type = is_text ? SI_TYPE_TEXT : SI_TYPE_BYTE;
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
	zval_ptr_dtor(&value);
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
	array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
	if (count) {
		zv_stack_push_counted(ctx, SI_TYPE_ARRAY, &value, (uint32_t)count);
	} else {
		zv_append(ctx, &value);
		zval_ptr_dtor(&value);
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
		array_init_size(&value, ((count > SIZE_INIT_LIMIT) ? SIZE_INIT_LIMIT : (uint32_t)count));
	} else {
		ZVAL_OBJ(&value, zend_objects_new(zend_standard_class_def));
	}
	if (count) {
		zv_stack_push_map(ctx, SI_TYPE_MAP, &value, (uint32_t)count);
	} else {
		zv_append(ctx, &value);
		zval_ptr_dtor(&value);
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
	zval_ptr_dtor(&container);
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
	Z_ADDREF(value);
	zv_append(ctx, &value);
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
		zval_ptr_dtor(&value);
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

bool cbor_is_len_string_ref(size_t str_len, uint32_t next_index)
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

static void tag_handler_str_ref_ns_data(dec_context *ctx, stack_item_zv *item, data_type_t type, zval *value)
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
	SI_SET_CHILD_HANDLER(item, &tag_handler_str_ref_ns_child);
	SI_SET_DATA_HANDLER(item, &tag_handler_str_ref_ns_data);
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
	SI_SET_DATA_HANDLER(item, &tag_handler_str_ref_ns_data);
	SI_SET_CHILD_HANDLER(item, &tag_handler_str_ref_ns_child);
	SI_SET_EXIT_HANDLER(item, &tag_handler_str_ref_ns_exit);
	SI_SET_FREE_HANDLER(item, &tag_handler_str_ref_ns_free);
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
	Z_ADDREF_P(str);
	ZVAL_COPY_VALUE(tmp_v, str);
	return tmp_v;
}

static bool tag_handler_str_ref_enter(dec_context *ctx, stack_item_zv *item)
{
	SI_SET_EXIT_HANDLER(item, &tag_handler_str_ref_exit);
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
			RETURN_CB_ERROR(E_DESC(CBOR_ERROR_TAG_TYPE, SHARE_INCOMPATIBLE));
		}
	}
	if (zend_hash_index_update(ctx->u.zv.refs, self->v.tag_h.v.shareable.index, real_v) == NULL) {
		zval_ptr_dtor(real_v);
		RETURN_CB_ERROR(CBOR_ERROR_INTERNAL);
	} else {
		Z_ADDREF_P(real_v);
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
			RETURN_CB_ERROR_V(value, E_DESC(CBOR_ERROR_TAG_TYPE, SHARE_INCOMPATIBLE));
		}
		if (zend_hash_index_update(ctx->u.zv.refs, item->v.tag_h.v.shareable.index, real_v) == NULL) {
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

static bool tag_handler_shareable_enter(dec_context *ctx, stack_item_zv *item)
{
	if (ctx->stack_top && ctx->stack_top->si_type == SI_TYPE_TAG_HANDLED && ((stack_item_zv *)ctx->stack_top)->v.tag_h.tag_id == CBOR_TAG_SHAREABLE) {
		/* nested shareable */
		RETURN_CB_ERROR_B(E_DESC(CBOR_ERROR_TAG_SYNTAX, SHARE_NESTED));
	}
	SI_SET_CHILD_HANDLER(item, &tag_handler_shareable_child);
	SI_SET_EXIT_HANDLER(item, &tag_handler_shareable_exit);
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
	SI_SET_EXIT_HANDLER(item, &tag_handler_shared_ref_exit);
	return true;
}

static bool zv_do_tag_enter(dec_context *ctx, zend_long tag_id)
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
		stack_item_zv *item = stack_new_item(ctx, SI_TYPE_TAG_HANDLED, 1);
		item->v.tag_h.tag_id = tag_id;
		if (!(*handler)(ctx, item)) {
			ASSERT_ERROR_SET();
			stack_free_item(ctx, &item->base);
			return true;
		}
		stack_push_item(ctx, &item->base);
		return true;
	}
	return false;
}

#define METHOD(name) zv_##name
#include "decode_base.c"

#include "decode_edn.c"
