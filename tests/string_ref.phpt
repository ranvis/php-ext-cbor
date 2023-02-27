--TEST--
tag stringref/stringref-namespace
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // example in http://cbor.schmorp.de/stringref
    $value = ['1', '222', '333', '4', '555', '666', '777', '888', '999', 'aaa', 'bbb', 'ccc', 'ddd', 'eee', 'fff', 'ggg', 'hhh', 'iii', 'jjj', 'kkk', 'lll', 'mmm', 'nnn', 'ooo', 'ppp', 'qqq', 'rrr', '333', 'ssss', 'qqq', 'rrr', 'ssss'];
    $data = 'd9010098204131433232324333333341344335353543363636433737374338383843393939436161614362626243636363436464644365656543666666436767674368686843696969436a6a6a436b6b6b436c6c6c436d6d6d436e6e6e436f6f6f437070704371717143727272d819014473737373d8191743727272d8191818';
    eq($value, cdec($data, CBOR_BYTE));
    eq('0x' . $data, cenc($value, CBOR_BYTE, ['string_ref' => true]));
    $value = ['aaa', 'aaa', ['bbb', 'aaa', 'aaa'], ['ccc', 'ccc'], 'aaa'];
    $data = 'd901008563616161d81900d90100836362626263616161d81901d901008263636363d81900d81900';
    eq($value, cdec($data, CBOR_TEXT));

    eq('0xd901008543616161d819008343626262d81900d819008243636363d81902d81900', cenc($value, options: ['string_ref' => true]));
    eq('0x8543616161436161618343626262436161614361616182436363634363636343616161', cenc($value, options: ['string_ref' => 'explicit']));
    eq('0xd901008543616161d819008343626262d81900d819008243636363d81902d81900', cenc(new Cbor\Tag(Cbor\Tag::STRING_REF_NS, $value), options: ['string_ref' => 'explicit']));
    $value[2] = new Cbor\Tag(Cbor\Tag::STRING_REF_NS, $value[2]);
    $value[3] = new Cbor\Tag(Cbor\Tag::STRING_REF_NS, $value[3]);
    eq('0x' . $data, cenc($value, CBOR_TEXT, ['string_ref' => true]));

    // indefinite-string
    $value = ['000', '@@@', '111', '000', '111', ['000', '111']];
    $data = 'd901009f433030305f43404040ff43313131d81900d819019fd81900d81901ffff';
    eq($value, cdec($data, CBOR_BYTE));
    // no namespace
    cdecThrows(CBOR_ERROR_TAG_SYNTAX, '9f433030305f43404040ff43313131d81900d819019fd81900d81901ffff');
    // not unsigned int, unassigned number
    cdecThrows(CBOR_ERROR_TAG_TYPE, 'd901009f43303030d819f6ff');
    cdecThrows(CBOR_ERROR_TAG_VALUE, 'd901009f43303030d81920ff');
    cdecThrows(CBOR_ERROR_TAG_VALUE, 'd901009f43303030d81901ff');
    // no-flag
    $data = 'd901008243303030d81900';
    eq(['000', '000'], cdec($data));
    $value = new Cbor\Tag(Cbor\Tag::STRING_REF_NS, [
        '000', new Cbor\Tag(Cbor\Tag::STRING_REF, 0),
    ]);
    eq($value, cdec($data, options: ['string_ref' => false]));
    // double-tag
    eq('0xd90100' . $data, cenc($value, options: ['string_ref' => true]));
    // ref value check
    $value = new Cbor\Tag(Cbor\Tag::STRING_REF_NS, [
        '000', new Cbor\Tag(Cbor\Tag::STRING_REF, 1),
    ]);
    $data = 'd901008243303030d81901';
    eq('0x' . $data, cenc($value));
    cdecThrows(CBOR_ERROR_TAG_VALUE, $data);
    cencThrows(CBOR_ERROR_TAG_VALUE, $value, options: ['string_ref' => true]);
});

?>
--EXPECT--
Done.
