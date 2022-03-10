--TEST--
simple values
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0xf4', cenc(false));
    eq('0xf5', cenc(true));
    eq('0xf6', cenc(null));
    eq('0xf7', cenc(Cbor\Undefined::get()));

    eq('false', cdecExport('f4'));
    eq('true', cdecExport('f5'));
    eq('NULL', cdecExport('f6'));
    eq(<<<'END'
        Cbor\Undefined::__set_state(array(
        ))
        END
    , cdecExport('f7'));

    // break
    cdecThrows(CBOR_ERROR_SYNTAX, 'ff');
    cdecThrows(CBOR_ERROR_SYNTAX, '81ff');
});

?>
--EXPECT--
Done.
