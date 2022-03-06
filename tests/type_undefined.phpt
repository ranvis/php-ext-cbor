--TEST--
'undefined' object behavior
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $instance = Cbor\Undefined::get();

    // singleton
    eq($instance, Cbor\Undefined::get());

    // clone is the same object
    eq($instance, (clone $instance));

    // dumped value is the same object
    eq($instance, eval('return ' . var_export($instance, true) . ';'));

    // evaluated as false
    assert($instance ? false : true);
    assert(!$instance ? true : false);
    assert($instance == false);

    // not the false value
    assert($instance !== false);

    // not the null value
    assert($instance != null);

    // cannot new
    try {
        new Cbor\Undefined();
        assert(false);
    } catch (\Throwable $e) {
        eq(get_class($e), 'Error');
    }
});

?>
--EXPECT--
Done.
