--TEST--
integer values
--SKIPIF--
<?php if (!extension_loaded("cbor")) print "skip"; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    eq('0x00', cenc(0));
    eq('0x01', cenc(1));
    eq('0x20', cenc(-1));
    eq('0x1818', cenc(24));
    eq('0x3818', cenc(-25));
    eq('0x18ff', cenc(+0xff));
    eq('0x38ff', cenc(-0x100));
    eq('0x190100', cenc(+0x100));
    eq('0x390100', cenc(-0x101));
    eq('0x19ffff', cenc(+0xffff));
    eq('0x39ffff', cenc(-0x1_0000));
    eq('0x1a00010000', cenc(+0x1_0000));
    eq('0x3a00010000', cenc(-0x1_0001));
    eq('0x1a7fffffff', cenc(+0x7fff_ffff));
    eq('0x3a7fffffff', cenc(-0x7fff_ffff - 1));
    if (PHP_INT_SIZE >= 8) {
        eq('0x1a80000000', cenc(+0x8000_0000));
        eq('0x3a80000000', cenc(-0x8000_0001));
        eq('0x1affffffff', cenc(+0xffff_ffff));
        eq('0x3affffffff', cenc(-0x1_0000_0000));
        eq('0x1b0000000100000000', cenc(+0x1_0000_0000));
        eq('0x3b0000000100000000', cenc(-0x1_0000_0001));
        eq('0x1b7fffffffffffffff', cenc(+0x7fff_ffff_ffff_ffff));
        eq('0x3b7fffffffffffffff', cenc(-0x7fff_ffff_ffff_ffff - 1));
    }

    eq(0, cdec('00'));
    eq(1, cdec('01'));
    eq(-1, cdec('20'));
    eq(24, cdec('1818'));
    eq(-25, cdec('3818'));
    eq(+0xff, cdec('18ff'));
    eq(-0x100, cdec('38ff'));
    eq(+0x100, cdec('190100'));
    eq(-0x101, cdec('390100'));
    eq(+0xffff, cdec('19ffff'));
    eq(-0x1_0000, cdec('39ffff'));
    eq(+0x1_0000, cdec('1a00010000'));
    eq(-0x1_0001, cdec('3a00010000'));
    eq(+0x7fff_ffff, cdec('1a7fffffff'));
    eq(-0x7fff_ffff - 1, cdec('3a7fffffff'));
    if (PHP_INT_SIZE >= 8) {
        eq(+0x8000_0000, cdec('1a80000000'));
        eq(-0x8000_0001, cdec('3a80000000'));
        eq(+0xffff_ffff, cdec('1affffffff'));
        eq(-0x1_0000_0000, cdec('3affffffff'));
        eq(+0x1_0000_0000, cdec('1b0000000100000000'));
        eq(-0x1_0000_0001, cdec('3b0000000100000000'));
        eq(+0x7fff_ffff_ffff_ffff, cdec('1b7fffffffffffffff'));
        eq(-0x7fff_ffff_ffff_ffff - 1, cdec('3b7fffffffffffffff'));

        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '1b8000000000000000');
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '3b8000000000000000');
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '1bffffffffffffffff');
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '3bffffffffffffffff');
    } else {
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '1a80000000');
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '3a80000000');
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '1affffffff');
        cdecThrows(CBOR_ERROR_UNSUPPORTED_VALUE, '3affffffff');
    }
});

?>
--EXPECT--
Done.
