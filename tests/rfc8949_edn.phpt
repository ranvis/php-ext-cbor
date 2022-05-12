--TEST--
Diagnostic notation, RFC 8949 examples
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0', cdec('00', CBOR_EDN));
    eq('1', cdec('01', CBOR_EDN));
    eq('10', cdec('0a', CBOR_EDN));
    eq('23', cdec('17', CBOR_EDN));
    eq('24', cdec('1818', CBOR_EDN));
    eq('25', cdec('1819', CBOR_EDN));
    eq('100', cdec('1864', CBOR_EDN));
    eq('1000', cdec('1903e8', CBOR_EDN));
    eq('1000000', cdec('1a000f4240', CBOR_EDN));
    eq('1000000000000', cdec('1b000000e8d4a51000', CBOR_EDN));
    eq('18446744073709551615', cdec('1bffffffffffffffff', CBOR_EDN));
    eq('2(h\'010000000000000000\')', cdec('c249010000000000000000', CBOR_EDN)); // 18446744073709551616
    eq('-18446744073709551616', cdec('3bffffffffffffffff', CBOR_EDN));
    eq('3(h\'010000000000000000\')', cdec('c349010000000000000000', CBOR_EDN)); // 010000000000000000
    eq('-1', cdec('20', CBOR_EDN));
    eq('-10', cdec('29', CBOR_EDN));
    eq('-100', cdec('3863', CBOR_EDN));
    eq('-1000', cdec('3903e7', CBOR_EDN));
    eq('0.0_1', cdec('f90000', CBOR_EDN));
    eq('-0.0_1', cdec('f98000', CBOR_EDN));
    eq('1.0_1', cdec('f93c00', CBOR_EDN));
    eq('1.1_3', cdec('fb3ff199999999999a', CBOR_EDN));
    eq('1.5_1', cdec('f93e00', CBOR_EDN));
    eq('65504.0_1', cdec('f97bff', CBOR_EDN));
    eq('100000.0_2', cdec('fa47c35000', CBOR_EDN));
	if (PHP_VERSION >= 80100) {
	    eq('3.4028235e+38_2', cdec('fa7f7fffff', CBOR_EDN)); // 3.4028234663852886e+38
	} else {
	    eq('3.40282347e+38_2', cdec('fa7f7fffff', CBOR_EDN)); // 3.4028234663852886e+38
	}
    eq('1.0e+300_3', cdec('fb7e37e43c8800759c', CBOR_EDN));
    eq('5.9605e-8_1', cdec('f90001', CBOR_EDN)); // 5.960464477539063
    eq('6.1035e-5_1', cdec('f90400', CBOR_EDN)); // 0.00006103515625
    eq('-4.0_1', cdec('f9c400', CBOR_EDN));
    eq('-4.1_3', cdec('fbc010666666666666', CBOR_EDN));
    eq('Infinity_1', cdec('f97c00', CBOR_EDN));
    eq('NaN_1', cdec('f97e00', CBOR_EDN));
    eq('-Infinity_1', cdec('f9fc00', CBOR_EDN));
    eq('Infinity_2', cdec('fa7f800000', CBOR_EDN));
    eq('NaN_2', cdec('fa7fc00000', CBOR_EDN));
    eq('-Infinity_2', cdec('faff800000', CBOR_EDN));
    eq('Infinity_3', cdec('fb7ff0000000000000', CBOR_EDN));
    eq('NaN_3', cdec('fb7ff8000000000000', CBOR_EDN));
    eq('-Infinity_3', cdec('fbfff0000000000000', CBOR_EDN));
    eq('false', cdec('f4', CBOR_EDN));
    eq('true', cdec('f5', CBOR_EDN));
    eq('null', cdec('f6', CBOR_EDN));
    eq('undefined', cdec('f7', CBOR_EDN));
    eq(' /ERROR:34/', cdec('f0', CBOR_EDN)); // simple(16)
    eq('simple(255)', cdec('f8ff', CBOR_EDN));
    eq('0("2013-03-21T20:04:00Z")', cdec('c074323031332d30332d32315432303a30343a30305a', CBOR_EDN));
    eq('1(1363896240)', cdec('c11a514b67b0', CBOR_EDN));
    eq('1(1363896240.5_3)', cdec('c1fb41d452d9ec200000', CBOR_EDN));
    eq('23(h\'01020304\')', cdec('d74401020304', CBOR_EDN));
    eq('24(h\'6449455446\')', cdec('d818456449455446', CBOR_EDN));
    eq('32("http://www.example.com")', cdec('d82076687474703a2f2f7777772e6578616d706c652e636f6d', CBOR_EDN));
    eq('h\'\'', cdec('40', CBOR_EDN));
    eq('h\'01020304\'', cdec('4401020304', CBOR_EDN));
    eq('""', cdec('60', CBOR_EDN));
    eq('"a"', cdec('6161', CBOR_EDN));
    eq('"IETF"', cdec('6449455446', CBOR_EDN));
    eq('"\\"\\\\"', cdec('62225c', CBOR_EDN));
    eq('"ü"', cdec('62c3bc', CBOR_EDN)); // "\u00fc"
    eq('"水"', cdec('63e6b0b4', CBOR_EDN)); // "\u6c34"
    eq('"𐅑"', cdec('64f0908591', CBOR_EDN)); // "\ud800\udd51"
    eq('[]', cdec('80', CBOR_EDN));
    eq('[1, 2, 3]', cdec('83010203', CBOR_EDN));
    eq('[1, [2, 3], [4, 5]]', cdec('8301820203820405', CBOR_EDN));
    eq('[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]', cdec('98190102030405060708090a0b0c0d0e0f101112131415161718181819', CBOR_EDN));
    eq('{}', cdec('a0', CBOR_EDN));
    eq('{1: 2, 3: 4}', cdec('a201020304', CBOR_EDN));
    eq('{"a": 1, "b": [2, 3]}', cdec('a26161016162820203', CBOR_EDN));
    eq('["a", {"b": "c"}]', cdec('826161a161626163', CBOR_EDN));
    eq('{"a": "A", "b": "B", "c": "C", "d": "D", "e": "E"}', cdec('a56161614161626142616361436164614461656145', CBOR_EDN));
    eq('(_ h\'0102\', h\'030405\')', cdec('5f42010243030405ff', CBOR_EDN));
    eq('(_ "strea", "ming")', cdec('7f657374726561646d696e67ff', CBOR_EDN));
    eq('[_ ]', cdec('9fff', CBOR_EDN));
    eq('[_ 1, [2, 3], [_ 4, 5]]', cdec('9f018202039f0405ffff', CBOR_EDN));
    eq('[_ 1, [2, 3], [4, 5]]', cdec('9f01820203820405ff', CBOR_EDN));
    eq('[1, [2, 3], [_ 4, 5]]', cdec('83018202039f0405ff', CBOR_EDN));
    eq('[1, [_ 2, 3], [4, 5]]', cdec('83019f0203ff820405', CBOR_EDN));
    eq('[_ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]', cdec('9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff', CBOR_EDN));
    eq('{_ "a": 1, "b": [_ 2, 3]}', cdec('bf61610161629f0203ffff', CBOR_EDN));
    eq('["a", {_ "b": "c"}]', cdec('826161bf61626163ff', CBOR_EDN));
    eq('{_ "Fun": true, "Amt": -2}', cdec('bf6346756ef563416d7421ff', CBOR_EDN));
});

?>
--EXPECT--
Done.
