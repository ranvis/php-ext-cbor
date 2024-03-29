/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define SET_READ_ERROR(result)  do { \
		if (UNEXPECTED(!result)) { \
			goto DI_FAILURE; \
		} \
	} while (0)

static cbor_error METHOD(dec_item)(const uint8_t *data, size_t len, cbor_di_decoded *out, dec_context *ctx)
{
	cbor_error error = 0;
	uint8_t type = cbor_di_get_type(data, len);
	out->read_len = 1;
	out->req_len = 0;
	switch (type) {
	case DI_UINT:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		if (!DI_IS_DOUBLE(*out)) {
			METHOD(proc_uint32)(ctx, out->v.i32);
		} else {
			METHOD(proc_uint64)(ctx, out->v.i64);
		}
		break;
	case DI_NINT:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		if (!DI_IS_DOUBLE(*out)) {
			METHOD(proc_nint32)(ctx, out->v.i32);
		} else {
			METHOD(proc_nint64)(ctx, out->v.i64);
		}
		break;
	case DI_BSTR:
		SET_READ_ERROR(cbor_di_read_str(data, len, out));
		if (EXPECTED(out->v.str.val)) {
			METHOD(proc_byte_string)(ctx, out->v.str.val, out->v.str.len);
		} else {
			METHOD(proc_byte_string_start)(ctx);
		}
		break;
	case DI_TSTR:
		SET_READ_ERROR(cbor_di_read_str(data, len, out));
		if (EXPECTED(out->v.str.val)) {
			METHOD(proc_text_string)(ctx, out->v.str.val, out->v.str.len);
		} else {
			METHOD(proc_text_string_start)(ctx);
		}
		break;
	case DI_ARRAY:
		SET_READ_ERROR(cbor_di_read_list(data, len, out));
		if (EXPECTED(!out->v.ext.is_indef)) {
			if (out->v.i64 > 0xffffffff) {
				return CBOR_ERROR_UNSUPPORTED_SIZE;
			}
			METHOD(proc_array_start)(ctx, (uint32_t)out->v.i64);
		} else {
			METHOD(proc_indef_array_start)(ctx);
		}
		break;
	case DI_MAP:
		SET_READ_ERROR(cbor_di_read_list(data, len, out));
		if (EXPECTED(!out->v.ext.is_indef)) {
			if (out->v.i64 > 0xffffffff) {
				return CBOR_ERROR_UNSUPPORTED_SIZE;
			}
			METHOD(proc_map_start)(ctx, (uint32_t)out->v.i64);
		} else {
			METHOD(proc_indef_map_start)(ctx);
		}
		break;
	case DI_TAG:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		METHOD(proc_tag)(ctx, !DI_IS_DOUBLE(*out) ? out->v.i32 : out->v.i64);
		break;
	case DI_FALSE:
	case DI_TRUE:
		METHOD(proc_boolean)(ctx, type == DI_TRUE);
		break;
	case DI_NULL:
		METHOD(proc_null)(ctx);
		break;
	case DI_UNDEF:
		METHOD(proc_undefined)(ctx);
		break;
	case DI_SIMPLE0:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		assert(out->v.i32 <= 31);
		METHOD(proc_simple)(ctx, out->v.i32);
		break;
	case DI_SIMPLE8:
		SET_READ_ERROR(cbor_di_read_int(data, len, out));
		if (out->v.i32 <= 31) {
			// 0..23: not-well-formed range is not used (RFC 8949 3.3)
			// 24..31 reserved to minimize confusion (RFC 8949 3.3)
			return CBOR_ERROR_MALFORMED_DATA;
		}
		METHOD(proc_simple)(ctx, out->v.i32);
		break;
	case DI_FLOAT16:
		SET_READ_ERROR(cbor_di_read_float(data, len, out));
		METHOD(proc_float16)(ctx, out->v.f16);
		break;
	case DI_FLOAT32:
		SET_READ_ERROR(cbor_di_read_float(data, len, out));
		METHOD(proc_float32)(ctx, out->v.i32);
		break;
	case DI_FLOAT64:
		SET_READ_ERROR(cbor_di_read_float(data, len, out));
		METHOD(proc_float64)(ctx, out->v.f64);
		break;
	case DI_BREAK: {
		stack_item *item = stack_pop_item(ctx);
		if (UNEXPECTED(item == NULL)) {
			THROW_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, BREAK_UNDERFLOW));
		}
		METHOD(proc_indef_break)(ctx, item);
		stack_free_item(ctx, item);
		break;
	}
	default:
		out->read_len = 0;
		return CBOR_ERROR_MALFORMED_DATA;
	}
	goto FINALLY;
DI_FAILURE:
	error = out->req_len ? CBOR_ERROR_TRUNCATED_DATA : CBOR_ERROR_MALFORMED_DATA;
	goto FINALLY;
FINALLY:
	return error;
}
