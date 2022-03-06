--TEST--
Encode RFC8949 examples (over 32bit)
--SKIPIF--
<?php if (
    !extension_loaded("cbor")
    || PHP_INT_SIZE < 8
) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x1b000000e8d4a51000', cenc(1000000000000));
});

?>
--EXPECT--
Done.
