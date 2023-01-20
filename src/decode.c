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
#include <Zend/zend_smart_str.h>
#include <assert.h>

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

enum tag_handler_index_value {
	THI_NONE = 0,
	THI_STR_REF_NS,
	THI_STR_REF,
	THI_SHAREABLE,
	THI_SHARED_REF,
	THI_COUNT,
};

typedef uint8_t tag_handler_index;

typedef struct {
	tag_handler_enter_t *h_enter;
	tag_handler_exit_t *h_exit;
	tag_handler_free_t *h_free;
	tag_handler_data_t *h_data;
	tag_handler_child_t *h_child;
} tag_handlers_t;

static tag_handlers_t tag_handlers[THI_COUNT]; /* cannot be const for VC++ */

struct stack_item_zv {
	stack_item base;
	tag_handler_index thi_data;
	tag_handler_index thi_child[2];
	union si_value_t {
		zval value;
		smart_str str;
		struct si_value_map_t {
			zval dest; /* extra: count of added elements for indefinite-length */
			zval key;
		} map;
		zend_long tag_id;
		struct si_value_tag_handled_t {
			zend_long id;
			tag_handler_index thi;
			union si_value_tag_h_value_t {
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
	static bool register_handler_vec_##member(stack_item_zv *item, tag_handler_index thi) \
	{ \
		int i, count = sizeof item->member / sizeof item->member[0]; \
		for (i = 0; i < count && item->member[i] != THI_NONE; i++) { \
			if (item->member[i] == thi) { \
				return true; \
			} \
		} \
		assert(i < count); \
		if (i >= count) { \
			return false; \
		} \
		item->member[i] = thi; \
		return true; \
	}
#define SI_SET_HANDLER_VEC(member, si, handler)  do { \
		if (!register_handler_vec_##member(si, handler) && !ctx->cb_error) { \
			ctx->cb_error = CBOR_ERROR_INTERNAL; \
		} \
	} while (0)
#define SI_SET_HANDLER(member, si, thi)  si->member = thi
#define SI_CALL_HANDLER(si, thi, member, ...)  (*tag_handlers[si->thi].member)(__VA_ARGS__)
#define SI_CALL_HANDLER_VEC(member, h_member, si, ...)  do { \
		for (int i = 0; i < sizeof (si)->member / sizeof (si)->member[0] && (si)->member[i]; i++) { \
			(*tag_handlers[(si)->member[i]].h_member)(__VA_ARGS__); \
		} \
	} while (0)

DECLARE_SI_SET_HANDLER_VEC(thi_child)

#define SI_SET_DATA_HANDLER(si, thi)  SI_SET_HANDLER(thi_data, si, thi)
#define SI_SET_CHILD_HANDLER(si, thi)  SI_SET_HANDLER_VEC(thi_child, si, thi)

#define SI_CALL_CHILD_HANDLER(si, ...)  SI_CALL_HANDLER_VEC(thi_child, h_child, si, __VA_ARGS__)

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

#include "decode_zv.h"
#include "decode_edn.h"
