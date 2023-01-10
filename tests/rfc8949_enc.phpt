--TEST--
Encode RFC 8949 examples
--SKIPIF--
<?php
if (!extension_loaded('cbor')) echo 'skip  extension is not loaded';
elseif (!extension_loaded('mbstring')) echo 'skip  mbstring extension is not loaded';
?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    // integers
    eq('0x00', cenc(0));
    eq('0x01', cenc(1));
    eq('0x0a', cenc(10));
    eq('0x17', cenc(23));
    eq('0x1818', cenc(24));
    eq('0x1819', cenc(25));
    eq('0x1864', cenc(100));
    eq('0x1903e8', cenc(1000));
    eq('0x1a000f4240', cenc(1000000));
    if (PHP_INT_SIZE >= 8) {
        eq('0x1b000000e8d4a51000', cenc(1000000000000));
    }
    eq('0x20', cenc(-1));
    eq('0x29', cenc(-10));
    eq('0x3863', cenc(-100));
    eq('0x3903e7', cenc(-1000));

    // floats
    eq('0xf90000', cenc(0.0, CBOR_FLOAT16));
    eq('0xf98000', cenc(-0.0, CBOR_FLOAT16));
    eq('0xf93c00', cenc(1.0, CBOR_FLOAT16));
    eq('0xfb3ff199999999999a', cenc(1.1));
    eq('0xf93e00', cenc(1.5, CBOR_FLOAT16));
    eq('0xf97bff', cenc(65504.0, CBOR_FLOAT16));
    eq('0xfa47c35000', cenc(100000.0, CBOR_FLOAT32));
    eq('0xfa7f7fffff', cenc(3.4028234663852886e+38, CBOR_FLOAT32));
    eq('0xfb7e37e43c8800759c', cenc(1.0e+300));
    eq('0xf90001', cenc(5.960464477539063e-8, CBOR_FLOAT16));
    eq('0xf90400', cenc(0.00006103515625, CBOR_FLOAT16));
    eq('0xf9c400', cenc(-4.0, CBOR_FLOAT16));
    eq('0xfbc010666666666666', cenc(-4.1));
    eq('0xf97c00', cenc(INF, CBOR_FLOAT16));
    eq('0xf97e00', cenc(NAN, CBOR_FLOAT16));
    eq('0xf9fc00', cenc(-INF, CBOR_FLOAT16));
    eq('0xfa7f800000', cenc(INF, CBOR_FLOAT32));
    eq('0xfa7fc00000', cenc(NAN, CBOR_FLOAT32));
    eq('0xfaff800000', cenc(-INF, CBOR_FLOAT32));
    eq('0xfb7ff0000000000000', cenc(INF));
    eq('0xfb7ff8000000000000', cenc(NAN));
    eq('0xfbfff0000000000000', cenc(-INF));

    // simple values
    eq('0xf4', cenc(false));
    eq('0xf5', cenc(true));
    eq('0xf6', cenc(null));
    eq('0xf7', cenc(Cbor\Undefined::get()));

    //0xf0 === simple(16)
    //0xf8ff === simple(255)

    // tagged values
    eq('0xc074323031332d30332d32315432303a30343a30305a', cenc(new Cbor\Tag(0, "2013-03-21T20:04:00Z"), CBOR_TEXT));
    eq('0xc11a514b67b0', cenc(new Cbor\Tag(1, 1363896240)));
    eq('0xc1fb41d452d9ec200000', cenc(new Cbor\Tag(1, 1363896240.5)));
    eq('0xd74401020304', cenc(new Cbor\Tag(23, "\x01\x02\x03\x04")));
    eq('0xd818456449455446', cenc(new Cbor\Tag(24, 'dIETF')));
    eq('0xd82076687474703a2f2f7777772e6578616d706c652e636f6d', cenc(new Cbor\Tag(32, "http://www.example.com"), CBOR_TEXT));

    // strings
    eq('0x40', cenc('', CBOR_BYTE));
    eq('0x4401020304', cenc(hex2bin('01020304'), CBOR_BYTE));
    eq('0x60', cenc("", CBOR_TEXT));
    eq('0x6161', cenc("a", CBOR_TEXT));
    eq('0x6449455446', cenc("IETF", CBOR_TEXT));
    eq('0x62225c', cenc("\"\\", CBOR_TEXT));
    eq('0x62c3bc', cenc("\u{00fc}", CBOR_TEXT));
    eq('0x63e6b0b4', cenc("\u{6c34}", CBOR_TEXT));
    eq('0x64f0908591', cenc(mb_convert_encoding(hex2bin('d800dd51'), 'UTF-8', 'UTF-16BE'), CBOR_TEXT));

    // arrays
    eq('0x80', cenc([]));
    eq('0x83010203', cenc([1, 2, 3]));
    eq('0x8301820203820405', cenc([1, [2, 3], [4, 5]]));
    eq('0x98190102030405060708090a0b0c0d0e0f101112131415161718181819', cenc(range(1, 25)));

    // maps
    eq('0xa0', cenc((object)[], CBOR_TEXT));
    //0xa201020304 === (object)[1 => 2, 3 => 4]
    eq('0xa26161016162820203', cenc((object)["a" => 1, "b" => [2, 3]], CBOR_KEY_TEXT));
    eq('0x826161a161626163', cenc(["a", (object)["b" => "c"]], CBOR_TEXT | CBOR_KEY_TEXT));
    eq('0xa56161614161626142616361436164614461656145', cenc((object)["a" => "A", "b" => "B", "c" =>"C", "d" => "D", "e" => "E"], CBOR_TEXT | CBOR_KEY_TEXT));

    // indefinite length
    //0x5f42010243030405ff === (_ h'0102', h'030405')
    //0x7f657374726561646d696e67ff === (_ "strea", "ming")
    //0x9fff === [_ ]
    //0x9f018202039f0405ffff === [_ 1, [2, 3], [_ 4, 5]]
    //0x9f01820203820405ff === [_ 1, [2, 3], [4, 5]]
    //0x83018202039f0405ff === [1, [2, 3], [_ 4, 5]]
    //0x83019f0203ff820405 === [1, [_ 2, 3], [4, 5]]
    //0x9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff === [_ range(1, 25)]
    //0xbf61610161629f0203ffff === {_ "a": 1, "b": [_ 2, 3]}
    //0x826161bf61626163ff === ["a", {_ "b": "c"}]
    //0xbf6346756ef563416d7421ff === {_ "Fun": true, "Amt": -2}
});

?>
--EXPECT--
Done.
