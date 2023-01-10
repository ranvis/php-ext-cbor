--TEST--
offset and length options
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq([0, 1, 2, 3], cdec('8400010203', 0, ['offset' => 0, 'length' => null]));
    eq([0, 1, 2, 3], cdec('8400010203', 0, ['offset' => 0, 'length' => 5]));
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '8400010203', 0, ['length' => 6]);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '8400010203', 0, ['offset' => 5]);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '8400010203', 0, ['offset' => 6]);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '8400010203', 0, ['offset' => 1, 'length' => 5]);
    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '8400010203', 0, ['length' => 4]);
    cdecThrows(CBOR_ERROR_EXTRANEOUS_DATA, '8400010203', 0, ['offset' => 1]);
    eq(0, cdec('8400010203', 0, ['offset' => 1, 'length' => 1]));
    eq([0, 1], cdec('838200010203', 0, ['offset' => 1, 'length' => 3]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, '00', 0, ['offset' => null]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, '00', 0, ['length' => false]);

    $decoder = new Cbor\Decoder();
    $test_add = function ($expected, ...$args) use ($decoder) {
        $decoder->add(...$args);
        eq($expected, $decoder->getBuffer(), 1);
        $decoder->reset();
    };
    $test_add('', 'abc', 0, 0);
    $test_add('a', 'abc', 0, 1);
    $test_add('bc', 'abc', 1);
    $test_add('b', 'abc', 1, 1);
    $test_add('b', 'abc', 1, -1);
    $test_add('bc', 'abc', -2);
    $test_add('b', 'abc', -2, -1);
    $test_add('a', 'abc', -5, 1);
    $test_add('', 'abc', 0, -3);
    $test_add('', 'abc', 3, 0);
    $test_add('', 'abc', 3, -1);
    $test_add('', 'abc', 3, 3);
    $test_add('', 'abc', -1, 0);
    $test_add('', 'abc', -1, -2);
    $test_add('', 'abc', -5, -4);
    $test_add('', 'abc', 3, -1);
    $test_add('', 'abc', 0, PHP_INT_MIN);
    $test_add('abc', 'abc', 0, PHP_INT_MAX);
    $test_add('', 'abc', PHP_INT_MIN, 0);
    $test_add('', 'abc', PHP_INT_MIN, PHP_INT_MIN);
    $test_add('abc', 'abc', PHP_INT_MIN, PHP_INT_MAX);
    $test_add('', 'abc', PHP_INT_MAX, 0);
    $test_add('', 'abc', PHP_INT_MAX, PHP_INT_MIN);
    $test_add('', 'abc', PHP_INT_MAX, PHP_INT_MAX);
});

?>
--EXPECT--
Done.
