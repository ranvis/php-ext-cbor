--TEST--
simple values
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0xf4', cenc(false));
    eq('0xf5', cenc(true));
    eq('0xf6', cenc(null));
    eq('0xf7', cenc(Cbor\Undefined::get()));

    eq(false, cdec('f4'));
    eq(true, cdec('f5'));
    eq(null, cdec('f6'));
    eq(Cbor\Undefined::get(), cdec('f7'));

    // simple value
    cdecThrows(CBOR_ERROR_UNSUPPORTED_TYPE, 'f820');

    // break
    cdecThrows(CBOR_ERROR_SYNTAX, 'ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '81ff');
});

?>
--EXPECT--
Done.
