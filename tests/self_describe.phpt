--TEST--
self-describe flag
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $nullWithSd = 'd9d9f7' . 'f6';

    eq(null, cdec('f6'));
    // skip self-describe tag
    eq(null, cdec($nullWithSd));
    // retain self-describe tag
    eq(new Cbor\Tag(Cbor\Tag::SELF_DESCRIBE, null), cdec($nullWithSd, CBOR_SELF_DESCRIBE));
    // only the very first one is stripped
    eq(new Cbor\Tag(Cbor\Tag::SELF_DESCRIBE, null), cdec(bin2hex(CBOR_SELF_DESCRIBE_DATA) . $nullWithSd));

    eq('0xf6', cenc(null));
    eq('0x' . $nullWithSd, cenc(null, CBOR_SELF_DESCRIBE));

    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, bin2hex(CBOR_SELF_DESCRIBE_DATA));
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, bin2hex(substr(CBOR_SELF_DESCRIBE_DATA, 0, -1)));

});

?>
--EXPECT--
Done.
