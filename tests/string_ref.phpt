--TEST--
tag stringref/stringref-namespace
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // example in http://cbor.schmorp.de/stringref
    $value = ['1', '222', '333', '4', '555', '666', '777', '888', '999', 'aaa', 'bbb', 'ccc', 'ddd', 'eee', 'fff', 'ggg', 'hhh', 'iii', 'jjj', 'kkk', 'lll', 'mmm', 'nnn', 'ooo', 'ppp', 'qqq', 'rrr', '333', 'ssss', 'qqq', 'rrr', 'ssss'];
    $data = 'd9010098204131433232324333333341344335353543363636433737374338383843393939436161614362626243636363436464644365656543666666436767674368686843696969436a6a6a436b6b6b436c6c6c436d6d6d436e6e6e436f6f6f437070704371717143727272d819014473737373d8191743727272d8191818';
    eq($value, cdec($data, CBOR_BYTE));
    $value = ['aaa', 'aaa', ['bbb', 'aaa', 'aaa'], ['ccc', 'ccc'], 'aaa'];
    $data = 'd901008563616161d81900d90100836362626263616161d81901d901008263636363d81900d81900';
    eq($value, cdec($data, CBOR_TEXT));

    // indefinite-string
    $value = ['000', '@@@', '111', '000', '111', ['000', '111']];
    $data = 'd901009f433030305f43404040ff43313131d81900d819019fd81900d81901ffff';
    eq($value, cdec($data, CBOR_BYTE));
	// no stringref-namespace
    cdecThrows(CBOR_ERROR_TAG_SYNTAX, '9f433030305f43404040ff43313131d81900d819019fd81900d81901ffff');
	// not unsigned int, unassigned number
    cdecThrows(CBOR_ERROR_TAG_TYPE, 'd901009f43303030d819f6ff');
    cdecThrows(CBOR_ERROR_TAG_VALUE, 'd901009f43303030d81920ff');
    cdecThrows(CBOR_ERROR_TAG_VALUE, 'd901009f43303030d81901ff');
    // no-flag
    $data = 'd901009f43303030d81900ff';
    eq(['000', '000'], cdec($data, CBOR_BYTE));
    $value = new Cbor\Tag(Cbor\Tag::STRING_REF_NAMESPACE, [
        '000', new Cbor\Tag(Cbor\Tag::STRING_REF, 0),
    ]);
    eq($value, cdec($data, CBOR_BYTE, ['string_ref' => false]));
});

?>
--EXPECT--
Done.
