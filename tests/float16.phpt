--TEST--
specific half-float values
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
    // INF
    eq('7f800000', floatHex(cdec('f97c00', CBOR_FLOAT16)));
    eq('0xf97c00', cenc(INF, CBOR_FLOAT16));
    eq('0xf97c00', cenc(65520.0, CBOR_FLOAT16));
    // -INF
    eq('ff800000', floatHex(cdec('f9fc00', CBOR_FLOAT16)));
    eq('0xf9fc00', cenc(-INF, CBOR_FLOAT16));
    // NAN
    eq(bin32Hex(0b0_11111111_100_0000_0000_0000_0000_0000), floatHex(cdec('f9' . bin16Hex(0b0_11111_1000000000), CBOR_FLOAT16)));
    eq('0xf9' . bin16Hex(0b0_11111_1000000000), cenc(floatBin(0b0_11111111_100_0000_0000_0000_0000_0000), CBOR_FLOAT16));
    // NAN bits flipped (except signaling)
    eq(
        bin32Hex('ffffe000'), // 0b1_11111111_111_1111_1110_0000_0000_0000
        floatHex(cdec('f9' . bin16Hex(0b1_11111_1111111111), CBOR_FLOAT16)),
    );
    eq('0xf9' . bin16Hex(0b1_11111_1111111111), cenc(floatBin('ffffe000'), CBOR_FLOAT16));
    // zeros
    eq('0xf90000', cenc(0.0, CBOR_FLOAT16));
    eq('0xf98000', cenc(-0.0, CBOR_FLOAT16));
    eq('0xf98000', cenc(-2**-25, CBOR_FLOAT16));
    eq('0xf98001', cenc(-2**-24, CBOR_FLOAT16));
});

function floatHex(float $num): string
{
    return bin2hex(pack('G', $num));
}

function floatBin(int|string $num): float
{
    $num = is_string($num) ? hex2bin($num) : pack('N', $num);
    return unpack('G', $num)[1];
}

function bin32Hex(int|string $num): string
{
    $num = is_string($num) ? hex2bin($num) : pack('N', $num);
    return unpack('H*', $num)[1];
}

function bin16Hex(int $num): string
{
    return unpack('H*', pack('n', $num))[1];
}

?>
--EXPECT--
Done.
