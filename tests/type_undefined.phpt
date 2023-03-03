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
    throws(Error::class, fn () => $instance->abc);
    throws(Error::class, fn () => $instance->abc = true);
    throws(Error::class, function () use ($instance) {
        unset($instance->abc);
    });
    throws(Error::class, fn () => $x = &$instance->abc);
    throws(Error::class, fn () => new Cbor\Undefined());
    $ctor = (new ReflectionClass(Cbor\Undefined::class))->getMethod('__construct');
    ok($ctor->isPrivate());
    if (PHP_VERSION_ID < 80100) {
        $ctor->setAccessible(true);
    }
    throws(Error::class, fn () => $ctor->invoke(Cbor\Undefined::get()));
    eq(1, (int)$instance);  // Warning; Same as (int)new stdClass

    ok(!isset($instance->foo));

    // uniqueness for serialization
    $serialized = 'C:14:"Cbor\Undefined":0:{}';
    eq($serialized, serialize($instance));
    ok($instance === unserialize($serialized));
    // invalid serialization
    $serialized = 'C:14:"Cbor\Undefined":1:{a}';
    throws(Error::class, fn () => unserialize($serialized));

    // void context
    Cbor\Undefined::get();
    $instance = null;
    Cbor\Undefined::get();
});

?>
--EXPECTF--
Warning: Object of class Cbor\Undefined could not be converted to int in %s on line %d
Done.
