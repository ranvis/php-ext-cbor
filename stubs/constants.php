<?php
/**
 * @author SATO Kentaro
 * @license BSD-2-Clause
 */

/* flags start */
/**
 * Binary string that indicates the data is CBOR format.
 */
const CBOR_SELF_DESCRIBE_DATA = "\xd9\xd9\xf7";

/**
 * Encode: Add self-describe tag (magic) to the beginning of the stream.
 *
 * Decode: Retain self-describe tag at the beginning of the stream.
 *   The tag is stripped if one exists and the flag is NOT specified.
 */
const CBOR_SELF_DESCRIBE = (1 << 15);

/**
 * Encode: Treat all PHP string as CBOR byte string (binary) instead of CBOR text string (UTF-8 encoded byte sequences).
 *   Either CBOR_BYTE or CBOR_TEXT must be specified to encode PHP string.
 *
 * Decode: Accept text strings as PHP string instead of Cbor\Byte instance.
 */
const CBOR_BYTE = (1 << 0);

/**
 * Encode: Treat all PHP string as CBOR text string (UTF-8 encoded byte sequences) instead of CBOR byte string (binary).
 *
 * Decode: Accept text strings as PHP string instead of Cbor\String instance.
 */
const CBOR_TEXT = (1 << 1);

/**
 * Encode: Store integer keys in PHP array/object as integer for map key.
 *
 * Decode: Accept integer keys in map as numeric string of PHP object key, or PHP array key.
 */
const CBOR_INT_KEY = (1 << 2);

/**
 * Encode: Store keys of map as byte string.
 *   Either CBOR_KEY_BYTE or CBOR_KEY_TEXT must be specified to encode PHP array/object string key.
 *
 * Decode: Accept text string instead of byte string for map string key.
 */
const CBOR_KEY_BYTE = (1 << 3);

/**
 * Encode: Store keys of map as text string.
 *
 * Decode: Accept text string instead of byte string for map string key.
 */
const CBOR_KEY_TEXT = (1 << 4);

/**
 * Ignore invalid UTF-8 sequences in text string.
 */
const CBOR_UNSAFE_TEXT = (1 << 5);

/**
 * Decode: Translate map to PHP array.
 */
const CBOR_MAP_AS_ARRAY = (1 << 6);

/**
 * Decode: Error if map key overwrites an existing key.
 */
const CBOR_MAP_NO_DUP_KEY = (1 << 7);

/**
 * Encode: Store PHP floats (which should be in double-precision, IEEE 754 binary64) as half-precision floats (IEEE 754 binary16).
 *
 * Decode: Accept half-precision floats as PHP float instead of Cbor\Float16.
 */
const CBOR_FLOAT16 = (1 << 8);

/**
 * Encode: Store PHP floats as single-precision floats (IEEE 754 binary32).
 *
 * Decode: Accept single-precision floats as PHP float instead of Cbor\Float32.
 */
const CBOR_FLOAT32 = (1 << 9);

/**
 * Decode: Decode data to Extended Diagnostic Notation string.
 */
const CBOR_EDN = (1 << 14);
/* flags end */

/* errors start */
const CBOR_ERROR_INVALID_FLAGS = 1;
const CBOR_ERROR_INVALID_OPTIONS = 2;
const CBOR_ERROR_DEPTH = 3;
const CBOR_ERROR_RECURSION = 4;
const CBOR_ERROR_SYNTAX = 5;
const CBOR_ERROR_UTF8 = 6;
const CBOR_ERROR_UNSUPPORTED_TYPE = 17;
const CBOR_ERROR_UNSUPPORTED_VALUE = 18;
const CBOR_ERROR_UNSUPPORTED_SIZE = 19;
const CBOR_ERROR_UNSUPPORTED_KEY_TYPE = 25;
const CBOR_ERROR_UNSUPPORTED_KEY_VALUE = 26;
const CBOR_ERROR_UNSUPPORTED_KEY_SIZE = 27;
const CBOR_ERROR_DUPLICATE_KEY = 28;
const CBOR_ERROR_TRUNCATED_DATA = 33;
const CBOR_ERROR_MALFORMED_DATA = 34;
const CBOR_ERROR_EXTRANEOUS_DATA = 35;
const CBOR_ERROR_TAG_SYNTAX = 41;
const CBOR_ERROR_TAG_TYPE = 42;
const CBOR_ERROR_TAG_VALUE = 43;
const CBOR_ERROR_INTERNAL = 241;
/* errors end */
