--TEST--
Decode RFC 8949 examples
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
    eq(0, cdec('00'));
    eq(1, cdec('01'));
    eq(10, cdec('0a'));
    eq(23, cdec('17'));
    eq(24, cdec('1818'));
    eq(25, cdec('1819'));
    eq(100, cdec('1864'));
    eq(1000, cdec('1903e8'));
    eq(1000000, cdec('1a000f4240'));
    if (PHP_INT_SIZE >= 8) {
        eq(1000000000000, cdec('1b000000e8d4a51000'));
    }
    eq(-1, cdec('20'));
    eq(-10, cdec('29'));
    eq(-100, cdec('3863'));
    eq(-1000, cdec('3903e7'));

    // floats
    eq(new Cbor\Float16(0.0), cdec('f90000'));
    eq(new Cbor\Float16(-0.0), cdec('f98000'));
    eq(new Cbor\Float16(1.0), cdec('f93c00'));
    eq(1.1, cdec('fb3ff199999999999a'));
    eq(new Cbor\Float16(1.5), cdec('f93e00'));
    eq(new Cbor\Float16(65504.0), cdec('f97bff'));
    eq(new Cbor\Float32(100000.0), cdec('fa47c35000'));
    eq(new Cbor\Float32(3.4028234663852886e+38), cdec('fa7f7fffff'));
    eq(1.0e+300, cdec('fb7e37e43c8800759c'));
    eq(new Cbor\Float16(5.960464477539063e-8), cdec('f90001'));
    eq(new Cbor\Float16(0.00006103515625), cdec('f90400'));
    eq(new Cbor\Float16(-4.0), cdec('f9c400'));
    eq(-4.1, cdec('fbc010666666666666'));
    eq(new Cbor\Float16(INF), cdec('f97c00'));
    eq(new Cbor\Float16(NAN), cdec('f97e00'));
    eq(new Cbor\Float16(-INF), cdec('f9fc00'));
    eq(new Cbor\Float32(INF), cdec('fa7f800000'));
    eq(new Cbor\Float32(NAN), cdec('fa7fc00000'));
    eq(new Cbor\Float32(-INF), cdec('faff800000'));
    eq(INF, cdec('fb7ff0000000000000'));
    eq(NAN, cdec('fb7ff8000000000000'));
    eq(-INF, cdec('fbfff0000000000000'));

    // simple values
    eq(false, cdec('f4'));
    eq(true, cdec('f5'));
    eq(null, cdec('f6'));
    eq(Cbor\Undefined::get(), cdec('f7'));

    //eq(simple(16), cdec('f0'));
    //eq(simple(255), cdec('f8ff'));

    // tagged values
    eq(new Cbor\Tag(0, '2013-03-21T20:04:00Z'), cdec('c074323031332d30332d32315432303a30343a30305a', CBOR_TEXT));
    eq(new Cbor\Tag(1, 1363896240), cdec('c11a514b67b0'));
    eq(new Cbor\Tag(1, 1363896240.5), cdec('c1fb41d452d9ec200000'));
    eq(new Cbor\Tag(23, hex2bin('01020304')), cdec('d74401020304'));
    eq(new Cbor\Tag(24, hex2bin('6449455446')), cdec('d818456449455446'));
    eq(new Cbor\Tag(32, 'http://www.example.com'), cdec('d82076687474703a2f2f7777772e6578616d706c652e636f6d', CBOR_TEXT));

    // strings
    eq('', cdec('40'));
    eq(hex2bin('01020304'), cdec('4401020304'));
    eq('', cdec('60', CBOR_TEXT));
    eq('a', cdec('6161', CBOR_TEXT));
    eq('IETF', cdec('6449455446', CBOR_TEXT));
    eq('"\\', cdec('62225c', CBOR_TEXT));
    eq("\u{00fc}", cdec('62c3bc', CBOR_TEXT));
    eq("\u{6c34}", cdec('63e6b0b4', CBOR_TEXT));
    eq(mb_convert_encoding(hex2bin('d800dd51'), 'UTF-8', 'UTF-16BE'), cdec('64f0908591', CBOR_TEXT));

    // arrays
    eq([], cdec('80'));
    eq([1, 2, 3], cdec('83010203'));
    eq([1, [2, 3], [4, 5]], cdec('8301820203820405'));
    eq(range(1, 25), cdec('98190102030405060708090a0b0c0d0e0f101112131415161718181819'));

    // maps
    eq((object)[], cdec('a0'));
    eq((object)[1 => 2, 3 => 4], cdec('a201020304', CBOR_INT_KEY));
    eq((object)['a' => 1, 'b' => [2, 3]], cdec('a26161016162820203', CBOR_TEXT | CBOR_KEY_TEXT));
    eq(['a', (object)['b' => 'c']], cdec('826161a161626163', CBOR_TEXT | CBOR_KEY_TEXT));
    eq((object)['a' => 'A', 'b' => 'B', 'c' => 'C', 'd' => 'D', 'e' => 'E'], cdec('a56161614161626142616361436164614461656145', CBOR_TEXT | CBOR_KEY_TEXT));

    // indefinite length
    eq(hex2bin('0102') . hex2bin('030405'), cdec('5f42010243030405ff'));
    eq('strea' . 'ming', cdec('7f657374726561646d696e67ff', CBOR_TEXT | CBOR_KEY_TEXT));
    eq([], cdec('9fff'));
    eq([1, [2, 3], [4, 5]], cdec('9f018202039f0405ffff'));
    eq([1, [2, 3], [4, 5]], cdec('9f01820203820405ff'));
    eq([1, [2, 3], [4, 5]], cdec('83018202039f0405ff'));
    eq([1, [2, 3], [4, 5]], cdec('83019f0203ff820405'));
    eq(range(1, 25), cdec('9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff'));
    eq((object)['a' => 1, 'b' => [2, 3]], cdec('bf61610161629f0203ffff', CBOR_TEXT | CBOR_KEY_TEXT));
    eq(['a', (object)['b' => 'c']], cdec('826161bf61626163ff', CBOR_TEXT | CBOR_KEY_TEXT));
    eq((object)['Fun' => true, 'Amt' => -2], cdec('bf6346756ef563416d7421ff', CBOR_TEXT | CBOR_KEY_TEXT));
});

?>
--EXPECT--
Done.
