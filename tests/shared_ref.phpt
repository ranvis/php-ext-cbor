--TEST--
tag shared_ref
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // example in http://cbor.schmorp.de/value-sharing
    $value = [[], null, []];
    $value[1] = &$value[0];
    eq('0x83808080', cenc($value));
    eq('0x83808080', cenc($value, options: ['shared_ref' => true]));
    eq('0x83d81c80d81d0080', cenc($value, options: ['shared_ref' => 'unsafe_ref']));
    $value = cdec('83808080', options: ['shared_ref' => true]);
    $value[0][0] = true;
    $value[1][0] = false;
    eq($value[0][0], true);
    eq([new Cbor\Tag(Cbor\Tag::SHAREABLE, []), new Cbor\Tag(Cbor\Tag::SHARED_REF, 0), []], cdec('83d81c80d81d0080', options: ['shared_ref' => false]));
    cdecThrows(CBOR_ERROR_TAG_TYPE, '83d81c80d81d0080', options: ['shared_ref' => true]);
    $value = cdec('83d81c80d81d0080', options: ['shared_ref' => 'unsafe_ref']);
    $value[0][0] = true;
    $value[1][0] = false;
    eq($value[0][0], false);

    $sh = new Cbor\Shareable('');
    eq([$sh, $sh], cdec('82d81c40d81d00', options: ['shared_ref' => 'shareable']));
    eq('', cdec('d81c40', options: ['shared_ref' => 'unsafe_ref']));

    cdecThrows(CBOR_ERROR_TAG_SYNTAX, 'd81cd81c00', options: ['shared_ref' => 'shareable']);
    cdecThrows(CBOR_ERROR_TAG_SYNTAX, 'd81cd81c00', options: ['shared_ref' => 'unsafe_ref']);
    cdecThrows(CBOR_ERROR_TAG_TYPE, '83d81c80d81d6080', options: ['shared_ref' => 'shareable']);
    cdecThrows(CBOR_ERROR_TAG_TYPE, '83d81c80d81d6080', options: ['shared_ref' => 'unsafe_ref']);
    cdecThrows(CBOR_ERROR_TAG_TYPE, '83d81c80d81d2080', options: ['shared_ref' => true]);
    cdecThrows(CBOR_ERROR_TAG_TYPE, '83d81c8100d81d2080', options: ['shared_ref' => true]);
    cdecThrows(CBOR_ERROR_TAG_VALUE, '83d81d00d81c8080', options: ['shared_ref' => true]);

    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1d81c4140d81d00', options: ['shared_ref' => 'shareable']);
    cdecThrows(CBOR_ERROR_UNSUPPORTED_KEY_TYPE, 'a1d81c4140d81d00', options: ['shared_ref' => 'unsafe_ref']);

    $value = cdec('82d81ca0d81d00', options: ['shared_ref' => true]);
    eq(spl_object_id($value[0]), spl_object_id($value[1]));
    cdecThrows(CBOR_ERROR_TAG_TYPE, '82d81ca0d81d00', CBOR_MAP_AS_ARRAY, ['shared_ref' => true]);
    $value = cdec('82d81ca0d81d00', CBOR_MAP_AS_ARRAY, ['shared_ref' => 'shareable']);
    eq(Cbor\Shareable::class, get_class($value[1]));
    eq(spl_object_id($value[0]), spl_object_id($value[1]));
    $value = cdec('82d81ca0d81d00', CBOR_MAP_AS_ARRAY, ['shared_ref' => 'unsafe_ref']);
    $value[0][0] = true;
    $value[1][0] = false;
    eq($value[0][0], false);

    $value = cdec('d81ca14140d81d00', options: ['shared_ref' => true]);
    eq(spl_object_id($value), spl_object_id($value->{'@'}));
    cencThrows(CBOR_ERROR_RECURSION, $value, options: ['shared_ref' => false]);
    eq('0xd81ca14140d81d00', cenc($value, options: ['shared_ref' => true])); // asserting above decode's reference usage
    $value->{'@'} = &$value;  // intentional & (must be skipped for encoding)
    eq('0xd81ca14140d81d00', cenc($value, options: ['shared_ref' => true]));
    eq('0xd81ca14140d81d00', cenc($value, options: ['shared_ref' => 'unsafe_ref']));
    $sc = new stdClass();
    $value = [$sc, $sc];
    eq('0x82d81ca0d81d00', cenc($value, options: ['shared_ref' => true]));
    $value = [$sc];
    $value[1] = &$value[0];
    eq('0x82d81ca0d81d00', cenc($value, options: ['shared_ref' => true]));

    // redundant shareable
    eq('0x81a0', cenc([new stdClass()], options: ['shared_ref' => true]));
    eq('0x81d81ca0', cenc([$tmp = new stdClass()], options: ['shared_ref' => true]));

    // Shareable is always shared
    $sh = new Cbor\Shareable('123');
    eq('0x82d81c43313233d81d00', cenc([$sh, $sh], options: ['shared_ref' => false]));
});

?>
--EXPECT--
Done.
