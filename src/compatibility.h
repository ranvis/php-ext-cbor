/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#if PHP_API_VERSION < 20210902 /* <PHP8.1 */
#define ZEND_ACC_NOT_SERIALIZABLE 0

bool zend_array_is_list(zend_array *array);

static zend_long php81_zend_atol(const char *s)
{
	zend_long i;
	ZEND_ATOL(i, s);
	return i;
}
#undef ZEND_ATOL
#define ZEND_ATOL(s)  php81_zend_atol(s)
#endif
