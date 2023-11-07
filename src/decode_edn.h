/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "compatibility.h"
#include <math.h>
#include <Zend/zend_strtod.h>

typedef struct stack_item_edn {
	stack_item base;
	bool indef_appended;
	uint8_t opt_indent;
	union edn_si_value_ {
		bool map_is_key;
	} v;
} stack_item_edn;

#define APPEND_CHAR(c)  smart_str_appendc(&ctx->u.edn.str, (c))
#define APPEND_LSTR(s)  smart_str_appendl_ex(&ctx->u.edn.str, ZEND_STRL(s), false)
#define APPEND_HEX(c)  do { \
		snprintf(hex_buf, sizeof hex_buf, "%02x", (c)); \
		smart_str_appendl_ex(&ctx->u.edn.str, hex_buf, 2, false); \
	} while (0)
#define APPEND_INDENT_NL()     edn_append_indent(ctx, true, false)
#define APPEND_INDENT_NL_SP()  edn_append_indent(ctx, true, true)
#define APPEND_DEINDENT_NL()   edn_append_indent(ctx, false, false)
#define APPEND_NL()     edn_append_nl(ctx, false)
#define APPEND_NL_SP()  edn_append_nl(ctx, true)
#define APPEND_SP()  edn_append_space(ctx)
#define IS_ASCII_CNTL(c)  ((c) < 0x20 || (c) == 0x7f) /* multi-eval */

static void edn_append_space(dec_context *ctx)
{
	if (ctx->args.edn.space) {
		APPEND_CHAR(' ');
	}
}

static void edn_append_nl(dec_context *ctx, bool space)
{
	if (!ctx->args.edn.indent_char) {
		if (space) {
			edn_append_space(ctx);
		}
		return;
	}
	APPEND_CHAR('\n');
	if (ctx->args.edn.indent) {
		int i = ctx->u.edn.indent_level;
		char *ptr = smart_str_extend(&ctx->u.edn.str, i);
		memset(ptr, ctx->args.edn.indent_char, i);
	}
}

static void edn_indent(dec_context *ctx, bool indent)
{
	if (ctx->args.edn.indent_char) {
		if (indent) {
			ctx->u.edn.indent_level += ctx->args.edn.indent;
		} else {
			ctx->u.edn.indent_level -= ctx->args.edn.indent;
		}
	}
}

static void edn_append_indent(dec_context *ctx, bool indent, bool space)
{
	edn_indent(ctx, indent);
	edn_append_nl(ctx, space);
}

static void edn_append_tstr_bstr(dec_context *ctx)
{
	APPEND_CHAR('"');
	APPEND_SP();
	APPEND_LSTR("h'");
}

static void edn_append_bstr_tstr(dec_context *ctx)
{
	APPEND_CHAR('\'');
	APPEND_SP();
	APPEND_CHAR('"');
}

static void edn_ctx_init(dec_context *ctx)
{
	memset(&ctx->u.edn.str, 0, sizeof ctx->u.edn.str);
	ctx->u.edn.indent_level = 0;
}

static void edn_ctx_free(dec_context *ctx)
{
	smart_str_free(&ctx->u.edn.str);
}

static void edn_si_free(stack_item *item_)
{
}

static void edn_si_push(dec_context *ctx, stack_item *item, stack_item *parent_item)
{
}

static void edn_si_pop(dec_context *ctx, stack_item *item_)
{
	stack_item_edn *item = (stack_item_edn *)item_;
	if (item->opt_indent == 2) {
		APPEND_DEINDENT_NL();
	}
}

typedef struct {
	uint8_t code;
	const char *name;
} error_desc;

const static error_desc error_descs[] = {
	{CBOR_ERROR_DEPTH, "depth"},
	{CBOR_ERROR_RECURSION, "recursion"},
	{CBOR_ERROR_SYNTAX, "syntax"},
	{CBOR_ERROR_UTF8, "utf8"},
	{CBOR_ERROR_TRUNCATED_DATA, "truncated data"},
	{CBOR_ERROR_MALFORMED_DATA, "malformed data"},
	{CBOR_ERROR_EXTRANEOUS_DATA, "extraneous data"},
	{CBOR_ERROR_INTERNAL, "internal"},
};

static int error_desc_cmp(const void *l, const void *r)
{
	return ((const error_desc *)l)->code - ((const error_desc *)r)->code;
}

static cbor_error edn_dec_finish(dec_context *ctx, cbor_decode_args *args, cbor_error error, zval *value)
{
	if (error) {
		uint8_t error_code = error & CBOR_ERROR_CODE_MASK;
		error_desc key_desc;
		key_desc.code = error_code;
		for (size_t i = 1; i < sizeof error_descs / sizeof error_descs[0]; i++) {
			assert(error_descs[i - 1].code < error_descs[i].code);
		}
		const error_desc *result_desc = bsearch(&key_desc, error_descs, sizeof error_descs / sizeof error_descs[0], sizeof error_descs[0], &error_desc_cmp);
		if (result_desc) {
			smart_str_append_printf(&ctx->u.edn.str, " /ERROR:%" PRIu32 " (%s)/", error_code, result_desc->name);
		} else {
			smart_str_append_printf(&ctx->u.edn.str, " /ERROR:%" PRIu32 "/", error_code);
		}
	}
	ZVAL_STR(value, smart_str_extract(&ctx->u.edn.str));
	return 0;
}

static void edn_stack_push_counted(dec_context *ctx, si_type_code si_type, uint32_t count)
{
	stack_item_edn *item = stack_new_item(ctx, si_type, count);
	item->indef_appended = false;
	stack_push_item(ctx, &item->base);
}

static void edn_stack_push_xstring(dec_context *ctx, si_type_code si_type)
{
	stack_item_edn *item = stack_new_item(ctx, si_type, 0);
	stack_push_item(ctx, &item->base);
}

static void edn_stack_push_map(dec_context *ctx, si_type_code si_type, uint32_t count)
{
	stack_item_edn *item = stack_new_item(ctx, si_type, count);
	if (count) {
		APPEND_CHAR('{');
		APPEND_INDENT_NL();
	} else {
		APPEND_LSTR("{_");
		APPEND_INDENT_NL_SP();
	}
	item->v.map_is_key = true;
	stack_push_item(ctx, &item->base);
}

static bool edn_append(dec_context *ctx, const char *str, size_t len);

static bool edn_append_counted(dec_context *ctx, const char *str, size_t len)
{
	stack_item *item = stack_pop_item(ctx);
	bool result = edn_append(ctx, str, len);
	stack_free_item(ctx, item);
	return result;
}

static bool edn_append_to_array(dec_context *ctx, stack_item_edn *item)
{
	if (item->base.count && --item->base.count == 0) {
		ASSERT_STACK_ITEM_IS_TOP(item);
		APPEND_DEINDENT_NL();
		return edn_append_counted(ctx, ZEND_STRL("]"));
	}
	item->indef_appended = true;
	APPEND_CHAR(',');
	APPEND_NL_SP();
	return true;
}

static bool edn_append_to_map(dec_context *ctx, stack_item_edn *item)
{
	if (item->v.map_is_key) {
		item->v.map_is_key = false;
		APPEND_CHAR(':');
		APPEND_SP();
		return true;
	}
	if (item->base.count && --item->base.count == 0) {
		ASSERT_STACK_ITEM_IS_TOP(item);
		APPEND_DEINDENT_NL();
		return edn_append_counted(ctx, ZEND_STRL("}"));
	}
	item->indef_appended = true;
	item->v.map_is_key = true;
	APPEND_CHAR(',');
	APPEND_NL_SP();
	return true;
}

static bool edn_append_to_tag(dec_context *ctx, stack_item_edn *item)
{
	ASSERT_STACK_ITEM_IS_TOP(item);
	stack_pop_item(ctx);
	stack_free_item(ctx, &item->base);
	return edn_append(ctx, ZEND_STRL(")"));
}

static bool edn_append(dec_context *ctx, const char *str, size_t len)
{
	if (len) {
		smart_str_appendl_ex(&ctx->u.edn.str, str, len, false);
	}
	stack_item_edn *item = (stack_item_edn *)ctx->stack_top;
	if (item == NULL) {
		return true;
	}
	switch (item->base.si_type) {
	case SI_TYPE_ARRAY:
		return edn_append_to_array(ctx, item);
	case SI_TYPE_MAP:
		return edn_append_to_map(ctx, item);
	case SI_TYPE_TAG:
		return edn_append_to_tag(ctx, item);
	}
	if (item->base.si_type & SI_TYPE_STRING_MASK) {
		return true;
	}
	RETURN_CB_ERROR_B(CBOR_ERROR_INTERNAL);
}

static void edn_proc_uint64(dec_context *ctx, uint64_t val)
{
	char buf[CBOR_INT_BUF_SIZE];
	size_t len = cbor_int_to_str(buf, val, false);
	edn_append(ctx, buf, len);
}

static void edn_proc_uint32(dec_context *ctx, uint32_t val)
{
	edn_proc_uint64(ctx, val);
}

static void edn_proc_nint64(dec_context *ctx, uint64_t val)
{
	char buf[CBOR_INT_BUF_SIZE];
	size_t len = cbor_int_to_str(buf, val, true);
	edn_append(ctx, buf, len);
}

static void edn_proc_nint32(dec_context *ctx, uint32_t val)
{
	edn_proc_nint64(ctx, val);
}

static void edn_append_esc(smart_str *str, char c)
{
	smart_str_appendc(str, '\\');
	smart_str_appendc(str, c);
}

static void edn_do_xstring(dec_context *ctx, const char *val, uint64_t length, bool is_text)
{
	stack_item_edn *item = (stack_item_edn *)ctx->stack_top;
#if UINT64_MAX > SIZE_MAX
	if (length > SIZE_MAX) {
		RETURN_CB_ERROR(CBOR_ERROR_UNSUPPORTED_SIZE);
	}
#endif
	if (item != NULL && item->base.si_type & SI_TYPE_STRING_MASK) {
		/* indefinite-length string */
		si_type_code str_si_type = is_text ? SI_TYPE_TEXT : SI_TYPE_BYTE;
		if (item->base.si_type != str_si_type) {
			RETURN_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, INCONSISTENT_STRING_TYPE));
		}
		if (!item->indef_appended) {
			item->indef_appended = true;
			APPEND_LSTR("(_");
			APPEND_INDENT_NL_SP();
		} else {
			APPEND_LSTR(",");
			APPEND_NL_SP();
		}
	}
	char hex_buf[3];
#define TO_BSTR() do { \
		if (in_text) { \
			in_text = false; \
			edn_append_tstr_bstr(ctx); \
		} \
	} while (0)
#define TO_TSTR() do { \
		if (!in_text) { \
			in_text = true; \
			edn_append_bstr_tstr(ctx); \
		} \
	} while (0)
#define APPEND_ESC_SEQ(c)  do { \
		esc_seq_char = c; \
		goto DO_APPEND_ESC_SEQ; \
	} while (0)
	const uint8_t *str = (const uint8_t *)val;
	if (is_text) {
		APPEND_CHAR('"');
		const uint8_t *end = str + length;
		bool in_text = true;
		char esc_seq_char;
		while (str < end) {
			if (!in_text) {
				if (IS_ASCII_CNTL(*str)) {
					APPEND_HEX(*str);
					str++;
					continue;
				}
			}
			switch (*str) {
			case '\\': APPEND_ESC_SEQ('\\'); break;
			case '"':  APPEND_ESC_SEQ('"'); break;
			case '\t': APPEND_ESC_SEQ('t'); break;
			case '\r': APPEND_ESC_SEQ('r'); break;
			case '\f': APPEND_ESC_SEQ('f'); break;
			case '\n': APPEND_ESC_SEQ('n'); break;
			case '\b': APPEND_ESC_SEQ('b'); break;
			DO_APPEND_ESC_SEQ:
				TO_TSTR();
				edn_append_esc(&ctx->u.edn.str, esc_seq_char);
				str++;
				break;
			default: {
				if (IS_ASCII_CNTL(*str)) {
					if (!in_text || str + 1 < end && IS_ASCII_CNTL(str[1])) {
						/* JSON-ish \uXXXX is lengthy; if now in hex notation or the next character is also cntl, use hex notation */
						TO_BSTR();
						continue;
					}
					TO_TSTR();
					smart_str_append_printf(&ctx->u.edn.str, "\\u%04x", *str);
					str++;
					break;
				}
				const uint8_t *prev = str;
				uint32_t cp = cbor_next_utf8(&str, end);
				if (cp == 0xffffffff) {  /* invalid character */
					TO_BSTR();
					do {
						APPEND_HEX(*str);
					} while (++str < end && (*str & 0xc0) == 0xc0);  /* 0b11000000: not start of character */
				} else {
					TO_TSTR();
					if ((cp >= 0x0080 && cp <= 0x009f)  /* C1 incl. NEXT LINE */
						|| cp == 0x200E || cp == 0x200F  /* bidi LTR, RTL */
						|| (cp >= 0x202A && cp <= 0x202E)  /* bidi LRE..RLO */
						|| (cp >= 0x2066 && cp <= 0x2069)  /* bidi LRI..PDI */
						|| cp == 0x2028 || cp == 0x2029 /* LINE SEP, PARA SEP */
						|| cp == 0x061C  /* ARABIC LETTER MARK */
					) {
						assert(cp <= 0xffff);
						smart_str_append_printf(&ctx->u.edn.str, "\\u%04x", cp);
					} else {
						smart_str_appendl_ex(&ctx->u.edn.str, (const char *)prev, str - prev, false);
					}
				}
				break;
			}
			}
		}
		APPEND_CHAR(in_text ? '"' : '\'');
	} else {
		const uint8_t *end = str + length;
		size_t offset = 0;
		uint8_t byte_space = ctx->args.edn.byte_space;
		uint16_t byte_wrap = ctx->args.edn.byte_wrap;
		if (item != NULL && byte_wrap && length > byte_wrap && item->opt_indent == 1) {
			item->opt_indent = 2;
			APPEND_INDENT_NL();
		}
		APPEND_LSTR("h'");
		while (str < end) {
			if (byte_wrap && offset == byte_wrap) {
				APPEND_CHAR('\'');
				APPEND_NL_SP();
				APPEND_LSTR("h'");
				offset = 0;
			} else if (byte_space && offset) {
				int space_len = 0;
				for (unsigned bm = 1; bm <= 32; bm <<= 1) {
					if ((byte_space & bm) && (offset % bm) == 0) {
						space_len++;
					}
				}
				if (space_len) {
					smart_str_appendl(&ctx->u.edn.str, "        ", space_len);
				}
			}
			APPEND_HEX(*str);
			str++;
			offset++;
		}
		APPEND_CHAR('\'');
	}
	edn_append(ctx, "", 0);
}

static void edn_proc_text_string(dec_context *ctx, const char *val, uint64_t length)
{
	edn_do_xstring(ctx, val, length, true);
}

static void edn_proc_text_string_start(dec_context *ctx)
{
	edn_stack_push_xstring(ctx, SI_TYPE_TEXT);
}

static void edn_proc_byte_string(dec_context *ctx, const char *val, uint64_t length)
{
	edn_do_xstring(ctx, val, length, false);
}

static void edn_proc_byte_string_start(dec_context *ctx)
{
	edn_stack_push_xstring(ctx, SI_TYPE_BYTE);
}

static void edn_proc_array_start(dec_context *ctx, uint32_t count)
{
	if (count) {
		APPEND_CHAR('[');
		APPEND_INDENT_NL();
		edn_stack_push_counted(ctx, SI_TYPE_ARRAY, (uint32_t)count);
	} else {
		edn_append(ctx, ZEND_STRL("[]"));
	}
}

static void edn_proc_indef_array_start(dec_context *ctx)
{
	APPEND_LSTR("[_");
	APPEND_INDENT_NL_SP();
	edn_stack_push_counted(ctx, SI_TYPE_ARRAY, 0);
}

static void edn_proc_map_start(dec_context *ctx, uint32_t count)
{
	if (count) {
		edn_stack_push_map(ctx, SI_TYPE_MAP, (uint32_t)count);
	} else {
		edn_append(ctx, ZEND_STRL("{}"));
	}
}

static void edn_proc_indef_map_start(dec_context *ctx)
{
	edn_stack_push_map(ctx, SI_TYPE_MAP, 0);
}

static void edn_proc_tag(dec_context *ctx, uint64_t val)
{
	char buf[CBOR_INT_BUF_SIZE];
	size_t len = cbor_int_to_str(buf, val, false);
	smart_str_appendl_ex(&ctx->u.edn.str, buf, len, false);
	APPEND_CHAR('(');
	stack_item_edn *item = (stack_item_edn *)stack_new_item(ctx, SI_TYPE_TAG, 1);
	item->opt_indent = 1;
	stack_push_item(ctx, &item->base);
}

static void edn_do_float(dec_context *ctx, double val, char indicator)
{
	char *str;
	char buf[ZEND_DOUBLE_MAX_LENGTH + 2 + 2 - 1];
	size_t len;
	if (isinf(val)) {
		str = strncpy(buf, (val > 0) ? "Infinity" : "-Infinity", sizeof buf - 3);
		len = strlen(str);
	} else if (isnan(val)) {
		str = strncpy(buf, "NaN", sizeof buf - 3);
		len = strlen(str);
	} else {
		int digits = indicator == '3' ? -1 : indicator == '2' ? 9 : 5;  /* cf. FLT_DECIMAL_DIG */
		str = zend_gcvt(val, digits, '.', 'e', buf);
		len = strlen(str);
		if (!memchr(str, '.', len)) {
			str[len++] = '.';
			str[len++] = '0';  /* '0' is required */
		}
	}
	str[len++] = '_';
	str[len++] = indicator;
	edn_append(ctx, str, len);
}

static void edn_proc_float16(dec_context *ctx, uint16_t val)
{
	double val64 = cbor_from_fp16i(val);
	edn_do_float(ctx, val64, '1');
}

static void edn_proc_float32(dec_context *ctx, uint32_t val)
{
	binary32_alias val32;
	val32.i = val;
	edn_do_float(ctx, (double)val32.f, '2');
}

static void edn_proc_float64(dec_context *ctx, double val)
{
	edn_do_float(ctx, val, '3');
}

static void edn_proc_null(dec_context *ctx)
{
	edn_append(ctx, ZEND_STRL("null"));
}

static void edn_proc_undefined(dec_context *ctx)
{
	edn_append(ctx, ZEND_STRL("undefined"));
}

static void edn_proc_simple(dec_context *ctx, uint32_t val)
{
	char buf[24];
	size_t len = snprintf(buf, sizeof buf, "simple(%" PRIu32 ")", val);
	edn_append(ctx, buf, len);
}

static void edn_proc_boolean(dec_context *ctx, bool val)
{
	if (val) {
		edn_append(ctx, ZEND_STRL("true"));
	} else {
		edn_append(ctx, ZEND_STRL("false"));
	}
}

static void edn_proc_indef_break(dec_context *ctx, stack_item *item_)
{
	stack_item_edn *item = (stack_item_edn *)item_;
	if (item->base.si_type & SI_TYPE_STRING_MASK) {
		bool is_text = item->base.si_type == SI_TYPE_TEXT;
		if (!item->indef_appended) {
			if (is_text) {
				edn_append(ctx, ZEND_STRL("\"\"_"));
			} else {
				edn_append(ctx, ZEND_STRL("''_"));
			}
		} else {
			APPEND_DEINDENT_NL();
			edn_append(ctx, ZEND_STRL(")"));
		}
	} else {  /* SI_TYPE_ARRAY, SI_TYPE_MAP, SI_TYPE_TAG */
		if (UNEXPECTED(item->base.count != 0)  /* definite-length */
				|| (item->base.si_type == SI_TYPE_MAP && UNEXPECTED(!item->v.map_is_key))) {  /* value is expected */
			THROW_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, BREAK_UNEXPECTED));
		}
		assert(item->base.si_type == SI_TYPE_ARRAY || item->base.si_type == SI_TYPE_MAP);
		if (item->indef_appended) {
			/* Remove trailing separator. Since string is appended on-the-fly, separator cannot be prepended before appending the next item. */
			char *ptr = ZSTR_VAL(ctx->u.edn.str.s);
			for (;;) {
				char ch = ptr[ZSTR_LEN(ctx->u.edn.str.s) - 1];
				if (!(ch == ' ' || ch == '\t' || ch == '\n' || ch == ',')) {
					break;
				}
				assert(ZSTR_LEN(ctx->u.edn.str.s) > 0);  /* should never underflow */
				ZSTR_LEN(ctx->u.edn.str.s)--;
			}
		}
		APPEND_DEINDENT_NL();
		if (item->base.si_type == SI_TYPE_ARRAY) {
			edn_append(ctx, ZEND_STRL("]"));
		} else {
			edn_append(ctx, ZEND_STRL("}"));
		}
	}
FINALLY: ;
}

#define METHOD(name) edn_##name
#include "decode_base.h"
#undef METHOD
