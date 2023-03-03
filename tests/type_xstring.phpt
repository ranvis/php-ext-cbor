--TEST--
xstring object behavior
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $instance = new Cbor\Text('abc');
    eq('abc', $instance->value);
    // cast
    eq('abc', (string)$instance);
    eq(1, (int)$instance);  // Warning
    // clone
    $copy = clone $instance;
    eq((string)$instance, (string)$copy);
    // change
    $copy->value = 'def';
    eq('def', (string)$copy);
    // isset/empty/unset
    $copy->value = '';
    ok(isset($copy->value));
    ok(empty($copy->value));
    ok(!isset($copy->value2));
    throws(Error::class, function () use ($copy) {
        unset($copy->value);
    });
    // encode
    eq('0x63616263', cenc($instance));
    $instance2 = new Cbor\Byte('abc');
    eq('0x43616263', cenc($instance2));
    // serialization
    eq('O:9:"Cbor\\Text":1:{i:0;s:3:"abc";}', serialize($instance));
    eq($instance, unserialize('O:9:"Cbor\\Text":1:{i:0;s:3:"abc";}'));
    throws(Error::class, fn () => unserialize('O:9:"Cbor\\Text":1:{i:1;s:3:"abc";}'));
    // export/restore
    eq($instance, eval('return ' . var_export($instance, true) . ';'));
    eq(['value' => 'abc'], get_object_vars($instance));
    throws(TypeError::class, fn () => Cbor\Text::__set_state(true));
    throws(Error::class, fn () => Cbor\Text::__set_state([]));
    // cannot set to ref
    throws(Error::class, function () use ($instance) {
        $v = 'abc';
        $instance->value = &$v;
    });
    // cannot set non-string
    throws(TypeError::class, fn () => $instance->value = 1);
    // other properties
    throws(Error::class, fn () => $instance->xyz);
    throws(Error::class, fn () => $instance->xyz = 3);
    // cast to string (heap allocated string)
    $instance = new Cbor\Text(substr(rand() . 'abc', -3));
    eq('abc', (string)$instance);
    // multiple calls to get_properties during iteration
    array_walk($instance, fn ($v, $k) => true);
});

?>
--EXPECTF--
Warning: Object of class Cbor\Text could not be converted to int in %s on line %d
Done.
