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
    eq('0xa3613061306131a14131413161326130', cenc((object)[
        '0',
        new Cbor\EncodeParams((object)['1' => '1'], ['flags' => CBOR_BYTE | CBOR_KEY_BYTE]),
        '0',
    ], CBOR_TEXT | CBOR_KEY_TEXT));
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
    cencThrows(CBOR_ERROR_INVALID_FLAGS, new Cbor\EncodeParams(0, ['flags' => true]));
    cencThrows(CBOR_ERROR_INVALID_FLAGS, new Cbor\EncodeParams(0, ['flags_clear' => true]));
    // flags_clear
    eq('0x83f90000fa00000000f90000', cenc([0.0, new Cbor\EncodeParams(0.0, ['flags_clear' => CBOR_FLOAT16]), 0.0], CBOR_FLOAT16 | CBOR_FLOAT32));
    // options
    $dt = new DateTimeImmutable('2013-03-21T20:04:00Z');
    eq('0x8200c074323031332d30332d32315432303a30343a30305a', cenc([new Cbor\EncodeParams(0, ['datetime'=> false]), $dt]));
    cencThrows(CBOR_ERROR_UNSUPPORTED_TYPE, new Cbor\EncodeParams($dt, ['datetime'=> false]));
    // CDE
    cencThrows(CBOR_ERROR_INVALID_FLAGS, new Cbor\EncodeParams(1, ['flags_clear' => CBOR_CDE]));
    eq('0x01', cenc(new Cbor\EncodeParams(1, ['flags' => CBOR_CDE])));
    cencThrows(CBOR_ERROR_INVALID_FLAGS, new Cbor\EncodeParams(1, ['flags' => CBOR_CDE]), options: ['shared_ref' => true]);
});

?>
--EXPECT--
Done.
