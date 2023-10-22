--TEST--
miscellaneous test
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

class UnencodableClass {}

run(function () {
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '');
    eq('0x00', cenc(0, options: null));
    eq(0, cdec('00', options: null));

    try {
        cbor_encode(new UnencodableClass());
    } catch (Cbor\Exception $e) {
        eqRegEx('/\b' . preg_quote(UnencodableClass::class, '/') . '\b/', $e->getMessage());
    }
});

?>
--EXPECT--
Done.
