--TEST--
float values
--SKIPIF--
<?php
if (!extension_loaded('cbor')) echo 'skip  extension is not loaded';
// assumes IEEE 754 floating-point, binary32
elseif (bin2hex(pack('G', INF)) !== '7f800000') echo 'skip  unknown float implementation';
?>
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
    eq('3.140625', (string)cdec('f94248', CBOR_FLOAT16 | CBOR_FLOAT32));

    eq('0xfa40490fdb', cenc(new Cbor\Float32($m_pi)));
    eq('0xf94248', cenc(new Cbor\Float16($m_pi)));
    eq(new Cbor\Float32(3.1415927410125732), cdec('fa40490fdb'));
    eq(new Cbor\Float16(3.140625), cdec('f94248'));

    eq('0xfb7ff0000000000000', cenc(INF));
    eq('0xfaff800000', cenc(-INF, CBOR_FLOAT32));
    eq('0xf97c00', cenc(INF, CBOR_FLOAT16));
    eq('0xf97c00', cenc(INF, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf9fc00', cenc(-INF, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf97c00', cenc(123456.0, CBOR_FLOAT16));
    eq('0xfa47f12000', cenc(123456.0, CBOR_FLOAT32));
    eq('0xfa47f12000', cenc(123456.0, CBOR_FLOAT16 | CBOR_FLOAT32));
    // NaN, don't use PHP constant as its bit notation is compiler dependent
    eq('0xf97e00', cenc(hex2double('7ff8000000000000'), CBOR_FLOAT16));
    eq('0xfb0000000000000000', cenc(0.0));
    eq('0xfb8000000000000000', cenc(-0.0));
    // sNaN preservation during internal cast
    eq('0xfb7ff0040000000000', cenc(cdec('f97c01', CBOR_FLOAT16)));
    eq('0xfa7f802000', cenc(cdec('f97c01', CBOR_FLOAT16), CBOR_FLOAT32));
    eq('0xf97c01', cenc(cdec('f97c01', CBOR_FLOAT16), CBOR_FLOAT16));
    eq('0xf97c01', cenc(cdec('f97c01', CBOR_FLOAT16), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf97e00', cenc(cdec('fa7f800001', CBOR_FLOAT32), CBOR_FLOAT16));

    eq(1.5, (float)new Cbor\Float32(1.5));
    eq(1.5, (float)new Cbor\Float16(1.5));
    eq('0xf90000', cenc(0.0, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf94248', cenc(3.140625, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xfa40490fdb', cenc(3.1415927410125732, CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xf97e01', cenc((double)Cbor\Float32::fromBinary(hex2bin('7fc02000')), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xfb3ff0000010000000', cenc(hex2double('3ff0000010000000'), CBOR_FLOAT16 | CBOR_FLOAT32));
    eq('0xfa3f800001', cenc(hex2double('3ff0000020000000'), CBOR_FLOAT16 | CBOR_FLOAT32));

    eq('0xf93e00', cenc(1.5, CBOR_CDE));
    eq('0xfa49742408', cenc(1000000.5, CBOR_CDE));
    eq('0xfb416312d010000000', cenc(10000000.5, CBOR_CDE));
    eq('0xf97c00', cenc(INF, CBOR_CDE));
    eq('0xf9fc00', cenc(-INF, CBOR_CDE));
    eq('0xf97e00', cenc(cdec('f97e00', CBOR_FLOAT16), CBOR_CDE));
});

function hex2double(string $str): float
{
    return unpack('E', hex2bin($str))[1];
}

?>
--EXPECT--
Done.
