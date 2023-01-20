/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "warn_muted.h"
#include <main/php.h>
#include "warn_unmuted.h"

#include "php_cbor.h"
#include <stdlib.h>

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))
#endif
