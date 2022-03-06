/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

#define PHP_CBOR_SELF_DESCRIBE_DATA  "\xd9\xd9\xf7"

/* EXPERIMENTAL: Flag values are subject to change. */

/**
 * Encode: Add self-describe tag (magic) to the beginning of the stream.
 * Decode: Retain self-describe tag at the beginning of the stream.
 *   The tag is stripped if one exists and the flag is NOT specified.
 */
#define PHP_CBOR_SELF_DESCRIBE    (1 << 0)

/**
 * Encode: Treat all PHP string as CBOR byte string (binary) instead of CBOR text string (UTF-8 encoded byte sequences).
 *   Either PHP_CBOR_BYTE or PHP_CBOR_TEXT must be specified to encode PHP string.
 * Decode: Accept text strings as PHP string instead of Cbor\Byte instance.
 */
#define PHP_CBOR_BYTE             (1 << 1)

/**
 * Encode: Treat all PHP string as CBOR text string (UTF-8 encoded byte sequences) instead of CBOR byte string (binary).
 *   It is caller's responsibility to pass valid UTF-8 strings.
 * Decode: Accept text strings as PHP string instead of Cbor\String instance.
 */
#define PHP_CBOR_TEXT             (1 << 2)

/**
 * Encode: Store unsigned integer keys in PHP array as unsigned integer for map key.
 * Decode: Accept unsigned integer keys in map as numeric string of PHP object key.
 */
#define PHP_CBOR_INT_KEY          (1 << 3)

/**
 * Encode: Store strings keys of map as byte string instead of text string.
 *   Either PHP_CBOR_KEY_BYTE or PHP_CBOR_KEY_TEXT must be specified to encode PHP array/object string key.
 * Decode: Accept text string instead of byte string for map string key.
 */
#define PHP_CBOR_KEY_BYTE         (1 << 4)

/**
 * Encode: Store strings keys of map as text string instead of byte string.
 * Decode: Accept text string instead of byte string for map string key.
 */
#define PHP_CBOR_KEY_TEXT         (1 << 5)

/** TODO: */
#define PHP_CBOR_MAP_AS_ARRAY     (1 << 6)

/**
 * Ignore invalid UTF-8 sequences in text string.
 */
#define PHP_CBOR_UNSAFE_TEXT      (1 << 7)

/**
 * Encode: Store PHP floats (which should be in double-precision, IEEE 754 binary64) as half-precision floats (IEEE 754 binary16)
 * Decode: Accept half-precision floats as PHP float instead of Cbor\Float16
 */
#define PHP_CBOR_FLOAT16          (1 << 8)

/**
 * Encode: Store PHP floats as single-precision floats (IEEE 754 binary32)
 * Decode: Accept single-precision floats as PHP float instead of Cbor\Float32
 */
#define PHP_CBOR_FLOAT32          (1 << 9)

/** EXPERIMENTAL: TODO: */
#define PHP_CBOR_COMPACT_FLOAT    (1 << 10)

/**
 * Encode: Store length of arrays/maps as indefinite if number of elements are large enough. While it makes encoded stream shorter by a byte or two per group, decoding can have a little overhead due to inefficient memory allocations.
 * Any use case?
 */
/* EXPERIMENTAL: */ #define PHP_CBOR_PACKED           (1 << 31)
