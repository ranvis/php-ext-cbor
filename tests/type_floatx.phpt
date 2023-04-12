--TEST--
floatx object behavior
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

class MyFloat extends Cbor\FloatX {
}

run(function () {
    $instance = new Cbor\Float32(2.0);
    eq(2.0, (float)$instance);
    $instance->value = 12.0;
    eq(12.0, (float)$instance);
    eq(12.0, $instance->value);
    $instance->value = INF;
    eq(INF, (float)$instance);
    $dump = print_r($instance, true);
    ok(str_contains($dump, '[value]'));
    $instance = new Cbor\Float16(2.0);
    eq(2.0, (float)$instance);
    $instance->value = 12.0;
    eq(12.0, (float)$instance);
    eq(12.0, $instance->value);
    eq(1, (int)$instance); // Warning; direct cast to int is unsupported!
    $instance->value = INF;
    eq(INF, (float)$instance);
    $instance->value = -INF;
    eq(-INF, (float)$instance);
    $instance->value = NAN;
    eq(NAN, (float)$instance);
    $instance = Cbor\Float32::fromBinary(hex2bin('3f800000'));
    eq(1.0, (float)$instance);
    ok(isset($instance->value));
    ok(!empty($instance->value));
    // clone
    $instance2 = clone $instance;
    ok($instance !== $instance2);
    // int is accepted
    $instance->value = 0;
    ok(empty($instance->value));
    eq(1.0, $instance2->value);
    // string of the valid length is accepted
    $instance->value = 'abcd';
    // serialization
    eq('O:12:"Cbor\Float32":1:{i:0;s:4:"abcd";}', serialize($instance));
    eq($instance, unserialize('O:12:"Cbor\Float32":1:{i:0;s:4:"abcd";}'));
    throws(Error::class, fn () => unserialize('O:12:"Cbor\Float32":1:{i:1;s:4:"abcd";}'));
    // export/restore
    eq($instance, eval('return ' . var_export($instance, true) . ';'));
    $instance->value = 1.5;
    eq(['value' => 1.5], get_object_vars($instance));
    throws(TypeError::class, fn () => Cbor\Float32::__set_state(true));
    throws(Error::class, fn () => Cbor\Float32::__set_state([]));
    // invalid type
    throws(TypeError::class, fn () => new Cbor\Float32('abcd'));
    throws(TypeError::class, fn () => Cbor\Float32::fromBinary([]));
    throws(ValueError::class, fn () => Cbor\Float32::fromBinary('ab'));
    throws(TypeError::class, function () use ($instance) {
        $instance->value = false;
    });
    throws(ValueError::class, function () use ($instance) {
        $instance->value = 'abc';
    });
    // cannot unset()
    throws(Error::class, function () use ($instance) {
        unset($instance->value);
    });
    // cannot set to ref
    throws(Error::class, function () use ($instance) {
        $v = 1.25;
        $instance->value = &$v;
    });
    throws(Error::class, function () use ($instance) {
        new MyFloat(1);
    });
    throws(Error::class, function () use ($instance) {
        MyFloat::fromBinary('abcd');
    });
    $instance = Cbor\Float16::fromBinary(hex2bin('3c00'));
    eq(1.0, (float)$instance);
    // multiple calls to get_properties during iteration
    array_walk($instance, fn ($v, $k) => true);

    eq('0xfa3fc00000', cenc(new Cbor\Float32(1.5), CBOR_FLOAT16));
    eq('0xfa3fc00000', cenc(new Cbor\Float32(1.5), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf93e00', cenc(new Cbor\Float16(1.5), CBOR_CDE));
    eq('0xf93e00', cenc(new Cbor\Float32(1.5), CBOR_CDE));
    eq('0xf93c01', cenc(new Cbor\Float32(1 + 1 / 1024), CBOR_CDE));
    eq('0xfa3f801000', cenc(new Cbor\Float32(1 + 1 / 2048), CBOR_CDE));
    eq('0xf9fc00', cenc(new Cbor\Float32(-INF), CBOR_CDE));
});

?>
--EXPECTF--
Warning: Object of class Cbor\Float16 could not be converted to int in %s on line %d
Done.
