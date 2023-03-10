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

    try {
        cbor_encode(new UnencodableClass());
    } catch (Cbor\Exception $e) {
        eqRegEx('/\b' . preg_quote(UnencodableClass::class, '/') . '\b/', $e->getMessage());
    }
});

?>
--EXPECT--
Done.
