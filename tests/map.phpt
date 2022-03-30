--TEST--
map values
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0xa0', cenc((object)[], 0));
    // non-list array
    eq('0xa3000102020103', cenc([0 => 1, 2 => 2, 1 => 3], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa3200100020103', cenc([-1 => 1, 0 => 2, 1 => 3], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq([-1 => 1, 0 => 2, 1 => 3], cdec('a3200100020103', CBOR_KEY_BYTE | CBOR_INT_KEY | CBOR_MAP_AS_ARRAY));

    // key starts with NUL character
    eq('0xa142006100', cenc((object)["\0a" => 0]));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_VALUE, 'a142006100');
    eq(["\0a" => 0], cdec('a142006100', CBOR_KEY_BYTE | CBOR_MAP_AS_ARRAY));

    // numeric string beyond zend_long as integer key
    eq('0xa31a80000000001bffffffffffffffff003bffffffffffffffff00', cenc([
        '2147483648' => 0,
        '18446744073709551615' => 0,
        '-18446744073709551616' => 0,
    ], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq([
        '2147483648' => 0,
        '18446744073709551615' => 0,
        '-18446744073709551616' => 0,
    ], cdec('a31a80000000001bffffffffffffffff003bffffffffffffffff00', CBOR_KEY_BYTE | CBOR_INT_KEY | CBOR_MAP_AS_ARRAY));
    eq('0xa21bfedcba9876543210003bfedcba987654321000', cenc((object)[
        '18364758544493064720' => 0,
        '-18364758544493064721' => 0,
    ], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq((object)[
        '18364758544493064720' => 0,
        '-18364758544493064721' => 0,
    ], cdec('a21bfedcba9876543210003bfedcba987654321000', CBOR_KEY_BYTE | CBOR_INT_KEY));

    // non-numeric or non-well-fromed numeric string as key
    eq('0xa14000', cenc((object)['' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa12000', cenc((object)['-1' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa10000', cenc((object)['0' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa143302e3000', cenc((object)['0.0' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa142303100', cenc((object)['01' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa14330786600', cenc((object)['0xf' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1422d3000', cenc((object)['-0' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1442d30786600', cenc((object)['-0xf' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1452d3030373700', cenc((object)['-0077' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1432d2d3100', cenc((object)['--1' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa142300000', cenc(["0\0" => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa142310000', cenc(["1\0" => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1432d310000', cenc(["-1\0" => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa142316100', cenc(['1a' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1432d316100', cenc(['-1a' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1412d00', cenc(['-' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa154313834343637343430373337303935353136313600', cenc(['18446744073709551616' => 0], CBOR_KEY_BYTE | CBOR_INT_KEY));

    eq('0xa1414b4156', cenc((object)['K' => 'V'], CBOR_BYTE | CBOR_KEY_BYTE));
    eq('0xa1614b4156', cenc((object)['K' => 'V'], CBOR_BYTE | CBOR_KEY_TEXT));
    eq('0xa1414b6156', cenc((object)['K' => 'V'], CBOR_TEXT | CBOR_KEY_BYTE));
    eq('0xa1614b6156', cenc((object)['K' => 'V'], CBOR_TEXT | CBOR_KEY_TEXT));

    eq((object)['K' => 'V'], cdec('a1414b4156', CBOR_BYTE | CBOR_KEY_BYTE));
    eq((object)['K' => 'V'], cdec('a1614b4156', CBOR_BYTE | CBOR_KEY_TEXT));
    eq((object)['K' => 'V'], cdec('a1414b6156', CBOR_TEXT | CBOR_KEY_BYTE));
    eq((object)['K' => 'V'], cdec('a1614b6156', CBOR_TEXT | CBOR_KEY_TEXT));

    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1614b01', CBOR_KEY_BYTE);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1414b01', CBOR_KEY_TEXT);

    eq('0xa10101', cenc((object)[1 => 1], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1413101', cenc((object)[1 => 1], CBOR_KEY_BYTE));
    cencThrows(CBOR_ERROR_INVALID_FLAGS, (object)[1 => 1], 0);
    eq('0xa10101', cenc([1 => 1], CBOR_KEY_BYTE | CBOR_INT_KEY));
    eq('0xa1413101', cenc([1 => 1], CBOR_KEY_BYTE));

    eq((object)['1' => 1], cdec('a1413101', CBOR_KEY_BYTE));
    eq((object)['1' => 1], cdec('a1613101', CBOR_KEY_TEXT));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1413101', 0);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1413101', CBOR_KEY_TEXT);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1613101', CBOR_KEY_BYTE);
    eq((object)['1' => 1], cdec('a10101', CBOR_INT_KEY));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a10101', 0);

    eq([1 => 1, 2 => 2], cdec('a24131010202', CBOR_MAP_AS_ARRAY | CBOR_KEY_BYTE | CBOR_INT_KEY));

    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'a101', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_SYNTAX, 'a1ff', 0);
    cdecThrows(CBOR_ERROR_SYNTAX, 'a101ff', CBOR_INT_KEY);
    eq((object)[], cdec('bfff', 0));
    cdecThrows(CBOR_ERROR_SYNTAX, 'bf01ff', CBOR_INT_KEY);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1818000');
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_VALUE, 'a1410000');
});

?>
--EXPECT--
Done.
