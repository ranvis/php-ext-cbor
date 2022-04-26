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
		return E_DESC(CBOR_ERROR_UNSUPPORTED_TYPE, SIMPLE);
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
	case DI_BREAK: {
		stack_item *item = stack_pop_item(ctx);
		if (UNEXPECTED(item == NULL)) {
			THROW_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, BREAK_UNDERFLOW));
		}
		if (UNEXPECTED(!item->si_type)) {
			THROW_CB_ERROR(E_DESC(CBOR_ERROR_SYNTAX, BREAK_UNEXPECTED));
		}
		proc_indef_break(ctx, item);
		stack_free_item(item);
		break;
	}
	default:
		out->read_len = 0;
		return CBOR_ERROR_MALFORMED_DATA;
	}
FINALLY:
	return error;
}
