--TEST--
EncodeParams test
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // byte/text flags
    eq('0xa3413041304131a16131613141324130', cenc((object)[
        '0',
        new Cbor\EncodeParams((object)['1' => '1'], ['flags' => CBOR_TEXT | CBOR_KEY_TEXT]),
        '0',
    ], CBOR_BYTE | CBOR_KEY_BYTE));
    // nesting
    eq('0x834130836131413061314130', cenc([
        '0',
        new Cbor\EncodeParams([
            '1',
            new Cbor\EncodeParams('0', ['flags' => CBOR_BYTE]),
            '1',
        ], ['flags' => CBOR_TEXT]),
        '0',
    ], CBOR_BYTE));
    // invalid flags
    cencThrows(CBOR_ERROR_INVALID_FLAGS, new Cbor\EncodeParams(0, ['flags' => CBOR_BYTE | CBOR_TEXT]));
    // flags_clear
    eq('0x83f90000fa00000000f90000', cenc([0.0, new Cbor\EncodeParams(0.0, ['flags_clear' => CBOR_FLOAT16]), 0.0], CBOR_FLOAT16 | CBOR_FLOAT32));
    // options
    $dt = new DateTimeImmutable('2013-03-21T20:04:00Z');
    eq('0x8200c074323031332d30332d32315432303a30343a30305a', cenc([new Cbor\EncodeParams(0, ['datetime'=> false]), $dt]));
    cencThrows(CBOR_ERROR_UNSUPPORTED_TYPE, new Cbor\EncodeParams($dt, ['datetime'=> false]));
});

?>
--EXPECT--
Done.
