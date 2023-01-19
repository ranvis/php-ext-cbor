/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <main/php.h>
#include <main/php_ini.h>
#include <ext/date/php_date.h> /* not globally required, here to mute a warning by this */
#include <ext/standard/info.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_smart_str.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include "php_cbor.h"

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif
