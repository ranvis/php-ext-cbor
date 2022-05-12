/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

/**
 * Binary string that indicates the data is CBOR format.
 */
#define CBOR_SELF_DESCRIBE_DATA  "\xd9\xd9\xf7"

/**
 * Encode: Add self-describe tag (magic) to the beginning of the stream.
 *
 * Decode: Retain self-describe tag at the beginning of the stream.
 *   The tag is stripped if one exists and the flag is NOT specified.
 */
#define CBOR_SELF_DESCRIBE    (1 << 15)

/**
 * Encode: Treat all PHP string as CBOR byte string (binary) instead of CBOR text string (UTF-8 encoded byte sequences).
 *   Either CBOR_BYTE or CBOR_TEXT must be specified to encode PHP string.
 *
 * Decode: Accept text strings as PHP string instead of Cbor\Byte instance.
 */
#define CBOR_BYTE             (1 << 0)

/**
 * Encode: Treat all PHP string as CBOR text string (UTF-8 encoded byte sequences) instead of CBOR byte string (binary).
 *
 * Decode: Accept text strings as PHP string instead of Cbor\String instance.
 */
#define CBOR_TEXT             (1 << 1)

/**
 * Encode: Store integer keys in PHP array/object as integer for map key.
 *
 * Decode: Accept integer keys in map as numeric string of PHP object key, or PHP array key.
 */
#define CBOR_INT_KEY          (1 << 2)

/**
 * Encode: Store keys of map as byte string.
 *   Either CBOR_KEY_BYTE or CBOR_KEY_TEXT must be specified to encode PHP array/object string key.
 *
 * Decode: Accept text string instead of byte string for map string key.
 */
#define CBOR_KEY_BYTE         (1 << 3)

/**
 * Encode: Store keys of map as text string.
 *
 * Decode: Accept text string instead of byte string for map string key.
 */
#define CBOR_KEY_TEXT         (1 << 4)

/**
 * Ignore invalid UTF-8 sequences in text string.
 */
#define CBOR_UNSAFE_TEXT      (1 << 5)

/**
 * Decode: Translate map to PHP array.
 */
#define CBOR_MAP_AS_ARRAY     (1 << 6)

/**
 * Decode: Error if map key overwrites an existing key.
 */
#define CBOR_MAP_NO_DUP_KEY   (1 << 7)

/**
 * Encode: Store PHP floats (which should be in double-precision, IEEE 754 binary64) as half-precision floats (IEEE 754 binary16).
 *
 * Decode: Accept half-precision floats as PHP float instead of Cbor\Float16.
 */
#define CBOR_FLOAT16          (1 << 8)

/**
 * Encode: Store PHP floats as single-precision floats (IEEE 754 binary32).
 *
 * Decode: Accept single-precision floats as PHP float instead of Cbor\Float32.
 */
#define CBOR_FLOAT32          (1 << 9)

/**
 * Decode: Decode data to Extended Diagnostic Notation string.
 */
#define CBOR_EDN              (1 << 14)

/**
 * Encode: Store length of arrays/maps as indefinite if number of elements are large enough. While it makes encoded stream shorter by a byte or two per group, decoding can have a little overhead due to inefficient memory allocations.
 * Any use case?
 */
/* EXPERIMENTAL: */ #define CBOR_PACKED           (1 << 31)
