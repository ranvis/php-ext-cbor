--TEST--
array
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x83010203', cenc([0 => 1, 1 => 2, 2 => 3]));
    $expected = [1, [2, 3], [4, 5]];
    eq($expected, cdec('8301820203820405'));
    eq($expected, cdec('9f018202039f0405ffff'));
    eq($expected, cdec('9f018202039f0405ffff'));
    eq($expected, cdec('9f01820203820405ff'));
    eq($expected, cdec('83018202039f0405ff'));
    eq($expected, cdec('83019f0203ff820405'));
    cdecThrows(CBOR_ERROR_EXTRANEOUS_DATA, '810000');

    // each length size, not canonical
    eq($expected, cdec('980301820203820405'));
    eq($expected, cdec('99000301820203820405'));
    eq($expected, cdec('9a0000000301820203820405'));
    eq($expected, cdec('9b000000000000000301820203820405'));

    // list (repackable)
    $list = [3 => 3, 0 => 0, 1 => 1, 2 => 2];
    unset($list[3]);
    eq('0x83000102', cenc($list));

    cdecThrows(CBOR_ERROR_UNSUPPORTED_SIZE, '9b0000000100000000');
});

?>
--EXPECT--
Done.
