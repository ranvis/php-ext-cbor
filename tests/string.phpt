--TEST--
byte strings and text strings
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x40', cenc(''));
    eq('0x4463626f72', cenc('cbor'));
    eq('0x4463626f72', cenc('cbor', CBOR_BYTE));
    eq('0x6463626f72', cenc('cbor', CBOR_TEXT));
    cencThrows(CBOR_ERROR_INVALID_FLAGS, 'cbor', 0);
    eq('0x773132333435363738393031323334353637383930313233', cenc('12345678901234567890123', CBOR_TEXT));
    eq('0x7818313233343536373839303132333435363738393031323334', cenc('123456789012345678901234', CBOR_TEXT));

    eq('1234567', cdec('4731323334353637'));
    eq('1234567', cdec('5f443132333443353637ff'));
    eq('1234567', cdec('7f606064313233346060633536376060ff', CBOR_TEXT));
    eq('', cdec('7fff', CBOR_TEXT));
    eq(new Cbor\Text('00'), cdec('623030'));

    // indefinite contains wrong string type
    cdecThrows(CBOR_ERROR_SYNTAX, '5f643132333443353637ff', CBOR_BYTE);
    cdecThrows(CBOR_ERROR_SYNTAX, '5f443132333463353637ff', CBOR_BYTE);
    cdecThrows(CBOR_ERROR_SYNTAX, '5f6431323334ff', CBOR_BYTE);
    // indefinite contains tagged string
    cdecThrows(CBOR_ERROR_SYNTAX, '5fd901004431323334ff', CBOR_BYTE);
    // indefinite contains non-string
    cdecThrows(CBOR_ERROR_SYNTAX, '5f01ff', CBOR_BYTE);

    eq('0xc180', cdecHex('42c180'));
    eq('0xc180', cdecHex('62c180', CBOR_TEXT | CBOR_UNSAFE_TEXT));
    cdecThrows(CBOR_ERROR_UTF8, '62c180', CBOR_TEXT);
    cdecThrows(CBOR_ERROR_UTF8, '66eda0bdedb880', CBOR_TEXT);
    eq('0x64f09f9880', cenc('😀', CBOR_TEXT));

    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '41');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '5f');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '5f4130');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '7a1fffffff00000000');
});

?>
--EXPECT--
Done.
