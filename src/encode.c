/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "codec.h"
#include "compatibility.h"
#include "di_encoder.h"
#include "tags.h"
#include "types.h"
#include "utf8.h"
#include "warn_muted.h"
#include <ext/date/php_date.h>
#include "warn_unmuted.h"
#include <Zend/zend_interfaces.h>
#include <Zend/zend_smart_str.h>
#include <assert.h>

#define CTX_TEXT_FLAG(ctx)  (((ctx)->args.flags & CBOR_TEXT) != 0)

#define MAKE_ZSTR(ls)  zend_string_init(ZEND_STRL(ls), false)

typedef struct {
	uint32_t next_index;
	HashTable *str_table[2];
} srns_item;

enum {
	EXT_STR_GMP_CN = 0,
	EXT_STR_DEC_MN,
	EXT_STR_DEC_CN,
	EXT_STR_URI_IN,

	EXT_STR_ENC_SERIALIZE_FN,
	EXT_STR_DATE_FORMAT_FN,
	EXT_STR_DATE_FORMAT,
	EXT_STR_GMP_CMP_FN,
	EXT_STR_GMP_COM_FN,
	EXT_STR_GMP_EXPORT_FN,
	EXT_STR_DEC_ISNAN_FN,
	EXT_STR_DEC_ISINF_FN,
	EXT_STR_DEC_ISNEG_FN,
	EXT_STR_DEC_TOSTR_FN,

#if TARGET_PHP_API_LT_82
	EXT_STR_COUNT,
#endif

	_EXT_STR_COUNT,
};

typedef struct {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
} enc_ctx_call_info;

typedef struct {
	cbor_encode_args args;
	uint32_t cur_depth;
	smart_str *buf;
	srns_item *srns; /* string ref namespace */
	HashTable *refs, *ref_lock; /* shared ref, lock is actually not needed fow now */
	struct enc_ctx_ce {
		zend_class_entry *date_i;
		zend_class_entry *gmp;
		zend_class_entry *decimal;
		zend_class_entry *uri_i;
	} ce;
	struct enc_ctx_call {
		enc_ctx_call_info count;
	} call;
	zend_string *str[_EXT_STR_COUNT];
} enc_context;

typedef enum {
	HASH_ARRAY = 0,
	HASH_OBJ,
	HASH_STD_CLASS,
} hash_type;

static cbor_error enc_zval(enc_context *ctx, zval *value);
static void enc_long(enc_context *ctx, zend_long value);
static void enc_z_double(enc_context *ctx, zval *value, bool is_small);
static cbor_error enc_string(enc_context *ctx, zend_string *value, bool to_text);
static cbor_error enc_string_len(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text);
static cbor_error enc_hash(enc_context *ctx, zval *value, hash_type type);
static cbor_error enc_typed_byte(enc_context *ctx, zval *ins);
static cbor_error enc_typed_text(enc_context *ctx, zval *ins);
static void enc_typed_floatx(enc_context *ctx, zval *ins, int bits);
static cbor_error enc_tag(enc_context *ctx, zval *ins);
static void enc_tag_bare(enc_context *ctx, zend_long tag_id);
static cbor_error enc_serializable(enc_context *ctx, zval *value);
static cbor_error enc_traversable(enc_context *ctx, zval *value);
static cbor_error enc_encodeparams(enc_context *ctx, zval *ins);

static void init_srns_stack(enc_context *ctx);
static void free_srns_stack(enc_context *ctx);
static cbor_error enc_string_ref(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text);
static cbor_error enc_ref_counted(enc_context *ctx, zval *value);
static cbor_error enc_shareable(enc_context *ctx, zval *value);
static cbor_error enc_datetime(enc_context *ctx, zval *value);
static cbor_error enc_bignum(enc_context *ctx, zval *value);
static cbor_error enc_decimal(enc_context *ctx, zval *value);
static cbor_error enc_uri(enc_context *ctx, zval *value);

static zend_result call_fn(zval *object, zend_string *func_str, zval *retval_ptr, uint32_t param_count, zval params[]/*, HashTable *named_params*/);
static zend_string *decode_dec_str(const char *in_c, size_t in_len);
static zend_string *bin_int_sub1(zend_string *str);

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
#define ENC_CHECK_EXCEPTION()  do {\
		if (UNEXPECTED(EG(exception))) { \
			ENC_RESULT(CBOR_ERROR_EXCEPTION); \
		} \
	} while (0)

static cbor_error validate_flags(uint32_t flags)
{
	if (flags & CBOR_BYTE && flags & CBOR_TEXT) {
		return E_DESC(CBOR_ERROR_INVALID_FLAGS, BOTH_STRING_FLAG);
	}
	if (flags & CBOR_KEY_BYTE && flags & CBOR_KEY_TEXT) {
		return E_DESC(CBOR_ERROR_INVALID_FLAGS, BOTH_KEY_STRING_FLAG);
	}
	return 0;
}

cbor_error php_cbor_encode(zval *value, zend_string **data, cbor_encode_args *args)
{
	cbor_error error;
	enc_context ctx;
	smart_str buf = {0};
	memset(&ctx, 0, sizeof ctx);
	assert(IS_UNDEF == 0);
	ctx.args = *args;
	ctx.cur_depth = 0;
	ctx.buf = &buf;
	if ((error = validate_flags(ctx.args.flags)) != 0) {
		return error;
	}
	if (ctx.args.flags & CBOR_SELF_DESCRIBE) {
		enc_tag_bare(&ctx, CBOR_TAG_SELF_DESCRIBE);
	}
	if (ctx.args.string_ref == OPT_TRUE) {
		enc_tag_bare(&ctx, CBOR_TAG_STRING_REF_NS);
		init_srns_stack(&ctx);
	}
	ctx.refs = zend_new_array(0);
	ctx.ref_lock = zend_new_array(0);
	error = enc_zval(&ctx, value);
	free_srns_stack(&ctx);
	if (ctx.refs) {
		zend_array_destroy(ctx.refs);
	}
	if (ctx.ref_lock) {
		zend_array_destroy(ctx.ref_lock);
	}
	if (!Z_ISUNDEF(ctx.call.count.fci.function_name)) {
		/* release, if allocated string */
	}
	for (int i = 0; i < _EXT_STR_COUNT; i++) {
		if (ctx.str[i]) {
			zend_string_release(ctx.str[i]);
		}
	}
	if (!error) {
		*data = smart_str_extract(&buf);
	} else {
		args->error_args = ctx.args.error_args;
		smart_str_free(&buf);
	}
	return error;
}

static cbor_error enc_zval(enc_context *ctx, zval *value)
{
	cbor_error error = 0;
	if (ctx->cur_depth++ > ctx->args.max_depth) {
		return CBOR_ERROR_DEPTH;
	}
RETRY:
	switch (Z_TYPE_P(value)) {
	case IS_NULL:
		cbor_di_write_null(ctx->buf);
		break;
	case IS_FALSE:
		cbor_di_write_bool(ctx->buf, false);
		break;
	case IS_TRUE:
		cbor_di_write_bool(ctx->buf, true);
		break;
	case IS_LONG:
		enc_long(ctx, Z_LVAL_P(value));
		break;
	case IS_DOUBLE:
		enc_z_double(ctx, value, false);
		break;
	case IS_STRING:
		if (UNEXPECTED(!(ctx->args.flags & (CBOR_BYTE | CBOR_TEXT)))) {
			error = E_DESC(CBOR_ERROR_INVALID_FLAGS, NO_STRING_FLAG);
			break;
		}
		ENC_RESULT(enc_string(ctx, Z_STR_P(value), CTX_TEXT_FLAG(ctx)));
	case IS_ARRAY:
		ENC_RESULT(enc_hash(ctx, value, HASH_ARRAY));
	case IS_OBJECT: {
		zend_class_entry *ce = Z_OBJCE_P(value);
		/* Cbor types are 'final' */
		if (ce == CBOR_CE(undefined)) {
			cbor_di_write_undef(ctx->buf);
		} else if (ce == CBOR_CE(byte)) {
			error = enc_typed_byte(ctx, value);
		} else if (ce == CBOR_CE(text)) {
			error = enc_typed_text(ctx, value);
		} else if (ce == CBOR_CE(float16)) {
			enc_typed_floatx(ctx, value, 16);
		} else if (ce == CBOR_CE(float32)) {
			enc_typed_floatx(ctx, value, 32);
		} else if (ce == CBOR_CE(tag)) {
			error = enc_tag(ctx, value);
		} else if (ce == CBOR_CE(shareable)) {
			error = enc_shareable(ctx, value);
		} else if (ce == zend_standard_class_def) {
			if (ctx->args.shared_ref && Z_REFCOUNT_P(value) > 1) {
				error = enc_ref_counted(ctx, value);
				if (error != CBOR_STATUS_VALUE_FOLLOWS) {
					ENC_RESULT(error);
				}
			}
			error = enc_hash(ctx, value, HASH_STD_CLASS);
		} else if (ce == CBOR_CE(encodeparams)) {
			error = enc_encodeparams(ctx, value);
		} else if (instanceof_function(ce, CBOR_CE(serializable))) {
			error = enc_serializable(ctx, value);
		} else if (instanceof_function(ce, zend_ce_traversable)) {
			error = enc_traversable(ctx, value);
		} else {
			if (!ctx->ce.date_i) {
				ctx->ce.date_i = php_date_get_interface_ce();  /* in core */
				ctx->str[EXT_STR_GMP_CN] = MAKE_ZSTR("gmp");
				if (zend_hash_exists(&module_registry, ctx->str[EXT_STR_GMP_CN])) {
					ctx->ce.gmp = zend_lookup_class_ex(ctx->str[EXT_STR_GMP_CN], ctx->str[EXT_STR_GMP_CN], ZEND_FETCH_CLASS_NO_AUTOLOAD);
				}
				ctx->str[EXT_STR_DEC_MN] = MAKE_ZSTR("decimal");
				ctx->str[EXT_STR_DEC_CN] = MAKE_ZSTR("decimal\\decimal");
				if (zend_hash_exists(&module_registry, ctx->str[EXT_STR_DEC_MN])) {
					ctx->ce.decimal = zend_lookup_class_ex(ctx->str[EXT_STR_DEC_CN], ctx->str[EXT_STR_DEC_CN], ZEND_FETCH_CLASS_NO_AUTOLOAD);
				}
				ctx->str[EXT_STR_URI_IN] = MAKE_ZSTR("psr\\http\\message\\uriinterface");
				ctx->ce.uri_i = zend_lookup_class_ex(ctx->str[EXT_STR_URI_IN], ctx->str[EXT_STR_URI_IN], ZEND_FETCH_CLASS_NO_AUTOLOAD);
			}
			if (ctx->args.datetime && instanceof_function(ce, ctx->ce.date_i)) {
				error = enc_datetime(ctx, value);
			} else if (ctx->args.bignum && ce == ctx->ce.gmp) {
				error = enc_bignum(ctx, value);
			} else if (ctx->args.decimal && ce == ctx->ce.decimal) {
				error = enc_decimal(ctx, value);
			} else if (ctx->args.uri && ctx->ce.uri_i && instanceof_function(ce, ctx->ce.uri_i)) {
				error = enc_uri(ctx, value);
			} else {
				error = CBOR_ERROR_UNSUPPORTED_TYPE;
				ctx->args.error_args.u.ce_name = ce->name;
			}
		}
		break;
	}
	case IS_REFERENCE:
		if (ctx->args.shared_ref == OPT_UNSAFE_REF && Z_TYPE_P(Z_REFVAL_P(value)) != IS_OBJECT) {
			/* no CBOR representation of &object is defined as of writing (double-sharedref or indirection(22098) is not what it means) */
			/* PHP reference actually shares container of the value, not the value it contains after all */
			error = enc_ref_counted(ctx, value);
			if (error != CBOR_STATUS_VALUE_FOLLOWS) {
				ENC_RESULT(error);
			}
		}
		ZVAL_DEREF(value);
		goto RETRY;
	default:
		error = CBOR_ERROR_UNSUPPORTED_TYPE;
	}
ENCODED:
	ctx->cur_depth--;
	return error;
}

static void enc_long(enc_context *ctx, zend_long value)
{
	if (value >= 0) {
		cbor_di_write_int(ctx->buf, DI_UINT, value);
	} else {
		cbor_di_write_int(ctx->buf, DI_NINT, -(value + 1));
	}
}

static void enc_z_double(enc_context *ctx, zval *value, bool is_small)
{
	int float_type = ctx->args.flags & (CBOR_FLOAT16 | CBOR_FLOAT32);
	if (float_type == (CBOR_FLOAT16 | CBOR_FLOAT32)) {
		/* Test if smaller type will fit. (But not to denormalized numbers.) */
		binary64_alias bin64;
		bin64.f = Z_DVAL_P(value);
		unsigned exp = (bin64.i >> 52) & 0x7ff;  /* exp */
		uint64_t frac = bin64.i & 0xfffffffffffff;  /* fraction */
		bool is_inf_nan = exp == 0x7ff;
		if (bin64.i == 0) { /* 0.0 */
			float_type = CBOR_FLOAT16;
		} else if ((is_inf_nan || (exp >= 1023 - 126 && exp <= 1023 + 127)) && !(frac & 0x1fffffff)) {  /* fraction: 52 to 23 */
			if ((is_inf_nan || (exp >= 1023 - 14 && exp <= 1023 + 15)) && !(frac & 0x3ffffffffff)) {  /* fraction: 52 to 10 */
				float_type = CBOR_FLOAT16;
			} else {
				float_type = CBOR_FLOAT32;
			}
		}
	}
	if (float_type == CBOR_FLOAT16) {
		/* XXX: Reusing float-to-half. For precise rounding, double may have to be used. */
		uint16_t binary16 = cbor_to_float16((float)Z_DVAL_P(value));
		cbor_di_write_float16(ctx->buf, binary16);
	} else if (float_type == CBOR_FLOAT32) {
		cbor_di_write_float32(ctx->buf, (float)Z_DVAL_P(value));
	} else {
		cbor_di_write_float64(ctx->buf, Z_DVAL_P(value));
	}
}

static void enc_xint(enc_context *ctx, uint64_t value, bool is_negative)
{
	cbor_di_write_int(ctx->buf, is_negative ? DI_NINT : DI_UINT, value);
}

static void enc_bin_int(enc_context *ctx, const char *str, size_t length, bool is_negative)
{
	uint64_t i_value = 0;
	assert(length <= 8);
	for (size_t i = 0; i < length; i++) {
		i_value <<= 8;
		i_value |= (uint8_t)str[i];
	}
	enc_xint(ctx, i_value, is_negative);
}

static cbor_error enc_string(enc_context *ctx, zend_string *value, bool to_text)
{
	return enc_string_len(ctx, ZSTR_VAL(value), ZSTR_LEN(value), value, to_text);
}

static cbor_error enc_string_len(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text)
{
	cbor_error error;
	if (ctx->srns) {
		error = enc_string_ref(ctx, value, length, v_str, to_text);
		if (error != CBOR_STATUS_VALUE_FOLLOWS) {
			return error;
		}
	}
	if (to_text) {
		if (!(ctx->args.flags & CBOR_UNSAFE_TEXT)
				&& !is_utf8((uint8_t *)value, length)) {
			return CBOR_ERROR_UTF8;
		}
	}
	cbor_di_write_int(ctx->buf, to_text ? DI_TSTR : DI_BSTR, length);
	if (length) {
		char *ptr = smart_str_extend(ctx->buf, length);
		memcpy(ptr, value, length);
	}
	return 0;
}

static bool convert_string_to_int(zend_string *str, uint64_t *value, bool *is_negative)
{
	char buf[24];
	char *ptr, *head_ptr;
	head_ptr = ptr = ZSTR_VAL(str);
	if (*head_ptr < '-' || *head_ptr > '9') {  /* "", etc. */
		assert('-' < '0' && '0' < '9');
		return false;
	}
	size_t len = ZSTR_LEN(str);
	if (len > 1 + 20) {  /* sign = 1, ceil(log10(2)*64) = 20 */
		return false;
	}
	if (*head_ptr == '0') {
		*value = 0;
		*is_negative = false;
		return len == 1;  /* "0" or error */
	}
	bool negative_fix = false;
	bool is_minus = *head_ptr == '-';
	if (is_minus) {
		if ((negative_fix = head_ptr[len - 1] == '6') != 0) {
			assert(len + 1 < sizeof buf);
			memcpy(buf, head_ptr, len + 1);
			head_ptr = ptr = buf;
			buf[len - 1] = '5';  /* (uint64_t)~0, "-18446744073709551616" => "...5" */
		}
		ptr++;
		if (*ptr == '-' || *ptr == '0' || *ptr == '\0') {  /* "--...", "-0...", "-" */
			return false;
		}
	}
	/* signed decimal string to uint64_t */
	uint64_t dec_value = 0;
	for (int i = 0;; i++) {
		int ch_dec = (uint8_t)*ptr - '0';
		if (ch_dec < 0 || ch_dec > 9
				|| (i > 18 && dec_value > (UINT64_C(0xffffffffffffffff) - ch_dec) / 10)) {
			break;
		}
		dec_value = dec_value * 10 + ch_dec;
		ptr++;
	}
	if (ptr != head_ptr + len) {
		return false;
	}
	if (is_minus && !negative_fix) {
		dec_value--;
	}
	*value = dec_value;
	*is_negative = is_minus;
	return true;
}

static cbor_error enc_hash(enc_context *ctx, zval *value, hash_type type)
{
	cbor_error error = 0;
	HashTable *ht = NULL;
	zend_ulong count;
	bool to_text = (ctx->args.flags & CBOR_KEY_TEXT) != 0;
	bool key_flag_error = !(ctx->args.flags & (CBOR_KEY_BYTE | CBOR_KEY_TEXT));
	bool is_list;
	bool is_indef_length = false;
	bool use_int_key = ctx->args.flags & CBOR_INT_KEY;
	if (Z_IS_RECURSIVE_P(value)) {
		return CBOR_ERROR_RECURSION;
	}
	if (type == HASH_ARRAY) {
		ht = Z_ARR_P(value);
	} else {
		ht = zend_get_properties_for(value, ZEND_PROP_PURPOSE_JSON);
	}
	is_list = type == HASH_ARRAY && zend_array_is_list(ht);
	count = zend_hash_num_elements(ht);
	if ((type == HASH_OBJ && count)
			|| UNEXPECTED(count > 0xff && (ctx->args.flags & CBOR_PACKED))) {
		is_indef_length = true;
		cbor_di_write_indef(ctx->buf, is_list ? DI_ARRAY : DI_MAP);
	} else {
		cbor_di_write_int(ctx->buf, is_list ? DI_ARRAY : DI_MAP, count);
	}
	if (count) {
		zend_ulong index;
		zend_string *key;
		zval *data;
		Z_PROTECT_RECURSION_P(value);
		ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
			if (!is_list) {
				if (key) {
					uint64_t key_int;
					bool is_negative;
					/* check property visibility if it is object and not stdClass */
					if (type == HASH_OBJ && *ZSTR_VAL(key) == '\0' && ZSTR_LEN(key) > 0) {
						continue; /* skip if not a public property */
					}
					if (use_int_key && convert_string_to_int(key, &key_int, &is_negative)) {
						enc_xint(ctx, key_int, is_negative);
						error = 0;
					} else if (UNEXPECTED(key_flag_error)) {
						error = E_DESC(CBOR_ERROR_INVALID_FLAGS, NO_KEY_STRING_FLAG);
					} else {
						error = enc_string(ctx, key, to_text);
					}
				} else if (!use_int_key) {
					char num_str[ZEND_LTOA_BUF_LEN];
					ZEND_LTOA((zend_long)index, num_str, sizeof num_str);
					if (UNEXPECTED(key_flag_error)) {
						error = E_DESC(CBOR_ERROR_INVALID_FLAGS, NO_KEY_STRING_FLAG);
					} else {
						error = enc_string_len(ctx, num_str, strlen(num_str), NULL, to_text);
					}
				} else {
					enc_long(ctx, (zend_long)index);
					error = 0;
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
			error = CBOR_ERROR_INTERNAL;
		}
		Z_UNPROTECT_RECURSION_P(value);
	}
	if (type != HASH_ARRAY) {
		zend_release_properties(ht);
	}
	if (is_indef_length) {
		cbor_di_write_break(ctx->buf);
	}
	return error;
}

#define PROP_L(prop_literal) prop_literal, sizeof prop_literal - 1

static cbor_error enc_typed_byte(enc_context *ctx, zval *ins)
{
	zend_string *str = cbor_get_xstring_value(ins);
	return enc_string(ctx, str, false);
}

static cbor_error enc_typed_text(enc_context *ctx, zval *ins)
{
	zend_string *str = cbor_get_xstring_value(ins);
	return enc_string(ctx, str, true);
}

static void enc_typed_floatx(enc_context *ctx, zval *ins, int bits)
{
	assert(Z_TYPE_P(ins) == IS_OBJECT);
	uint8_t *ptr = cbor_di_write_float_head(ctx->buf, bits);
	cbor_floatx_get_value(Z_OBJ_P(ins), (char *)ptr);
}

static void enc_tag_bare(enc_context *ctx, zend_long tag_id)
{
	/* XXX: tag above ZEND_LONG_MAX cannot be created. As of writing there is one valid tag like this. */
	assert(tag_id >= 0);
	cbor_di_write_int(ctx->buf, DI_TAG, tag_id);
}

static cbor_error enc_tag(enc_context *ctx, zval *ins)
{
	cbor_error error = 0;
	zval tmp_tag, tmp_content;
	zval *tag, *content;
	zend_long tag_id;
	srns_item *orig_srns = NULL;
	bool new_srns = false;
	tag = zend_read_property(CBOR_CE(tag), Z_OBJ_P(ins), PROP_L("tag"), false, &tmp_tag);
	content = zend_read_property(CBOR_CE(tag), Z_OBJ_P(ins), PROP_L("content"), false, &tmp_content);
	if (!tag || !content) {
		return CBOR_ERROR_INTERNAL;
	}
	tag_id = Z_LVAL_P(tag);
	if (tag_id < 0) {
		return CBOR_ERROR_SYNTAX;
	}
	enc_tag_bare(ctx, tag_id);
	if (tag_id == CBOR_TAG_STRING_REF_NS && ctx->args.string_ref) {
		new_srns = true;
		orig_srns = ctx->srns;
		init_srns_stack(ctx);
	} else if (tag_id == CBOR_TAG_STRING_REF && ctx->args.string_ref) {
		zend_long tag_content;
		if (Z_TYPE_P(content) != IS_LONG) {
			return CBOR_ERROR_TAG_TYPE;
		}
		if (!ctx->srns) {
			return CBOR_ERROR_TAG_SYNTAX;
		}
		tag_content = Z_LVAL_P(content);
		if (tag_content < 0 || (zend_ulong)tag_content >= ctx->srns->next_index) {
			return CBOR_ERROR_TAG_VALUE;
		}
	}
	error = enc_zval(ctx, content);
	if (new_srns) {
		free_srns_stack(ctx);
		ctx->srns = orig_srns;
	}
	return error;
}

static cbor_error enc_serializable(enc_context *ctx, zval *value)
{
	cbor_error error;
	zval r_value;
	if (Z_IS_RECURSIVE_P(value)) {
		return CBOR_ERROR_RECURSION;
	}
	if (!ctx->str[EXT_STR_ENC_SERIALIZE_FN]) {
		ctx->str[EXT_STR_ENC_SERIALIZE_FN] = MAKE_ZSTR("cborserialize");
	}
	if (call_fn(value, ctx->str[EXT_STR_ENC_SERIALIZE_FN], &r_value, 0, NULL) != SUCCESS) {
		return CBOR_ERROR_INTERNAL;
	}
	if (UNEXPECTED(EG(exception))) {
		return CBOR_ERROR_EXCEPTION;
	}
	Z_PROTECT_RECURSION_P(value);
	error = enc_zval(ctx, &r_value);
	Z_UNPROTECT_RECURSION_P(value);
	zval_ptr_dtor(&r_value);
	return error;
}

static cbor_error enc_traversable(enc_context *ctx, zval *value)
{
	if (Z_IS_RECURSIVE_P(value)) {
		return CBOR_ERROR_RECURSION;
	}
	Z_PROTECT_RECURSION_P(value);
	cbor_error error = 0;
	zend_object_iterator *it = NULL;
	bool is_indef_length = !zend_is_countable(value);
	zend_long count;
	if (is_indef_length) {
		cbor_di_write_indef(ctx->buf, DI_MAP);
		count = -1;
	} else {
		if (Z_ISUNDEF(ctx->call.count.fci.function_name)) {
			zval z_count_str;
#if TARGET_PHP_API_LT_82
			if (!ctx->str[EXT_STR_COUNT]) {
				ctx->str[EXT_STR_COUNT] = MAKE_ZSTR("count");
			}
			ZVAL_STR(&z_count_str, ctx->str[EXT_STR_COUNT]);
#else
			ZVAL_STR(&z_count_str, ZSTR_KNOWN(ZEND_STR_COUNT));
#endif
			if (zend_fcall_info_init(&z_count_str, 0, &ctx->call.count.fci, &ctx->call.count.fcc, NULL, NULL) != SUCCESS) {
				ENC_RESULT(EG(exception) ? CBOR_ERROR_EXCEPTION : CBOR_ERROR_INTERNAL);
			}
		}
		zend_fcall_info *fci = &ctx->call.count.fci;
		zval z_count;
		fci->retval = &z_count;
		fci->params = value;
		fci->param_count = 1;
		zend_result call_result = zend_call_function(fci, &ctx->call.count.fcc);
		zval_ptr_dtor(&z_count); /* Can free safely before accessing as IS_LONG is the only valid type */
		if (UNEXPECTED(call_result != SUCCESS || EG(exception) || Z_TYPE(z_count) != IS_LONG || Z_LVAL(z_count) < 0)) {
			ENC_CHECK(EG(exception) ? CBOR_ERROR_EXCEPTION : CBOR_ERROR_INTERNAL);
		}
		count = Z_LVAL(z_count);
		cbor_di_write_int(ctx->buf, DI_MAP, count);
	}
	zend_object *obj = Z_OBJ_P(value);
	zend_class_entry *ce = obj->ce;
	it = ce->get_iterator(ce, value, 0);
	ENC_CHECK_EXCEPTION();
	(*it->funcs->rewind)(it);
	ENC_CHECK_EXCEPTION();
	zend_long index = 0;
	while ((*it->funcs->valid)(it) == SUCCESS) {
		ENC_CHECK_EXCEPTION();
		if (!is_indef_length) {
			if (index == count) {
				ENC_RESULT(CBOR_ERROR_EXTRANEOUS_DATA);
			}
		}
		zval key;
		if (it->funcs->get_current_key) {
			(*it->funcs->get_current_key)(it, &key);
			ENC_CHECK_EXCEPTION();
		} else { /* fallback to 0-based index */
			ZVAL_LONG(&key, index);
		}
		index++;
		ENC_CHECK(enc_zval(ctx, &key));
		zval_ptr_dtor(&key);
		zval *current = (*it->funcs->get_current_data)(it);
		ENC_CHECK_EXCEPTION();
		/* CBOR_INT_KEY is not applied here. For traversable keys are not coerced, users can cast freely */
		ENC_CHECK(enc_zval(ctx, current));
		zval_ptr_dtor(current);
		(*it->funcs->move_forward)(it);
		ENC_CHECK_EXCEPTION();
	}
	ENC_CHECK_EXCEPTION();
	if (is_indef_length) {
		cbor_di_write_break(ctx->buf);
	} else if (index != count) {
		ENC_RESULT(CBOR_ERROR_TRUNCATED_DATA);
	}
ENCODED:
	if (it) {
		zend_iterator_dtor(it);
	}
	Z_UNPROTECT_RECURSION_P(value);
	return error;
}

static cbor_error enc_encodeparams(enc_context *ctx, zval *ins)
{
	cbor_error error;
	cbor_encode_args saved_args;
	zval tmp_value, tmp_params;
	zval *value, *params, *conf;
	zend_long flags;
	HashTable *ht;
	saved_args = ctx->args;
	if (Z_IS_RECURSIVE_P(ins)) {
		ENC_RESULT(CBOR_ERROR_RECURSION);
	}
	value = zend_read_property_ex(CBOR_CE(encodeparams), Z_OBJ_P(ins), ZSTR_KNOWN(ZEND_STR_VALUE), false, &tmp_value);
	params = zend_read_property(CBOR_CE(encodeparams), Z_OBJ_P(ins), PROP_L("params"), false, &tmp_params);
	if (!value || !params) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	ht = Z_ARR_P(params);
	conf = zend_hash_str_find_deref(ht, ZEND_STRL("flags_clear"));
	if (conf) {
		if (Z_TYPE_P(conf) != IS_LONG) {
			ENC_RESULT(CBOR_ERROR_INVALID_FLAGS);
		}
		flags = Z_LVAL_P(conf);
		ctx->args.flags &= ~flags;
	}
	conf = zend_hash_str_find_deref(ht, ZEND_STRL("flags"));
	if (conf) {
		uint32_t mutex_flags = 0;
		if (Z_TYPE_P(conf) != IS_LONG) {
			ENC_RESULT(CBOR_ERROR_INVALID_FLAGS);
		}
		flags = Z_LVAL_P(conf);
		if (flags & CBOR_BYTE) {
			mutex_flags = CBOR_TEXT;
		} else if (flags & CBOR_TEXT) {
			mutex_flags = CBOR_BYTE;
		}
		if (flags & CBOR_KEY_BYTE) {
			mutex_flags |= CBOR_KEY_TEXT;
		} else if (flags & CBOR_KEY_TEXT) {
			mutex_flags |= CBOR_KEY_BYTE;
		}
		ctx->args.flags &= ~mutex_flags;
		ctx->args.flags |= flags;
	}
	ENC_CHECK(validate_flags(ctx->args.flags));
	ENC_CHECK(php_cbor_override_encode_options(&ctx->args, ht));
	Z_PROTECT_RECURSION_P(ins);
	error = enc_zval(ctx, value);
	Z_UNPROTECT_RECURSION_P(ins);
ENCODED:
	ctx->args = saved_args;
	return error;
}

static void init_srns_stack(enc_context *ctx)
{
	srns_item *srns = (srns_item *)emalloc(sizeof *srns);
	ctx->srns = srns;
	srns->next_index = 0;
	srns->str_table[0] = zend_new_array(0);
	srns->str_table[1] = zend_new_array(0);
}

static void free_srns_stack(enc_context *ctx)
{
	srns_item *srns = ctx->srns;
	if (srns) {
		zend_array_release(srns->str_table[0]);
		zend_array_release(srns->str_table[1]);
		efree(srns);
	}
}

static cbor_error enc_string_ref(enc_context *ctx, const char *value, size_t length, zend_string *v_str, bool to_text)
{
	cbor_error error = 0;
	srns_item *srns = ctx->srns;
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
		enc_tag_bare(ctx, CBOR_TAG_STRING_REF);
		enc_long(ctx, Z_LVAL_P(str_index));
		goto ENCODED;
	}
	if (!cbor_is_len_string_ref(length, srns->next_index)) {
		ENC_RESULT(CBOR_STATUS_VALUE_FOLLOWS);
	}
	if (!(~srns->next_index)) {  /* until max - 1 for simplicity */
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	ZVAL_LONG(&new_index, srns->next_index);
	srns->next_index++;
	if (!zend_hash_add_new(str_table, v_str, &new_index)) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	error = CBOR_STATUS_VALUE_FOLLOWS;
ENCODED:
	if (created) {
		zend_string_release(v_str);
	}
	return error;
}

static cbor_error enc_ref_counted(enc_context *ctx, zval *value)
{
#if ZEND_ULONG_MAX < UINTPTR_MAX
#error "Unimplemented: maybe use pointer as a binary string key?"
#endif
	zend_ulong key_index = (zend_ulong)Z_COUNTED_P(value);
	zval new_index, *ref_index;
	assert(Z_REFCOUNTED_P(value));
	if ((ref_index = zend_hash_index_find(ctx->refs, key_index)) != NULL) {
		enc_tag_bare(ctx, CBOR_TAG_SHARED_REF);
		enc_long(ctx, Z_LVAL_P(ref_index));
		return 0;
	}
	ZVAL_LONG(&new_index, zend_hash_num_elements(ctx->refs));
	if (!zend_hash_index_add_new(ctx->refs, key_index, &new_index)
			|| !zend_hash_next_index_insert_new(ctx->ref_lock, value)) {
		return CBOR_ERROR_INTERNAL;
	}
	Z_TRY_ADDREF_P(value);
	enc_tag_bare(ctx, CBOR_TAG_SHAREABLE);
	return CBOR_STATUS_VALUE_FOLLOWS;
}

static cbor_error enc_shareable(enc_context *ctx, zval *value)
{
	cbor_error error = 0;
	zval tmp_v;
	error = enc_ref_counted(ctx, value);
	if (error != CBOR_STATUS_VALUE_FOLLOWS) {
		return error;
	}
	value = zend_read_property_ex(CBOR_CE(shareable), Z_OBJ_P(value), ZSTR_KNOWN(ZEND_STR_VALUE), false, &tmp_v);
	if (!value) {
		return CBOR_ERROR_INTERNAL;
	}
	if (Z_TYPE_P(value) == IS_OBJECT && Z_OBJ_P(value)->ce == CBOR_CE(shareable)) {
		return CBOR_ERROR_TAG_VALUE;  /* nested Shareable */
	}
	return enc_zval(ctx, value);
}

static zend_result call_fn(zval *object, zend_string *func_str, zval *retval_ptr, uint32_t param_count, zval params[]/*, HashTable *named_params*/)
{
	zval func_name;
	ZVAL_NEW_STR(&func_name, func_str);
	return call_user_function(NULL, object, &func_name, retval_ptr, param_count, params);
}

static cbor_error enc_datetime(enc_context *ctx, zval *value)
{
	cbor_error error;
	zval r_value;
	zval params[1];
	zend_string *r_str;
	size_t i, len;
	if (!ctx->str[EXT_STR_DATE_FORMAT_FN]) {
		ctx->str[EXT_STR_DATE_FORMAT_FN] = MAKE_ZSTR("format");
		ctx->str[EXT_STR_DATE_FORMAT] = MAKE_ZSTR("Y-m-d\\TH:i:s.uP");
	}
	ZVAL_NEW_STR(&params[0], ctx->str[EXT_STR_DATE_FORMAT]);
	if (call_fn(value, ctx->str[EXT_STR_DATE_FORMAT_FN], &r_value, 1, params) != SUCCESS) {
		return CBOR_ERROR_INTERNAL;
	}
	r_str = Z_STR(r_value);
	/* 0         1         2         3   */
	/* 012345678901234567890123456789012 */
	/* 2001-02-03T04:05:06.000007+08:09  */
	len = 32;
	if (ZSTR_LEN(r_str) != len) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	r_str = zend_string_separate(r_str, false);
	/* remove redundant fractional zeros */
	for (i = 25; ; i--) {
		assert(i >= 19);
		if (ZSTR_VAL(r_str)[i] != '0') {
			if (ZSTR_VAL(r_str)[i] != '.') {
				i++;
			}
			break;
		}
	}
	if (i < 26) {
		memmove(&ZSTR_VAL(r_str)[i], &ZSTR_VAL(r_str)[26], len - 26);
		len -= 26 - i;
	}
	if (memcmp(&ZSTR_VAL(r_str)[len - 6], "+00:00", 6) == 0) {
		/* convert +00:00 to Z */
		len -= 5;
		ZSTR_VAL(r_str)[len - 1] = 'Z';
	}
	if (len != 32) {
		ZSTR_VAL(r_str)[len] = '\0';
		r_str = zend_string_truncate(r_str, len, false);
	}
	enc_tag_bare(ctx, CBOR_TAG_DATETIME);
	error = enc_string(ctx, r_str, true);
ENCODED:
	zend_string_release(r_str);
	return error;
}

static cbor_error enc_bignum(enc_context *ctx, zval *value)
{
	cbor_error error = 0;
	zval r_value, com_value;
	zval params[2];
	zend_string *r_str = NULL;
	size_t len;
	bool is_negative;
	ZVAL_UNDEF(&r_value);
	ZVAL_UNDEF(&com_value);
	if (!ctx->str[EXT_STR_GMP_EXPORT_FN]) {
		ctx->str[EXT_STR_GMP_CMP_FN] = MAKE_ZSTR("gmp_cmp");
		ctx->str[EXT_STR_GMP_COM_FN] = MAKE_ZSTR("gmp_com");
		ctx->str[EXT_STR_GMP_EXPORT_FN] = MAKE_ZSTR("gmp_export");
	}
	ZVAL_COPY_VALUE(&params[0], value);
	ZVAL_LONG(&params[1], 0);
	if (call_fn(NULL, ctx->str[EXT_STR_GMP_CMP_FN], &r_value, 2, params) != SUCCESS) {
		return CBOR_ERROR_INTERNAL;
	}
	is_negative = Z_LVAL(r_value) < 0;
	if (is_negative) {
		if (call_fn(NULL, ctx->str[EXT_STR_GMP_COM_FN], &com_value, 1, params) != SUCCESS) {
			return CBOR_ERROR_INTERNAL;
		}
		value = &com_value;
		ZVAL_COPY_VALUE(&params[0], value);
	}
	if (call_fn(NULL, ctx->str[EXT_STR_GMP_EXPORT_FN], &r_value, 1, params) != SUCCESS) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	r_str = Z_STR(r_value);
	len = ZSTR_LEN(r_str);
	if (len <= 8) {
		enc_bin_int(ctx, ZSTR_VAL(r_str), len, is_negative);
		goto ENCODED;
	}
	/* len > 8 */
	r_str = zend_string_separate(r_str, false);
	if (len == 1 && ZSTR_VAL(r_str)[0] == '0') {  /* or already empty */
		len = 0;
	}
	if (len != ZSTR_LEN(r_str)) {
		ZSTR_VAL(r_str)[len] = '\0';
		r_str = zend_string_truncate(r_str, len, false);
	}
	enc_tag_bare(ctx, is_negative ? CBOR_TAG_BIGNUM_N : CBOR_TAG_BIGNUM_U);
	error = enc_string(ctx, r_str, false);
ENCODED:
	zval_ptr_dtor(&com_value);
	if (r_str) {
		zend_string_release(r_str);
	}
	return error;
}

static cbor_error enc_decimal(enc_context *ctx, zval *value)
{
	cbor_error error = 0;
	zval r_value;
	zend_string *r_str = NULL;
	zend_string *bin_str = NULL;
	const char *r_ptr, *c_ptr;
	size_t len;
	bool is_negative;
	zend_long exp, exp_p;
	ZVAL_UNDEF(&r_value);
	if (!ctx->str[EXT_STR_DEC_TOSTR_FN]) {
		ctx->str[EXT_STR_DEC_ISNAN_FN] = MAKE_ZSTR("isnan");
		ctx->str[EXT_STR_DEC_ISINF_FN] = MAKE_ZSTR("isinf");
		ctx->str[EXT_STR_DEC_ISNEG_FN] = MAKE_ZSTR("isnegative");
		ctx->str[EXT_STR_DEC_TOSTR_FN] = MAKE_ZSTR("tostring");
	}
	if (call_fn(value, ctx->str[EXT_STR_DEC_ISNAN_FN], &r_value, 0, NULL) != SUCCESS) {
		return CBOR_ERROR_INTERNAL;
	}
	if (Z_TYPE(r_value) == IS_TRUE) {
		zval *nan = zend_get_constant_str(ZEND_STRL("NAN"));  /* use PHP's constant, not the extension's compiler's */
		enc_z_double(ctx, nan, true);
		goto ENCODED;
	}
	if (call_fn(value, ctx->str[EXT_STR_DEC_ISINF_FN], &r_value, 0, NULL) != SUCCESS) {
		return CBOR_ERROR_INTERNAL;
	}
	if (Z_TYPE(r_value) == IS_TRUE) {
		if (call_fn(value, ctx->str[EXT_STR_DEC_ISNEG_FN], &r_value, 0, NULL) != SUCCESS) {
			return CBOR_ERROR_INTERNAL;
		}
		ZVAL_DOUBLE(&r_value, (Z_TYPE(r_value) == IS_TRUE) ? -INFINITY : INFINITY);
		enc_z_double(ctx, &r_value, true);
		goto ENCODED;
	}
	if (call_fn(value, ctx->str[EXT_STR_DEC_TOSTR_FN], &r_value, 0, NULL) != SUCCESS) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	/* output will be like '-123.45E+67' */
	r_str = Z_STR(r_value);
	len = ZSTR_LEN(r_str);
	if (!len) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	r_ptr = ZSTR_VAL(r_str);
	is_negative = *r_ptr == '-';
	if (is_negative) {
		r_ptr++;
		if (!--len) {
			ENC_RESULT(CBOR_ERROR_INTERNAL);
		}
	}
	exp = 0;
	c_ptr = strchr(r_ptr, 'E');
	if (c_ptr) {
		size_t exp_len = len - (c_ptr - r_ptr);
		/* Decimal extension appears not to support arbitraly length exponent. So is this. */
		if (exp_len - 1 >= MAX_LENGTH_OF_LONG) {  /* skip 'E', exclude max length to avoid overflow */
			ENC_RESULT(CBOR_ERROR_UNSUPPORTED_VALUE);
		}
		exp = atoi(c_ptr + 1);
		len -= exp_len;
	}
	bin_str = decode_dec_str(r_ptr, len);
	if (!bin_str) {
		ENC_RESULT(CBOR_ERROR_INTERNAL);
	}
	if (is_negative) {
		if (ZSTR_LEN(bin_str) == 0) {
			/* No mention of decimal negative zero in RFC 8949.
			 * We choose decimal without sign rather than -0f without precision. */
			is_negative = false;
		} else {
			bin_str = bin_int_sub1(bin_str);
		}
	}
	c_ptr = strchr(r_ptr, '.');
	exp_p = c_ptr ? -(int)(len - 1 - (c_ptr - r_ptr)) : 0;  /* to keep precision of Decimal instance, exp never be greater than zero */
	if ((exp > 0 && exp_p > ZEND_LONG_MAX - exp)
			|| (exp < 0 && exp_p < ZEND_LONG_MIN - exp)) {
		ENC_RESULT(CBOR_ERROR_UNSUPPORTED_VALUE);
	}
	exp += exp_p;
	len = ZSTR_LEN(bin_str);
	if (exp || len > 8) {
		enc_tag_bare(ctx, CBOR_TAG_DECIMAL);
		cbor_di_write_int(ctx->buf, DI_ARRAY, 2);
		enc_long(ctx, exp);
	}
	if (len <= 8) {
		enc_bin_int(ctx, ZSTR_VAL(bin_str), len, is_negative);
		goto ENCODED;
	}
	enc_tag_bare(ctx, is_negative ? CBOR_TAG_BIGNUM_N : CBOR_TAG_BIGNUM_U);
	error = enc_string(ctx, bin_str, false);
ENCODED:
	if (r_str) {
		zend_string_release(r_str);
	}
	if (bin_str) {
		zend_string_release(bin_str);
	}
	return error;
}

static cbor_error enc_uri(enc_context *ctx, zval *value)
{
	cbor_error error = 0;
	zend_object *obj;
	zval str;
	obj = Z_OBJ_P(value);
	if (obj->handlers->cast_object(obj, &str, IS_STRING) != SUCCESS) {
		assert(EG(exception));
		return CBOR_ERROR_EXCEPTION;
	}
	if (Z_TYPE(str) == IS_STRING) {
		enc_tag_bare(ctx, CBOR_TAG_URI);
		error = enc_string(ctx, Z_STR(str), true);
	} else {
		error = CBOR_ERROR_TAG_TYPE;
	}
	zval_ptr_dtor(&str);
	return error;
}

static int dec_char_to_int(uint8_t c)
{
	if (EXPECTED(c >= '0' && c <= '9')) {
		return c - '0';
	}
	return 255;
}

static zend_string *decode_dec_str(const char *in_c, size_t in_len)
{
	int out_max = (int)(415241 * in_len / 1000000 + 1);  /* floor(log256(10) * in_len) + 1 */
	bool out_c_heap, multi_c_heap;
	uint8_t *out_c = do_alloca_ex(out_max, 256, out_c_heap);
	uint8_t *multi_c = do_alloca_ex(out_max, 256, multi_c_heap);
	int multi_pos = out_max - 1, multi_max;
	int out_pos = out_max - 1;
	int carry, i;
	zend_string *out_str;
	out_str = NULL;
	if (UNEXPECTED(in_len > SIZE_MAX / 415241)) {
		goto BAIL;
	}
	multi_c[multi_pos] = 1;
	multi_max = multi_pos;
	memset(out_c, 0, out_max);
	out_c[out_pos] = 0;
	for (int in_pos = (int)in_len - 1; in_pos >= 0; in_pos--) {
		int n = dec_char_to_int(((uint8_t *)in_c)[in_pos]);
		if (n > 9) {
			continue;  /* skip non-digit */
		}
		if (n) {
			carry = 0;
			for (i = out_max - 1; i >= multi_pos; i--) {
				int result = out_c[i] + carry + multi_c[i] * n;
				out_c[i] = result & 0xff;
				carry = result >> 8;
			}
			for (; carry; i--) {
				if (i < 0) {
					goto BAIL;
				}
				int result = out_c[i] + carry;
				out_c[i] = result & 0xff;
				carry = result >> 8;
			}
		}
		carry = 0;
		for (i = multi_max; i >= multi_pos; i--) {
			int result = multi_c[i] * 10 + carry;
			multi_c[i] = result & 0xff;
			carry = result >> 8;
		}
		for (; carry; i--) {
			if (i < 0) {
				goto BAIL;
			}
			multi_c[i] = carry & 0xff;
			carry = carry >> 8;
			multi_pos--;
		}
		for (; multi_max >= multi_pos; multi_max--) {
			if (multi_c[multi_max]) {
				break;
			}
		}
	}
	for (i = 0; i < out_max && !out_c[i]; i++) {
		/* skip zeros */
	}
	out_str = zend_string_init((const char *)&out_c[i], out_max - i, false);
BAIL:
	free_alloca(out_c, out_c_heap);
	free_alloca(multi_c, multi_c_heap);
	return out_str;
}

static zend_string *bin_int_sub1(zend_string *str)
{
	uint8_t *in_c = (uint8_t *)ZSTR_VAL(str);
	size_t in_len = ZSTR_LEN(str);
	size_t i;
	assert(in_len);
	for (i = in_len; --i <= in_len; ) {
		in_c[i]--;
		if (in_c[i] != 0xff) {
			break;
		}
	}
	assert(i >= 0);  /* input must not be 0 */
	if (i == 0 && !in_c[0]) {
		memmove(&in_c[0], &in_c[1], in_len - 1);
		str = zend_string_realloc(str, in_len - 1, false);
	}
	return str;
}
