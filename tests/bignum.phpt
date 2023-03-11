--TEST--
bignum tag
--SKIPIF--
<?php
if (!extension_loaded('cbor')) echo 'skip  extension is not loaded';
elseif (!extension_loaded('gmp')) echo 'skip  GMP extension is not loaded';
?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x00', cenc(gmp_init('0')));
    eq('0x20', cenc(gmp_init('-1')));
    eq('0x1bfedcba9876543210', cenc(gmp_init('0xfedcba9876543210')));
    eq('0x3bfedcba9876543210', cenc(gmp_init('-0xfedcba9876543211')));
    eq('0x1bffffffffffffffff', cenc(gmp_init('0xffffffffffffffff')));
    eq('0x3bffffffffffffffff', cenc(gmp_init('-0x10000000000000000')));
    eq('0xc249010000000000000000', cenc(gmp_init('0x10000000000000000')));
    eq('0xc2581d02fde529a3274c649cfeb4b180adb5cb9602a9e0638ab2000000000000', cenc(gmp_fact(52)));

    // with string-ref, but shared_ref doesn't take effect for non-stdClass
    $v = gmp_init('0x10000000000000000');
    eq('0x82c249010000000000000000c249010000000000000000', cenc([$v, $v], options: ['string_ref' => false, 'shared_ref' => true]));
    eq('0xd9010082c249010000000000000000c2d81900', cenc([$v, $v], options: ['string_ref' => true, 'shared_ref' => true]));

    cencThrows(CBOR_ERROR_UNSUPPORTED_TYPE, gmp_init('0'), options: ['bignum' => false]);
});

?>
--EXPECT--
Done.
