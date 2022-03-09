--TEST--
array
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $expected = [1, [2, 3], [4, 5]];
    eq($expected, cdec('8301820203820405'));
    eq($expected, cdec('9f018202039f0405ffff'));
    eq($expected, cdec('9f018202039f0405ffff'));
    eq($expected, cdec('9f01820203820405ff'));
    eq($expected, cdec('83018202039f0405ff'));
    eq($expected, cdec('83019f0203ff820405'));

    cdecThrows(CBOR_ERROR_TRUNCATED_DATA, '9a1fffffff00000000');
});

?>
--EXPECT--
Done.
