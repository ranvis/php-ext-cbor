--TEST--
'undefined' object behavior
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // void context
    Cbor\Undefined::get();

    // singleton
    $instance = Cbor\Undefined::get();
    ok($instance === Cbor\Undefined::get());

    // clone is the same object
    ok($instance === (clone $instance));

    // dumped value is the same object
    ok($instance === eval('return ' . var_export($instance, true) . ';'));

    // evaluated as false
    ok($instance ? false : true);
    ok(!$instance ? true : false);
    ok($instance == false);

    // not the false value
    ok($instance !== false);

    // not the null value
    ok($instance != null);

    // prohibited actions
    throws('Error', fn () => $instance->abc);
    throws('Error', fn () => $instance->abc = true);
    throws('Error', function () use ($instance) {
        unset($instance->abc);
    });
    throws('Error', fn () => $x = &$instance->abc);
    throws('Error', fn () => new Cbor\Undefined());

    // uniqueness for serialization
    $serialized = 'C:14:"Cbor\Undefined":0:{}';
    eq($serialized, serialize($instance));
    ok($instance === unserialize($serialized));

    // void context
    Cbor\Undefined::get();
    $instance = null;
    Cbor\Undefined::get();
});

?>
--EXPECT--
Done.
