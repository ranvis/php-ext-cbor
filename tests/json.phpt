--TEST--
JSON serialization
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // Undefined
    $instance = Cbor\Undefined::get();
    eq('null', json_encode($instance));
    // FloatX
    $instance = new Cbor\Float16(1.5);
    eq('1.5', json_encode($instance));
    $instance = new Cbor\Float32(1.5);
    eq('1.5', json_encode($instance));
    // Byte
    $instance = new Cbor\Byte('abc');
    eq('"abc"', json_encode($instance));
    $instance = new Cbor\Byte(hex2bin('c180'));
    eq(false, json_encode($instance));
    // Text
    $instance = new Cbor\Text('abc');
    eq('"abc"', json_encode($instance));
    // Shareable
    $instance = new Cbor\Shareable((object)[1 => 2]);
    eq('{"1":2}', json_encode($instance));
});

?>
--EXPECT--
Done.
