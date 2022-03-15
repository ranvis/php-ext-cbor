--TEST--
tag stringref/stringref-namespace
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // example in http://cbor.schmorp.de/value-sharing
    $value = cdec('83808080', options: ['shared_ref' => true]);
    $value[0][0] = true;
    $value[1][0] = false;
    eq($value[0][0], true);
    eq([new Cbor\Tag(28, []), new Cbor\Tag(29, 0), []], cdec('83d81c80d81d0080', options: ['shared_ref' => false]));
    $value = cdec('83d81c80d81d0080', options: ['shared_ref' => true]);
    $value[0][0] = true;
    $value[1][0] = false;
    eq($value[0][0], false);

    cdecThrows(CBOR_ERROR_TAG_TYPE, '83d81c80d81d6080', options: ['shared_ref' => true]);
    cdecThrows(CBOR_ERROR_TAG_VALUE, '83d81c80d81d2080', options: ['shared_ref' => true]);
    cdecThrows(CBOR_ERROR_TAG_VALUE, '83d81d00d81c8080', options: ['shared_ref' => true]);

    $value = cdec('d81ca14140d81d00', options: ['shared_ref' => true]);
    eq(spl_object_id($value), spl_object_id($value->{'@'}));

    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1d81c4140d81d00', options: ['shared_ref' => true]);
});

?>
--EXPECT--
Done.
