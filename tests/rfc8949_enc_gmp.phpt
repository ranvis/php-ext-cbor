--TEST--
Encode RFC 8949 examples
--SKIPIF--
<?php
if (!extension_loaded('cbor')) echo 'skip  extension is not loaded';
elseif (!extension_loaded('gmp')) echo 'skip  GMP extension is not loaded';
?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x1bffffffffffffffff', cenc(gmp_init('18446744073709551615')));
    eq('0xc249010000000000000000', cenc(gmp_init('18446744073709551616')));
    eq('0x3bffffffffffffffff', cenc(gmp_init('-18446744073709551616')));
    eq('0xc349010000000000000000', cenc(gmp_init('-18446744073709551617')));
});

?>
--EXPECT--
Done.
