--TEST--
max_size option
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    if (PHP_INT_SIZE == 4) {
        $code = CBOR_ERROR_UNSUPPORTED_SIZE;
    } else {
        $code = CBOR_ERROR_TRUNCATED_DATA;
    }
    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, 'ba1fffffff00000000');
    cdecThrows($code, 'ba1fffffff00000000', options: ['max_size' => 0x7fffffff]);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, '9a1fffffff00000000');
    cdecThrows($code, '9a1fffffff00000000', options: ['max_size' => 0x7fffffff]);

    eq([1, 2], cdec('9f0102ff', options: ['max_size' => 2]));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, '9f0102ff', options: ['max_size' => 1]);

    eq((object)[1 => 1, 2], cdec('bf01010202ff', CBOR_INT_KEY, ['max_size' => 2]));
    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, 'bf01010202ff', CBOR_INT_KEY, ['max_size' => 1]);
});

?>
--EXPECT--
Done.
