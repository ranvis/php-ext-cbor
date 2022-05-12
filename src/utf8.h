/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define is_utf8(str, len) cbor_is_utf8(str, len)

bool cbor_is_utf8(const uint8_t *str, size_t len);
uint32_t cbor_next_utf8(const uint8_t **str, const uint8_t *end);
