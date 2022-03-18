--TEST--
bignum tag
--SKIPIF--
<?php if (
    !extension_loaded("cbor")
    || !extension_loaded("gmp")
) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x00', cenc(gmp_init('0')));
    eq('0x20', cenc(gmp_init('-1')));
    eq('0x1bfedcba9876543210', cenc(gmp_init('0xfedcba9876543210')));
    eq('0x3bfedcba9876543210', cenc(gmp_init('-0xfedcba9876543211')));
    eq('0xc240', cenc(gmp_init('0'), options:['bignum' => 'force']));
    eq('0xc340', cenc(gmp_init('-1'), options:['bignum' => 'force']));
    eq('0xc248fedcba9876543210', cenc(gmp_init('0xfedcba9876543210'), options:['bignum' => 'force']));
    eq('0xc348fedcba9876543210', cenc(gmp_init('-0xfedcba9876543211'), options:['bignum' => 'force']));
    eq('0xc249010000000000000000', cenc(gmp_init('0x10000000000000000')));

    // with string-ref, but shared_ref doesn't take effect
    $v = gmp_init('0x10000000000000000');
    eq('0x82c249010000000000000000c249010000000000000000', cenc([$v, $v], options: ['string_ref' => false, 'shared_ref' => true]));
    eq('0xd9010082c249010000000000000000c2d81900', cenc([$v, $v], options: ['string_ref' => true, 'shared_ref' => true]));
});

?>
--EXPECT--
Done.
