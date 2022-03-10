--TEST--
map values
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0xa0', cenc((object)[], 0));

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

    eq('0xa1413101', cenc((object)[1 => 1], CBOR_KEY_BYTE | CBOR_INT_KEY));
    cencThrows(CBOR_ERROR_INVALID_FLAGS, (object)[1 => 1], CBOR_INT_KEY);
    eq('0xa10101', cenc([1 => 1], CBOR_INT_KEY));

    eq((object)['1' => 1], cdec('a1413101', CBOR_KEY_BYTE));
    eq((object)['1' => 1], cdec('a1613101', CBOR_KEY_TEXT));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1413101', 0);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1413101', CBOR_KEY_TEXT);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1613101', CBOR_KEY_BYTE);
    eq((object)['1' => 1], cdec('a10101', CBOR_INT_KEY));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a10101', 0);

    eq([1 => 1, 2 => 2], cdec('a24131010202', CBOR_MAP_AS_ARRAY | CBOR_KEY_BYTE | CBOR_INT_KEY));
});

?>
--EXPECT--
Done.
