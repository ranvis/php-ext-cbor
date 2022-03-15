/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "private.h"
#include "compatibility.h"
#include "tags.h"
#include "utf8.h"

#define BX_INIT(ctx_p) \
	size_t bx_extend_len = 0; \
	size_t bx_len = 0; \
	smart_str *bx_buf = (ctx_p)->buf; \
	char *bx_ptr
#define BX_ALLOC(n)  \
	assert(bx_extend_len == 0), \
	bx_extend_len = n, \
	bx_ptr = smart_str_extend(bx_buf, bx_extend_len)
#define BX_ARG_PTR  bx_ptr
#define BX_ARG_LEN  bx_extend_len
#define BX_ARGS  (unsigned char *)BX_ARG_PTR, BX_ARG_LEN
#define BX_PUT(stmt)  bx_len = (stmt)
#define BX_ADV()  do { \
		assert(bx_len); \
		bx_ptr += bx_len, bx_extend_len -= bx_len; \
	} while (false)
#define BX_CANCEL()  do { \
		if (bx_buf->s) { \
			ZSTR_LEN(bx_buf->s) -= bx_extend_len, bx_extend_len = 0, bx_len = 0; \
		} \
	} while (false)
#define BX_END()  do { \
		assert(bx_len); \
		assert(bx_extend_len >= bx_len); \
		if (bx_extend_len > 0) { \
			ZSTR_LEN(bx_buf->s) -= bx_extend_len - bx_len; \
			bx_extend_len = 0; \
		} \
	} while (false)
#define BX_END_CHECK()  do { \
		if (UNEXPECTED(bx_len == 0)) { \
			return PHP_CBOR_ERROR_INTERNAL; \
		} \
		BX_END(); \
	} while (false)

#define CTX_TEXT_FLAG(ctx)  (((ctx)->args.flags & PHP_CBOR_TEXT) != 0)

typedef struct {
	uint32_t next_index;
	HashTable *str_table[2];
} srns_item_t;

typedef struct {
	php_cbor_encode_args args;
	uint32_t cur_depth;
	smart_str *buf;
	srns_item_t *srns; /* string ref namespace */
	HashTable *refs, *ref_lock; /* shared ref, lock is actually not needed fow now */
} enc_context;

typedef enum {
	HASH_ARRAY = 0,
	HASH_OBJ,
	HASH_STD_CLASS,
} hash_type;

static php_cbor_error enc_zval(enc_context *ctx, zval *value);
static php_cbor_error enc_long(enc_context *ctx, zend_long value);
static php_cbor_error enc_string(enc_context *ctx, zend_string *value, bool to_text);
static php_cbor_error enc_string_len(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text);
static php_cbor_error enc_hash(enc_context *ctx, zval *value, hash_type type);
static php_cbor_error enc_typed_byte(enc_context *ctx, zval *ins);
static php_cbor_error enc_typed_text(enc_context *ctx, zval *ins);
static php_cbor_error enc_typed_floatx(enc_context *ctx, zval *ins, int bits);
static php_cbor_error enc_tag(enc_context *ctx, zval *ins);
static php_cbor_error enc_tag_bare(enc_context *ctx, zend_long tag_id);

static void init_srns_stack(enc_context *ctx);
static void free_srns_stack(enc_context *ctx);
static php_cbor_error enc_string_ref(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text);
static php_cbor_error enc_ref_counted(enc_context *ctx, zval *value);
static php_cbor_error enc_shareable(enc_context *ctx, zval *value);

void php_cbor_minit_encode()
{
}

#define ENC_RESULT(r)  do { \
		error = (r); \
		goto ENCODED; \
	} while (0);
#define ENC_CHECK(r)  do { \
		if ((error = (r)) != 0) { \
			goto ENCODED; \
		} \
	} while (0);

php_cbor_error php_cbor_encode(zval *value, zend_string **data, const php_cbor_encode_args *args)
{
	php_cbor_error error;
	enc_context ctx;
	smart_str buf = {0};
	ctx.args = *args;
	ctx.cur_depth = 0;
	ctx.buf = &buf;
	if ((ctx.args.flags & PHP_CBOR_BYTE && ctx.args.flags & PHP_CBOR_TEXT)
			|| (ctx.args.flags & PHP_CBOR_KEY_BYTE && ctx.args.flags & PHP_CBOR_KEY_TEXT)
			|| (ctx.args.flags & PHP_CBOR_FLOAT16 && ctx.args.flags & PHP_CBOR_FLOAT32)) {
		return PHP_CBOR_ERROR_INVALID_FLAGS;
	}
	ctx.srns = NULL;
	ctx.refs = ctx.ref_lock = NULL;
	if (ctx.args.flags & PHP_CBOR_SELF_DESCRIBE) {
		ENC_CHECK(enc_tag_bare(&ctx, PHP_CBOR_TAG_SELF_DESCRIBE));
	}
	if (ctx.args.string_ref == OPT_TRUE) {
		ENC_CHECK(enc_tag_bare(&ctx, PHP_CBOR_TAG_STRING_REF_NS));
		init_srns_stack(&ctx);
	}
	if (ctx.args.shared_ref) {
		ctx.refs = zend_new_array(0);
		ctx.ref_lock = zend_new_array(0);
	}
	error = enc_zval(&ctx, value);
ENCODED:
	free_srns_stack(&ctx);
	if (ctx.refs) {
		zend_array_destroy(ctx.refs);
	}
	if (ctx.ref_lock) {
		zend_array_destroy(ctx.ref_lock);
	}
	if (!error) {
		*data = smart_str_extract(&buf);
	} else {
		smart_str_free(&buf);
	}
	return error;
}

static php_cbor_error enc_zval(enc_context *ctx, zval *value)
{
	php_cbor_error error = 0;
	BX_INIT(ctx);
	if (ctx->cur_depth++ > ctx->args.max_depth) {
		return PHP_CBOR_ERROR_DEPTH;
	}
RETRY:
	switch (Z_TYPE_P(value)) {
	case IS_NULL:
		BX_ALLOC(1);
		BX_PUT(cbor_encode_null(BX_ARGS));
		break;
	case IS_FALSE:
		BX_ALLOC(1);
		BX_PUT(cbor_encode_bool(false, BX_ARGS));
		break;
	case IS_TRUE:
		BX_ALLOC(1);
		BX_PUT(cbor_encode_bool(true, BX_ARGS));
		break;
	case IS_LONG:
		ENC_RESULT(enc_long(ctx, Z_LVAL_P(value)));
	case IS_DOUBLE:
		if (UNEXPECTED(ctx->args.flags & PHP_CBOR_FLOAT16)) {
			BX_ALLOC(3);
			BX_PUT(cbor_encode_half((float)Z_DVAL_P(value), BX_ARGS));
		} else if (UNEXPECTED(ctx->args.flags & PHP_CBOR_FLOAT32)) {
			BX_ALLOC(5);
			BX_PUT(cbor_encode_single((float)Z_DVAL_P(value), BX_ARGS));
		} else {
			BX_ALLOC(9);
			BX_PUT(cbor_encode_double(Z_DVAL_P(value), BX_ARGS));
		}
		break;
	case IS_STRING:
		if (!(ctx->args.flags & (PHP_CBOR_BYTE | PHP_CBOR_TEXT))) {
			error = PHP_CBOR_ERROR_INVALID_FLAGS;
			break;
		}
		ENC_RESULT(enc_string(ctx, Z_STR_P(value), CTX_TEXT_FLAG(ctx)));
	case IS_ARRAY:
		ENC_RESULT(enc_hash(ctx, value, HASH_ARRAY));
	case IS_OBJECT: {
		zend_class_entry *ce = Z_OBJCE_P(value);
		/* Cbor types are 'final' */
		if (ce == CBOR_CE(undefined)) {
			BX_ALLOC(1);
			BX_PUT(cbor_encode_undef(BX_ARGS));
		} else if (ce == CBOR_CE(byte)) {
			ENC_RESULT(enc_typed_byte(ctx, value));
		} else if (ce == CBOR_CE(text)) {
			ENC_RESULT(enc_typed_text(ctx, value));
		} else if (ce == CBOR_CE(float16)) {
			ENC_RESULT(enc_typed_floatx(ctx, value, 16));
		} else if (ce == CBOR_CE(float32)) {
			ENC_RESULT(enc_typed_floatx(ctx, value, 32));
		} else if (ce == CBOR_CE(tag)) {
			ENC_RESULT(enc_tag(ctx, value));
		} else if (ce == CBOR_CE(shareable)) {
			ENC_RESULT(enc_shareable(ctx, value));
		} else if (ce == zend_standard_class_def) {
			if (ctx->args.shared_ref && Z_REFCOUNT_P(value) > 1) {
				error = enc_ref_counted(ctx, value);
				if (error != PHP_CBOR_STATUS_VALUE_FOLLOWS) {
					ENC_RESULT(error);
				}
			}
			ENC_RESULT(enc_hash(ctx, value, HASH_STD_CLASS));
		} else {
			/* TODO: */
			BX_ALLOC(1);
			BX_PUT(cbor_encode_undef(BX_ARGS));
			error = PHP_CBOR_ERROR_UNSUPPORTED_TYPE;
		}
		break;
	}
	case IS_REFERENCE:
		if (ctx->args.shared_ref == OPT_UNSAFE_REF && Z_TYPE_P(Z_REFVAL_P(value)) != IS_OBJECT) {
			/* no CBOR representation of &object is defined as of writing (double-sharedref or indirection(22098) is not what it means) */
			/* PHP reference actually shares container of the value, not the value it contains after all */
			error = enc_ref_counted(ctx, value);
			if (error != PHP_CBOR_STATUS_VALUE_FOLLOWS) {
				ENC_RESULT(error);
			}
		}
		ZVAL_DEREF(value);
		goto RETRY;
	default:
		error = PHP_CBOR_ERROR_UNSUPPORTED_TYPE;
	}
	if (error) {
		BX_CANCEL();
	} else {
		BX_END_CHECK();
	}
ENCODED:
	ctx->cur_depth--;
	return error;
}

static php_cbor_error enc_long(enc_context *ctx, zend_long value)
{
	BX_INIT(ctx);
	BX_ALLOC(9);
	if (value >= 0) {
		BX_PUT(cbor_encode_uint(value, BX_ARGS));
	} else {
		BX_PUT(cbor_encode_negint(-(value + 1), BX_ARGS));
	}
	BX_END_CHECK();
	return 0;
}

static php_cbor_error enc_string(enc_context *ctx, zend_string *value, bool to_text)
{
	return enc_string_len(ctx, ZSTR_VAL(value), ZSTR_LEN(value), value, to_text);
}

static php_cbor_error enc_string_len(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text)
{
	BX_INIT(ctx);
	php_cbor_error error;
	if (ctx->srns) {
		error = enc_string_ref(ctx, value, length, v_str, to_text);
		if (error != PHP_CBOR_STATUS_VALUE_FOLLOWS) {
			return error;
		}
	}
	BX_ALLOC(9 + length);
	if (to_text) {
		if (!(ctx->args.flags & PHP_CBOR_UNSAFE_TEXT)
				&& !is_utf8((uint8_t *)value, length)) {
			return PHP_CBOR_ERROR_UTF8;
		}
		BX_PUT(cbor_encode_string_start(length, BX_ARGS));
	} else {
		BX_PUT(cbor_encode_bytestring_start(length, BX_ARGS));
	}
	if (length) {
		BX_ADV();
		memcpy(BX_ARG_PTR, value, length);
		BX_PUT(length);
	}
	BX_END_CHECK();
	return 0;
}

static php_cbor_error enc_hash(enc_context *ctx, zval *value, hash_type type)
{
	php_cbor_error error = 0;
	HashTable *ht = NULL;
	zend_object *obj = NULL;
	zend_ulong count;
	bool to_text = (ctx->args.flags & PHP_CBOR_KEY_TEXT) != 0;
	bool key_flag_error = !(ctx->args.flags & (PHP_CBOR_KEY_BYTE | PHP_CBOR_KEY_TEXT));
	bool is_list;
	bool is_indef_length = false;
	bool use_int_key = ctx->args.flags & PHP_CBOR_INT_KEY;
	BX_INIT(ctx);
	if (type != HASH_ARRAY) {
		obj = Z_OBJ_P(value);
		if (GC_IS_RECURSIVE(obj)) {
			return PHP_CBOR_ERROR_RECURSION;
		}
	} else {
		ht = Z_ARR_P(value);
		if (GC_IS_RECURSIVE(ht)) {
			return PHP_CBOR_ERROR_RECURSION;
		}
	}
	if (type != HASH_ARRAY) {
		ht = zend_get_properties_for(value, ZEND_PROP_PURPOSE_JSON);
	}
	is_list = type == HASH_ARRAY && zend_array_is_list(ht);
	count = zend_hash_num_elements(ht);
	BX_ALLOC(9);
	if ((type == HASH_OBJ && count)
			|| UNEXPECTED(count > 0xff && (ctx->args.flags & PHP_CBOR_PACKED))) {
		is_indef_length = true;
		if (is_list) {
			BX_PUT(cbor_encode_indef_array_start(BX_ARGS));
		} else {
			BX_PUT(cbor_encode_indef_map_start(BX_ARGS));
		}
	} else {
		if (is_list) {
			BX_PUT(cbor_encode_array_start(count, BX_ARGS));
		} else {
			BX_PUT(cbor_encode_map_start(count, BX_ARGS));
		}
	}
	BX_END_CHECK();
	if (count) {
		zend_ulong index;
		zend_string *key;
		zval *data;
		if (type != HASH_ARRAY) {
			GC_TRY_PROTECT_RECURSION(obj);
		} else {
			GC_TRY_PROTECT_RECURSION(ht);
		}
		ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
			if (!is_list) {
				if (key) {
					/* check property visibility if it is object and not stdClass */
					if (type == HASH_OBJ && *ZSTR_VAL(key) == '\0' && ZSTR_LEN(key) > 0) {
						continue; /* skip if not a public property */
					}
					if (UNEXPECTED(key_flag_error)) {
						error = PHP_CBOR_ERROR_INVALID_FLAGS;
					} else {
						error = enc_string(ctx, key, to_text);
					}
				} else if (type != HASH_ARRAY || !use_int_key) {
					char num_str[ZEND_LTOA_BUF_LEN];
					ZEND_LTOA((zend_long)index, num_str, sizeof num_str);
					if (UNEXPECTED(key_flag_error)) {
						error = PHP_CBOR_ERROR_INVALID_FLAGS;
					} else {
						error = enc_string_len(ctx, num_str, strlen(num_str), NULL, to_text);
					}
				} else {
					error = enc_long(ctx, (zend_long)index);
				}
			}
			if (UNEXPECTED(error)) {
				break;
			}
			if ((error = enc_zval(ctx, data)) != 0) {
				break;
			}
			count--;
		}
		ZEND_HASH_FOREACH_END();
		if (!error && count) {
			error = PHP_CBOR_ERROR_INTERNAL;
		}
		if (type != HASH_ARRAY) {
			GC_TRY_UNPROTECT_RECURSION(obj);
		} else {
			GC_TRY_UNPROTECT_RECURSION(ht);
		}
	}
	if (type != HASH_ARRAY) {
		zend_release_properties(ht);
	}
	if (is_indef_length) {
		BX_ALLOC(8);
		BX_PUT(cbor_encode_break(BX_ARGS));
		BX_END_CHECK();
	}
	return error;
}

#define PROP_L(prop_literal) prop_literal, sizeof prop_literal - 1

static php_cbor_error enc_typed_byte(enc_context *ctx, zval *ins)
{
	zval tmp_v, *value;
	value = zend_read_property(CBOR_CE(byte), Z_OBJ_P(ins), PROP_L("value"), false, &tmp_v);
	if (!value) {
		return PHP_CBOR_ERROR_INTERNAL;
	}
	return enc_string(ctx, Z_STR_P(value), false);
}

static php_cbor_error enc_typed_text(enc_context *ctx, zval *ins)
{
	zval tmp_v, *value;
	value = zend_read_property(CBOR_CE(text), Z_OBJ_P(ins), PROP_L("value"), false, &tmp_v);
	if (!value) {
		return PHP_CBOR_ERROR_INTERNAL;
	}
	return enc_string(ctx, Z_STR_P(value), true);
}

static php_cbor_error enc_typed_floatx(enc_context *ctx, zval *ins, int bits)
{
	zval tmp_v, *value;
	value = zend_read_property(CBOR_CE(floatx), Z_OBJ_P(ins), PROP_L("value"), false, &tmp_v);
	if (!value) {
		return PHP_CBOR_ERROR_INTERNAL;
	}
	BX_INIT(ctx);
	BX_ALLOC(8);
	if (bits == 16) {
		BX_PUT(cbor_encode_half((float)Z_DVAL_P(value), BX_ARGS));
	} else {
		BX_PUT(cbor_encode_single((float)Z_DVAL_P(value), BX_ARGS));
	}
	BX_END_CHECK();
	return 0;
}

static php_cbor_error enc_tag_bare(enc_context *ctx, zend_long tag_id)
{
	BX_INIT(ctx);
	BX_ALLOC(8);
	/* XXX: tag above ZEND_LONG_MAX cannot be created. As of writing there is one valid tag like this. */
	assert(tag_id >= 0);
	BX_PUT(cbor_encode_tag(tag_id, BX_ARGS));
	BX_END_CHECK();
	return 0;
}

static php_cbor_error enc_tag(enc_context *ctx, zval *ins)
{
	php_cbor_error error = 0;
	zval tmp_tag, tmp_content;
	zval *tag, *content;
	zend_long tag_id;
	srns_item_t *orig_srns = NULL;
	bool new_srns = false;
	tag = zend_read_property(CBOR_CE(tag), Z_OBJ_P(ins), PROP_L("tag"), false, &tmp_tag);
	content = zend_read_property(CBOR_CE(tag), Z_OBJ_P(ins), PROP_L("content"), false, &tmp_content);
	if (!tag || !content) {
		return PHP_CBOR_ERROR_INTERNAL;
	}
	tag_id = Z_LVAL_P(tag);
	if (tag_id < 0) {
		return PHP_CBOR_ERROR_SYNTAX;
	}
	ENC_CHECK(enc_tag_bare(ctx, tag_id));
	if (tag_id == PHP_CBOR_TAG_STRING_REF_NS && ctx->args.string_ref) {
		new_srns = true;
		orig_srns = ctx->srns;
		init_srns_stack(ctx);
	} else if (tag_id == PHP_CBOR_TAG_STRING_REF && ctx->args.string_ref) {
		zend_long tag_content;
		if (Z_TYPE_P(content) != IS_LONG) {
			return PHP_CBOR_ERROR_TAG_TYPE;
		}
		if (!ctx->srns) {
			return PHP_CBOR_ERROR_TAG_SYNTAX;
		}
		tag_content = Z_LVAL_P(content);
		if (tag_content < 0 || tag_content >= ctx->srns->next_index) {
			return PHP_CBOR_ERROR_TAG_VALUE;
		}
	}
	error = enc_zval(ctx, content);
ENCODED:
	if (new_srns) {
		free_srns_stack(ctx);
		ctx->srns = orig_srns;
	}
	return error;
}

static void init_srns_stack(enc_context *ctx)
{
	srns_item_t *srns = (srns_item_t *)emalloc(sizeof *srns);
	ctx->srns = srns;
	srns->next_index = 0;
	srns->str_table[0] = zend_new_array(0);
	srns->str_table[1] = zend_new_array(0);
}

static void free_srns_stack(enc_context *ctx)
{
	srns_item_t *srns = ctx->srns;
	if (srns) {
		zend_array_release(srns->str_table[0]);
		zend_array_release(srns->str_table[1]);
		efree(srns);
	}
}

static php_cbor_error enc_string_ref(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text)
{
	php_cbor_error error = 0;
	srns_item_t *srns = ctx->srns;
	HashTable *str_table;
	int table_index;
	zval new_index, *str_index;
	bool created = false;
	assert(ctx->srns);
	table_index = to_text ? 1 : 0;
	if (!v_str) {
		v_str = zend_string_init_fast(value, length);
		created = true;
	}
	str_table = srns->str_table[table_index];
	str_index = zend_hash_find(str_table, v_str);
	if (str_index) {
		ENC_CHECK(enc_tag_bare(ctx, PHP_CBOR_TAG_STRING_REF));
		ENC_RESULT(enc_long(ctx, Z_LVAL_P(str_index)));
	}
	if (!php_cbor_is_len_string_ref(length, srns->next_index)) {
		ENC_CHECK(PHP_CBOR_STATUS_VALUE_FOLLOWS);
	}
	if (srns->next_index == ZEND_LONG_MAX) {  /* until max - 1 for simplicity */
		ENC_CHECK(PHP_CBOR_ERROR_INTERNAL);
	}
	ZVAL_LONG(&new_index, srns->next_index);
	srns->next_index++;
	if (!zend_hash_add_new(str_table, v_str, &new_index)) {
		ENC_CHECK(PHP_CBOR_ERROR_INTERNAL);
	}
	error = PHP_CBOR_STATUS_VALUE_FOLLOWS;
ENCODED:
	if (created) {
		zend_string_release(v_str);
	}
	return error;
}

static php_cbor_error enc_ref_counted(enc_context *ctx, zval *value)
{
#if ZEND_ULONG_MAX < UINTPTR_MAX
#error "Unimplemented: maybe use pointer as a binary string key?"
#endif
	php_cbor_error error = 0;
	zend_ulong key_index = (zend_ulong)Z_COUNTED_P(value);
	zval new_index, *ref_index;
	assert(Z_REFCOUNTED_P(value));
	if ((ref_index = zend_hash_index_find(ctx->refs, key_index)) != NULL) {
		ENC_CHECK(enc_tag_bare(ctx, PHP_CBOR_TAG_SHARED_REF));
		ENC_RESULT(enc_long(ctx, Z_LVAL_P(ref_index)));
	}
	ZVAL_LONG(&new_index, zend_hash_num_elements(ctx->refs));
	if (!zend_hash_index_add_new(ctx->refs, key_index, &new_index)
			|| !zend_hash_next_index_insert_new(ctx->ref_lock, value)) {
		return PHP_CBOR_ERROR_INTERNAL;
	}
	Z_TRY_ADDREF_P(value);
	ENC_CHECK(enc_tag_bare(ctx, PHP_CBOR_TAG_SHAREABLE));
	error = PHP_CBOR_STATUS_VALUE_FOLLOWS;
ENCODED:
	return error;
}

static php_cbor_error enc_shareable(enc_context *ctx, zval *value)
{
	php_cbor_error error = 0;
	zval tmp_v;
	error = enc_ref_counted(ctx, value);
	if (error != PHP_CBOR_STATUS_VALUE_FOLLOWS) {
		return error;
	}
	value = zend_read_property(CBOR_CE(shareable), Z_OBJ_P(value), PROP_L("value"), false, &tmp_v);
	if (!value) {
		return PHP_CBOR_ERROR_INTERNAL;
	}
	if (Z_TYPE_P(value) == IS_OBJECT && Z_OBJ_P(value)->ce == CBOR_CE(shareable)) {
		return PHP_CBOR_ERROR_TAG_VALUE;  /* nested Shareable */
	}
	return enc_zval(ctx, value);
}
