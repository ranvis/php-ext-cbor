--TEST--
floatx object behavior
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
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
    // invalid type
    throws(TypeError::class, fn () => new Cbor\Float32('abcd'));
    throws(TypeError::class, fn () => Cbor\Float32::fromBinary([]));
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
});

?>
--EXPECT--
Done.
