/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#include "cbor.h"

#if PHP_API_VERSION < 20210902 /* <PHP8.1 */
/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) Zend Technologies Ltd. (http://www.zend.com)           |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

/* Based on https://github.com/php/php-src/blob/a8b9dbc632685bfb000615c2599f252b417a61b6/Zend/zend_hash.h */
bool zend_array_is_list(zend_array *array)
{
	zend_long expected_idx = 0;
	zend_long num_idx;
	zend_string *str_idx;
	/* Empty arrays are lists */
	if (zend_hash_num_elements(array) == 0) {
		return 1;
	}

	/* Packed arrays are lists */
	if (HT_IS_PACKED(array)) {
		if (HT_IS_WITHOUT_HOLES(array)) {
			return 1;
		}
	}
	/* Check if the list could theoretically be repacked */
	ZEND_HASH_FOREACH_KEY(array, num_idx, str_idx) {
		if (str_idx != NULL || num_idx != expected_idx++) {
			return 0;
		}
	}
	ZEND_HASH_FOREACH_END();

	return 1;
}
#endif
