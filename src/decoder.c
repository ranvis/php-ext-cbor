/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"
#include "codec.h"
#include "types.h"
#include <Zend/zend_exceptions.h>
#include <Zend/zend_interfaces.h>
#include <assert.h>

#define IS_STR_OWNED(str)  (!ZSTR_IS_INTERNED(str) && GC_REFCOUNT(str) <= 1)  /* eval twice */

typedef struct {
	cbor_decode_args args;
	zend_string *buffer;
	cbor_decode_context *ctx;
	cbor_fragment mem;
	bool is_processing;
	zval data;
	zend_object std;
} decoder_class;

static zend_object_handlers decoder_handlers;

static zend_object *decoder_create(zend_class_entry *ce)
{
	decoder_class *base = zend_object_alloc(sizeof(decoder_class), ce);
	base->buffer = zend_empty_string;
	base->mem.base = base->mem.offset = base->mem.length = base->mem.limit = 0;
	base->ctx = NULL;
	base->is_processing = false;
	ZVAL_UNDEF(&base->data);
	zend_object_std_init(&base->std, ce);
	base->std.handlers = &decoder_handlers;
	return &base->std;
}

static void decoder_free(zend_object *obj)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, obj);
	if (base->ctx) {
		php_cbor_decode_delete(base->ctx);
	}
	zend_string_release(base->buffer);
	zend_object_std_dtor(obj);
}

#define NO_REENTRANT(base)  do { \
		if (base->is_processing) { \
			zend_throw_error(NULL, "The operation is not permitted while decoding is in progress."); \
			RETURN_THROWS(); \
		} \
	} while (0)

PHP_METHOD(Cbor_Decoder, __construct)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_long flags = CBOR_BYTE | CBOR_KEY_BYTE;
	HashTable *options = NULL;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|lh", &flags, &options) != SUCCESS) {
		RETURN_THROWS();
	}
	base->args.flags = (uint32_t)flags;
	cbor_error error = php_cbor_set_decode_options(&base->args, options);
	if (error) {
		php_cbor_throw_error(error, false, 0);
		RETURN_THROWS();
	}
}

PHP_METHOD(Cbor_Decoder, decode)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_string *data;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &data) != SUCCESS) {
		RETURN_THROWS();
	}
	zval value;
	cbor_decode_args args = base->args;  /* Make a copy of decoding args. */
	cbor_error error = php_cbor_decode(data, &value, &args);
	if (error) {
		php_cbor_throw_error(error, true, args.error_arg);
		RETURN_THROWS();
	}
	RETVAL_COPY_VALUE(&value);
}

static void reset_buffer_offset(decoder_class *base)
{
	if (!base->mem.offset) {
		return;
	}
	assert(IS_STR_OWNED(base->buffer));  /* separation test */
	char *ptr = ZSTR_VAL(base->buffer);
	memmove(ptr, &ptr[base->mem.offset], base->mem.length - base->mem.offset);
	base->mem.length -= base->mem.offset;
	base->mem.base += base->mem.offset;
	base->mem.offset = 0;
}

static cbor_error decode_buffer(decoder_class *base)
{
	assert(Z_TYPE(base->data) == IS_UNDEF);
	if (base->mem.offset == base->mem.length) {
		return 0;
	}
	if (!base->ctx) {
		base->ctx = php_cbor_decode_new(&base->args, &base->mem);
	}
	assert(!base->is_processing);
	base->is_processing = true;
	base->mem.ptr = (const uint8_t *)ZSTR_VAL(base->buffer);
	cbor_error error = php_cbor_decode_process(base->ctx);
	base->is_processing = false;
	if (error == CBOR_ERROR_TRUNCATED_DATA) {  /* no enough data */
		return 0;
	}
	php_cbor_decode_finish(base->ctx, &base->args, error, &base->data);
	php_cbor_decode_delete(base->ctx);
	base->ctx = NULL;
	return error;
}

PHP_METHOD(Cbor_Decoder, add)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_string *data;
	zend_long data_off = 0;
	zend_long data_len = 0;
	bool is_len_null = true;
	NO_REENTRANT(base);
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|ll!", &data, &data_off, &data_len, &is_len_null) != SUCCESS) {
		RETURN_THROWS();
	}
	size_t append_len = ZSTR_LEN(data);
	if (data_off < 0) {
		data_off += append_len;
		data_off = max(0, data_off);
	}
	data_off = min((zend_ulong)data_off, append_len);
	if (!is_len_null) {
		if (!data_len) {
			return;
		} else if (data_len < 0) {
			data_len += append_len;
			data_len = max(0, max(0, data_len) - data_off);
		} else if ((zend_ulong)data_len >= append_len - data_off) {
			is_len_null = true;
		}
	}
	if ((zend_ulong)data_off >= append_len) {
		return;
	}
	if (base->mem.offset == base->mem.length && !data_off && is_len_null) {
		zend_string_release(base->buffer);
		base->buffer = data;
		base->mem.base += base->mem.offset;
		base->mem.offset = 0;
		base->mem.length = append_len;
		zend_string_addref(base->buffer);
		/* delay separation, to make it memory-efficient as caller can give up the string soon after this */
	} else {
		const char *src = ZSTR_VAL(data);
		if (data_off) {
			src += data_off;
			append_len -= data_off;
		}
		if (!is_len_null) {
			append_len = min((zend_ulong)data_len, append_len);
		}
		if (append_len > SIZE_MAX - base->mem.length) {
			php_cbor_throw_error(CBOR_ERROR_UNSUPPORTED_SIZE, false, 0);
			RETURN_THROWS();
		}
		if (!IS_STR_OWNED(base->buffer)) {
			/* separate to heap string, reset offset and add room to append */
			base->mem.length -= base->mem.offset;
			zend_string *new_str = zend_string_alloc(base->mem.length + append_len, false);
			memcpy(ZSTR_VAL(new_str), &ZSTR_VAL(base->buffer)[base->mem.offset], base->mem.length);
			zend_string_release(base->buffer);
			base->buffer = new_str;
			base->mem.base += base->mem.offset;
			base->mem.offset = 0;
		}
		if (base->mem.length + append_len > ZSTR_LEN(base->buffer)) {
			reset_buffer_offset(base);
			size_t new_size = base->mem.length + append_len;
			if (new_size > ZSTR_LEN(base->buffer)) {
				base->buffer = zend_string_realloc(base->buffer, new_size, false);
			}
		}
		char *ptr = ZSTR_VAL(base->buffer);
		memcpy(&ptr[base->mem.length], src, append_len);
		base->mem.length += append_len;
	}
}

PHP_METHOD(Cbor_Decoder, process)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	NO_REENTRANT(base);
	zend_parse_parameters_none();
	zval_ptr_dtor(&base->data);
	ZVAL_UNDEF(&base->data);
	cbor_error error = decode_buffer(base);
	if (base->mem.offset > 512 * 1024 && IS_STR_OWNED(base->buffer)) {
		/* shrink buffer if offset is large enough and not used by others */
		reset_buffer_offset(base);
		base->buffer = zend_string_realloc(base->buffer, base->mem.length, false);
	}
	if (error) {
		php_cbor_throw_error(error, true, base->args.error_arg);
		RETURN_THROWS();
	}
	RETVAL_BOOL(Z_TYPE(base->data) != IS_UNDEF);
}

PHP_METHOD(Cbor_Decoder, reset)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	NO_REENTRANT(base);
	zend_parse_parameters_none();
	if (base->ctx) {
		php_cbor_decode_delete(base->ctx);
		base->ctx = NULL;
	}
	zend_string_release(base->buffer);
	base->buffer = zend_empty_string;
	base->mem.base = base->mem.offset = base->mem.length = base->mem.limit = 0;
	zval_ptr_dtor(&base->data);
	ZVAL_UNDEF(&base->data);
}

PHP_METHOD(Cbor_Decoder, hasValue)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_parse_parameters_none();
	RETVAL_BOOL(Z_TYPE(base->data) != IS_UNDEF);
}

PHP_METHOD(Cbor_Decoder, getValue)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_bool clear = true;
	NO_REENTRANT(base);
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &clear) != SUCCESS) {
		RETURN_THROWS();
	}
	if (Z_TYPE(base->data) == IS_UNDEF) {
		zend_throw_exception(NULL, "The decoded value is not ready.", 0);
		RETURN_THROWS();
	}
	RETVAL_ZVAL(&base->data, true, clear);
	if (clear) {
		ZVAL_UNDEF(&base->data);
	}
}

PHP_METHOD(Cbor_Decoder, isPartial)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_parse_parameters_none();
	RETVAL_BOOL(base->ctx != NULL);
}

PHP_METHOD(Cbor_Decoder, isProcessing)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_parse_parameters_none();
	RETVAL_BOOL(base->is_processing);
}

PHP_METHOD(Cbor_Decoder, getBuffer)
{
	decoder_class *base = CUSTOM_OBJ(decoder_class, Z_OBJ_P(ZEND_THIS));
	zend_parse_parameters_none();
	zend_string *copy = zend_string_init_fast(&ZSTR_VAL(base->buffer)[base->mem.offset], base->mem.length - base->mem.offset);
	RETVAL_STR(copy);
}

void php_cbor_minit_decoder()
{
	CBOR_CE(decoder)->create_object = &decoder_create;
#if PHP_API_VERSION < 20210902 /* <PHP8.1 */
	CBOR_CE(decoder)->serialize = zend_class_serialize_deny;
	CBOR_CE(decoder)->unserialize = zend_class_unserialize_deny;
#endif
	memcpy(&decoder_handlers, &std_object_handlers, sizeof(zend_object_handlers));
	decoder_handlers.offset = XtOffsetOf(decoder_class, std);
	decoder_handlers.free_obj = &decoder_free;
	decoder_handlers.clone_obj = NULL;
}
