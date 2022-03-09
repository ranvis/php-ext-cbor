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

    eq("'1234567'", cdecExport('4731323334353637'));
    eq("'1234567'", cdecExport('5f443132333443353637ff'));
    eq("'1234567'", cdecExport('7f606064313233346060633536376060ff', CBOR_TEXT));

    cdecThrows(CBOR_ERROR_SYNTAX, '5f643132333443353637ff', CBOR_BYTE);
    cdecThrows(CBOR_ERROR_SYNTAX, '5f443132333463353637ff', CBOR_BYTE);
    cdecThrows(CBOR_ERROR_SYNTAX, '5f6431323334ff', CBOR_BYTE);
    cdecThrows(CBOR_ERROR_SYNTAX, '5f01ff', CBOR_BYTE);

    eq('0x80', cdecHex('4180'));
    eq('0x80', cdecHex('6180', CBOR_TEXT | CBOR_UNSAFE_TEXT));
    cdecThrows(CBOR_ERROR_UTF8, '6180', CBOR_TEXT);
    eq('0x64f09f9880', cenc('😀', CBOR_TEXT));
});

?>
--EXPECT--
Done.