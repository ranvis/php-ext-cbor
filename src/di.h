/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define DI_MAJOR_TYPE_MASK  0xe0  /* 0b1110_0000 */
#define DI_MAJOR_TYPE(n)  ((n) << 5)

/* additional info */
enum {
	DI_INFO_INT0_MAX = 23,

	DI_INFO_INT8,
	DI_INFO_INT16,
	DI_INFO_INT32,
	DI_INFO_INT64,

	DI_INFO_INDEF = 31,

	DI_INFO_MAX = 31,

	/* major type 7 additional info */
	DI_INFO_FALSE = 20,
	DI_INFO_TRUE,
	DI_INFO_NULL,
	DI_INFO_UNDEF,
	DI_INFO_SIMPLE,
	DI_INFO_FLOAT16,
	DI_INFO_FLOAT32,
	DI_INFO_FLOAT64,
	DI_INFO_BREAK = 31,
};

/* codec constants */
enum {
	DI_UINT = 1,
	DI_NINT,
	DI_BSTR,
	DI_TSTR,
	DI_ARRAY,
	DI_MAP,
	DI_TAG,
	/* above must corresponds to CBOR major types (with offset) */
	DI_FALSE,
	DI_TRUE,
	DI_NULL,
	DI_UNDEF,
	DI_SIMPLE0,
	DI_SIMPLE8,
	DI_FLOAT16,
	DI_FLOAT32,
	DI_FLOAT64,
	DI_BREAK,
};
