--TEST--
decimal tag
--SKIPIF--
<?php
if (!extension_loaded('cbor')) echo 'skip  extension is not loaded';
elseif (!extension_loaded('decimal')) echo 'skip  Decimal extension is not loaded';
?>
--FILE--
<?php

use Decimal\Decimal;

require_once __DIR__ . '/common.php';

run(function () {
    // RFC 8949
    eq('0xc48221196ab3', cenc(new Decimal('273.15')));

    eq('0x00', cenc(new Decimal('0')));
    eq('0x00', cenc(new Decimal('-0')));
    eq('0xc4822000', cenc(new Decimal('-0.0')));
    eq('0xc4822000', cenc(new Decimal('0.0')));
    eq('0xc482381f00', cenc(new Decimal('-0.00000000000000000000000000000000')));
    eq(cenc(NAN), cenc(new Decimal(NAN)));
    eq(cenc(INF), cenc(new Decimal(INF)));
    eq(cenc(-INF), cenc(new Decimal(-INF)));
    eq('0xfa7f800000', cenc(new Decimal(INF), CBOR_FLOAT32));
    eq('0xf97c00', cenc(new Decimal(INF), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf97c00', cenc(new Decimal(INF), CBOR_CDE));
    eq(cenc(NAN, CBOR_FLOAT16), cenc(new Decimal(NAN), CBOR_CDE));
    eq('0x1bffffffffffffffff', cenc(new Decimal('18446744073709551615')));
    eq('0x3bffffffffffffffff', cenc(new Decimal('-18446744073709551616')));
    eq('0xc48200c249010000000000000000', cenc(new Decimal('18446744073709551616')));
    eq('0xc48200c349010000000000000000', cenc(new Decimal('-18446744073709551617')));

    eq('0xc4822d1b0000f739ee4749d0', cenc(new Decimal('2.71828182845904')));
    eq('0xc482071ab7ebb40d', cenc(new Decimal('3.085677581e16', 10)));
    eq('0xc482381b3a5f7f4679', cenc(new Decimal('-1.602176634e-19', 10)));
    eq('0xc48200c2581d02fde529a3274c649cfeb4b180adb5cb9602a9e0638ab2000000000000', cenc(new Decimal('80658175170943878571660636856403766975289505440883277824000000000000', 100)));
    eq('0xc4820ac258190148f3106af09263adac713a02b9869d05be86b08000000000', cenc(new Decimal('8065817517094387857166063685640376697528950544088327782400e10', 100)));

    cencThrows(CBOR_ERROR_UNSUPPORTED_TYPE, new Decimal('0'), options: ['decimal' => false]);
});

?>
--EXPECT--
Done.
