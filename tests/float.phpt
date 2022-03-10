--TEST--
float values
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    ini_set('precision', 15);
    $m_pi = 3.141592653589793;
    eq('0xfb400921fb54442d18', cenc($m_pi));
    eq('0xfa40490fdb', cenc($m_pi, CBOR_FLOAT32));
    eq('0xf94248', cenc($m_pi, CBOR_FLOAT16));
    eq('3.14159265358979', (string)cdec('fb400921fb54442d18'));
    eq('3.1415927', substr((string)cdec('fa40490fdb', CBOR_FLOAT32), 0, 9));
    eq('3.14', substr((string)cdec('f94248', CBOR_FLOAT16), 0, 4));

    eq('0xfa40490fdb', cenc(new Cbor\Float32($m_pi)));
    eq('0xf94248', cenc(new Cbor\Float16($m_pi)));
    eq(new Cbor\Float32(3.1415927410125732), cdec('fa40490fdb'));
    eq(new Cbor\Float16(3.140625), cdec('f94248'));

    eq('0xfb7ff0000000000000', cenc(INF));
    eq('0xfaff800000', cenc(-INF, CBOR_FLOAT32));
    eq('0xf97e00', cenc(NAN, CBOR_FLOAT16));
    eq('0xfb0000000000000000', cenc(0.0));
    eq('0xfb8000000000000000', cenc(-0.0));

    eq(1.5, (float)new Cbor\Float32(1.5));
    eq(1.5, (float)new Cbor\Float16(1.5));
});

?>
--EXPECT--
Done.
