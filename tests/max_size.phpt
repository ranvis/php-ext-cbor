--TEST--
array
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, 'ba1fffffff00000000');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, 'ba1fffffff00000000', options: ['max_size' => 0x7fffffff]);

    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, '9a1fffffff00000000');
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9a1fffffff00000000', options: ['max_size' => 0x7fffffff]);
});

?>
--EXPECT--
Done.
