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
    // cast to string
    eq('abc', (string)$instance);
    // serialization
    eq('O:9:"Cbor\\Text":1:{i:0;s:3:"abc";}', serialize($instance));
    eq($instance, unserialize('O:9:"Cbor\\Text":1:{i:0;s:3:"abc";}'));
    throws(Error::class, fn () => unserialize('O:9:"Cbor\\Text":1:{i:1;s:3:"abc";}'));
    // export/restore
    eq($instance, eval('return ' . var_export($instance, true) . ';'));
    eq(['value' => 'abc'], get_object_vars($instance));
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
--EXPECT--
Done.
