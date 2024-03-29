--TEST--
max_depth option
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $data = '00';
    $value = cdec($data, options: ['max_depth' => 0]);
    eq('0x' . $data, cenc($value, options: ['max_depth' => 0]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, options: ['max_depth' => -1]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, options: ['max_depth' => 16777216]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, options: ['max_depth' => true]);

    $data = 'd82600';
    cdecThrows(CBOR_ERROR_DEPTH, $data, options: ['max_depth' => 0]);
    $value = cdec($data, options: ['max_depth' => 1]);
    cencThrows(CBOR_ERROR_DEPTH, $value, options: ['max_depth' => 0]);
    eq('0x' . $data, cenc($value, options: ['max_depth' => 1]));

    $data = str_repeat('d826', 5) . '00';
    cdecThrows(CBOR_ERROR_DEPTH, $data, options: ['max_depth' => 4]);
    $value = cdec($data, options: ['max_depth' => 5]);
    cencThrows(CBOR_ERROR_DEPTH, $value, options: ['max_depth' => 4]);
    eq('0x' . $data, cenc($value, options: ['max_depth' => 5]));
});

?>
--EXPECT--
Done.
