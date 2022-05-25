/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#if PHP_API_VERSION < 20210902 /* <PHP8.1 */
#define ZEND_ACC_NOT_SERIALIZABLE 0

bool zend_array_is_list(zend_array *array);

#define ZEND_DOUBLE_MAX_LENGTH  PHP_DOUBLE_MAX_LENGTH
#define zend_gcvt  php_gcvt
#endif
