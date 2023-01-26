/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define TARGET_PHP_API_LT_81  (PHP_API_VERSION < 20210902)  /* <PHP8.1 */
#define TARGET_PHP_API_LT_82  (PHP_API_VERSION < 20220829)  /* <PHP8.2 */
/* Since it is a matter of the interface, PHP_VERSION_ID is not preferred */

#if TARGET_PHP_API_LT_82
typedef int zend_result_82;
#else
typedef zend_result zend_result_82;
#endif

#if TARGET_PHP_API_LT_81
#define ZEND_ACC_NOT_SERIALIZABLE 0

bool zend_array_is_list(zend_array *array);

#define ZEND_DOUBLE_MAX_LENGTH  PHP_DOUBLE_MAX_LENGTH
#define zend_gcvt  php_gcvt
#endif
