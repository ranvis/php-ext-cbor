--TEST--
float values
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $m_pi = 3.141592653589793;
    eq('0xfb400921fb54442d18', cenc($m_pi));
    eq('0xfa40490fdb', cenc($m_pi, CBOR_FLOAT32));
    eq('0xf94248', cenc($m_pi, CBOR_FLOAT16));
    eq('0x400921fb54442d18', toDump(pack('E', (float)cdec('fb400921fb54442d18'))));
    eq('0x400921fb60000000', toDump(pack('E', (float)cdec('fa40490fdb', CBOR_FLOAT32))));
    eq('3.140625', (string)cdec('f94248', CBOR_FLOAT16));

    eq('0xfa40490fdb', cenc(new Cbor\Float32($m_pi)));
    eq('0xf94248', cenc(new Cbor\Float16($m_pi)));
    eq(new Cbor\Float32(3.1415927410125732), cdec('fa40490fdb'));
    eq(new Cbor\Float16(3.140625), cdec('f94248'));

    eq('0xfb7ff0000000000000', cenc(INF));
    eq('0xfaff800000', cenc(-INF, CBOR_FLOAT32));
    /* NaN, don't use PHP constant as its bit notation is compiler dependent */
    eq('0xf97e00', cenc(hex2double('7ff8000000000000'), CBOR_FLOAT16));
    eq('0xfb0000000000000000', cenc(0.0));
    eq('0xfb8000000000000000', cenc(-0.0));

    eq(1.5, (float)new Cbor\Float32(1.5));
    eq(1.5, (float)new Cbor\Float16(1.5));
    eq('0xf94248', cenc(3.140625, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xfa40490fdb', cenc(3.1415927410125732, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf97e01', cenc((double)Cbor\Float32::fromBinary(hex2bin('7fc02000')), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xfb3ff0000010000000', cenc(hex2double('3ff0000010000000'), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xfa3f800001', cenc(hex2double('3ff0000020000000'), CBOR_FLOAT16 | CBOR_FLOAT32));
});

function hex2double(string $str): float
{
    return unpack('E', hex2bin($str))[1];
}

?>
--EXPECT--
Done.
