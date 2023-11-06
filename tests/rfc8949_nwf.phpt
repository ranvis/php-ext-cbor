--TEST--
RFC 8949 F.1. not well-formed
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // End of input in a head
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '18');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '19');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '1a');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '1b');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '1901');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '1a0102');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '1b01020304050607');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '38');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '58');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '78');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '98');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9a01ff00');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'b8');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'd8');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'f8');  // simple value (1 byte follows)
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'f900');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'fa0000');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'fb000000');
    // Definite-length strings with short data
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '41');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '61');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '5affffffff00');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '5bffffffffffffffff010203');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '7affffffff00');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '7b7fffffffffffffff010203');
    // Definite-length maps and arrays not closed with enough items
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '81');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '818181818181818181');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '8200');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'a1');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'a20102', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'a100', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'a2000000', CBOR_INT_KEY);
    // Tag number not followed by tag content
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'c0');
    // Indefinite-length strings not closed by a "break" stop code
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '5f4100');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '7f6100');
    // Indefinite-length maps and arrays not closed by a "break" stop code
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9f');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9f0102');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'bf');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'bf01020102', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '819f');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9f8000');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9f9f9f9f9fffffffff');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9f819f819f9fffffff');

    // Subkind 1:
    // Reserved additional information values
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '1c');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '1d');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '1e');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '3c');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '3d');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '3e');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '5c');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '5d');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '5e');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '7c');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '7d');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '7e');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '9c');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '9d');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '9e');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'bc');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'bd');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'be');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'dc');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'dd');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'de');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'fc');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'fd');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'fe');

    // Subkind 2:
    // Reserved two-byte encodings of simple values
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'f800');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'f801');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'f818');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'f81f');

    // Subkind 3:
    // Indefinite-length string chunks not of the correct type
    cdecThrows(CBOR_ERROR_SYNTAX, '5f00ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '5f21ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '5f6100ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '5f80ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '5fa0ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '5fc000ff');
    cdecThrows(CBOR_ERROR_UNSUPPORTED_TYPE, '5fe0ff');  // e0:simple(0)
    cdecThrows(CBOR_ERROR_SYNTAX, '7f4100ff');

    // Indefinite-length string chunks not definite length
    cdecThrows(CBOR_ERROR_SYNTAX, '5f5f4100ffff');
    cdecThrows(CBOR_ERROR_SYNTAX, '7f7f6100ffff');

    // Subkind 4:
    // Break occurring on its own outside of an indefinite-length item
    cdecThrows(CBOR_ERROR_SYNTAX, 'ff');

    // Break occurring in a definite-length array or map or a tag
    // (some tests have data appended to avoid throwing prematurely)
    cdecThrows(CBOR_ERROR_SYNTAX, '81ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '8200ff');
    cdecThrows(CBOR_ERROR_SYNTAX, 'a1ff' . 'ff');
    cdecThrows(CBOR_ERROR_SYNTAX, 'a1ff00');
    cdecThrows(CBOR_ERROR_SYNTAX, 'a100ff', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_SYNTAX, 'a20000ff' . 'ff', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_SYNTAX, '9f81ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '9f829f819f9fffffffff');

    // Break in an indefinite-length map that would lead to an odd
    // number of items (break in a value position)
    cdecThrows(CBOR_ERROR_SYNTAX, 'bf00ff', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_SYNTAX, 'bf000000ff', CBOR_INT_KEY);

    // Subkind 5:
    // Major type 0, 1, 6 with additional information 31
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '1f');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, '3f');
    cdecThrows(CBOR_ERROR_MALFORMED_DATA, 'df');
});

?>
--EXPECT--
Done.
