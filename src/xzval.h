/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

/* Hack to treat 64bit values beyond ZEND_LONG in zval. */
/* zvals of these types should not be passed to external PHP functions. */

typedef zval xzval;

#define IS_X_UINT 251
#define IS_X_NINT 252

static zend_always_inline bool xz_isxxint(xzval *z) {
	return Z_TYPE_P(z) == IS_X_UINT || Z_TYPE_P(z) == IS_X_NINT;
}
#define XZ_ISXXINT_P(zp)  xz_isxxint(zp)
#define XZ_ISXXINT(z)  XZ_ISXXINT_P(&(z))

#ifdef WORDS_BIGENDIAN
#define WW_HI value.ww.w1
#define WW_LO value.ww.w2
#else
#define WW_HI value.ww.w2
#define WW_LO value.ww.w1
#endif
#define XZ_XINT(z)  (((uint64_t)(z).WW_HI << 32) | (z).WW_LO)
#define XZ_XINT_SET(z, v)  (z)->WW_HI = (uint32_t)(v >> 32), (z)->WW_LO = (uint32_t)v

#define XZ_XINT_P(zp)  XZ_XINT(*(zp))

#define XZVAL_XINT(zp, v, t)  do { \
		xzval *tmp = (zp); \
		XZ_XINT_SET(tmp, v); \
		Z_TYPE_INFO_P(tmp) = (t); \
	} while (0)
#define XZVAL_UINT(zp, v)  XZVAL_XINT(zp, v, IS_X_UINT)
#define XZVAL_NINT(zp, v)  XZVAL_XINT(zp, v, IS_X_NINT)

#define XZVAL_PURIFY(zp) do { \
		xzval *tmp = (zp); \
		if (XZ_ISXXINT_P(tmp)) { \
			ZVAL_UNDEF(tmp); \
		} \
	} while (0);
