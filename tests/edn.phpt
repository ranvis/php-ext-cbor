--TEST--
Decode to diagnostic notation
--SKIPIF--
<?php if (!extension_loaded('cbor')) echo 'skip  extension is not loaded'; ?>
--FILE--
<?php

require_once __DIR__ . '/common.php';

run(function () {
    $data = '9f404201026443424f52438090a001827f657374726561646d696e67fffa47c35000a1f97e00f4bf82f6d5826161bf61626163fff5fff8f0ff';
    $diag = '[_ h\'\', h\'0102\', "CBOR", h\'8090a0\', 1, [(_ "strea", "ming"), 100000.0_2], {NaN_1: false}, {_ [null, 21(["a", {_ "b": "c"}])]: true}, simple(240)]';
    eq($diag, cdec($data, CBOR_EDN));
    eq($diag, cdec($data, CBOR_EDN, ['indent' => false]));
    eq(str_replace(' ', '', $diag), cdec($data, CBOR_EDN, ['space' => false]));
    $diag = <<<'END'
        [_
          h'',
          h'0102',
          "CBOR",
          h'8090a0',
          1,
          [
            (_
              "strea",
              "ming"
            ),
            100000.0_2
          ],
          {
            NaN_1: false
          },
          {_
            [
              null,
              21([
                "a",
                {_
                  "b": "c"
                }
              ])
            ]: true
          },
          simple(240)
        ]
        END;
    eq($diag, cdec($data, CBOR_EDN, ['indent' => 2]));
    eq(str_replace(' ', '', str_replace('  ', "\t", $diag)), cdec($data, CBOR_EDN, ['indent' => "\t", 'space' => false]));
    eq(str_replace('  ', '', $diag), cdec($data, CBOR_EDN, ['indent' => 0]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['indent' => 30]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['space' => 4]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['indent' => ">"]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['indent' => true]);

    // byte_space, byte_wrap
    $data = '581c00000000000000001111111111111111222222222222222233333333';
    eq('h\'00000000000000001111111111111111222222222222222233333333\'', cdec($data, CBOR_EDN));
    eq('h\'00000000000000001111111111111111222222222222222233333333\'', cdec($data, CBOR_EDN, ['byte_wrap' => false]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['byte_wrap' => true]);
    eq('h\'00 00 00 00 00 00 00 00 11 11 11 11 11 11 11 11 22 22 22 22 22 22 22 22 33 33 33 33\'', cdec($data, CBOR_EDN, ['byte_space' => 1]));
    eq('h\'00000000 00000000  11111111 11111111  22222222 22222222  33333333\'', cdec($data, CBOR_EDN, ['byte_space' => 4 | 8]));
    eq('h\'0000000000000000 1111111111111111\'
h\'2222222222222222 33333333\'', cdec($data, CBOR_EDN, ['byte_space' => 8, 'byte_wrap' => 16, 'indent' => 2]));
    eq('h\'00000000000000\' h\'00111111111111\' h\'11112222222222\' h\'22222233333333\'', cdec($data, CBOR_EDN, ['byte_space' => 8, 'byte_wrap' => 7]));
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['byte_space' => ~0]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['byte_space' => false]);
    cdecThrows(CBOR_ERROR_INVALID_OPTIONS, $data, CBOR_EDN, ['byte_wrap' => 0]);

    // text strings
    eq('"\n" h\'000d0a\'', cdec('640a000d0a', CBOR_EDN));
    // invalid UTF-8 sequences
    eq('"abc" h\'81c2\' "d"', cdec('6661626381c264', CBOR_EDN));
    eq('"" h\'818283\' "abc"', cdec('66818283616263', CBOR_EDN));
    eq('"" h\'8182830a\' "bc"', cdec('668182830a6263', CBOR_EDN));
    eq('"" h\'818283\' "a\n\b"', cdec('66818283610a08', CBOR_EDN));
    eq('"" h\'818283\'', cdec('63818283', CBOR_EDN));

    // self-describe flag
    $data = 'd9d9f7' . 'f6';
    eq('null', cdec($data, CBOR_EDN));
    eq('55799(null)', cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE));
    $data = 'd9d9f7480001020304050607';
    eq('55799(h\'0001020304050607\')', cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE, ['indent' => 2]));
    eq('55799(h\'0001020304050607\')', cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE, ['indent' => 2, 'byte_wrap' => 8]));
    eq(<<<'END'
        55799(
          h'00010203'
          h'04050607'
        )
        END, cdec($data, CBOR_EDN | CBOR_SELF_DESCRIBE, ['indent' => 2, 'byte_wrap' => 4]));

    // escape sequences
    $data = "\\\"\t\r\f\n\x08";
    $data = bin2hex(chr(0x60 + strlen($data)) . $data);
    eq('"\\\\\\"\\t\\r\\f\\n\\b"', cdec($data, CBOR_EDN));
    // text escaping
    $data = "\u{202e}efas\u{202c}";
    $data = bin2hex(chr(0x60 + strlen($data)) . $data);
    eq('"\u202eefas\u202c"', cdec($data, CBOR_EDN));
    // single control character
    $data = "\x00";
    $data = bin2hex(chr(0x60 + strlen($data)) . $data);
    eq('"\u0000"', cdec($data, CBOR_EDN));
    // empty indefinite string
    eq('\'\'_', cdec('5fff', CBOR_EDN));
    eq('""_', cdec('7fff', CBOR_EDN));

    // inconsistent string type
    eq(' /ERROR:' . CBOR_ERROR_SYNTAX . '/', cdec('5f60ff', CBOR_EDN));
    // break in definite length
    eq('[ /ERROR:' . CBOR_ERROR_SYNTAX . '/', cdec('81ff', CBOR_EDN));

    // simple values
    eq('simple(0)', cdec('e0', CBOR_EDN));
    eq('simple(19)', cdec('f3', CBOR_EDN));
    eq(' /ERROR:34/', cdec('f817', CBOR_EDN)); // not well-formed
    eq(' /ERROR:34/', cdec('f818', CBOR_EDN)); // reserved
    eq(' /ERROR:34/', cdec('f81f', CBOR_EDN)); // reserved
    eq('simple(32)', cdec('f820', CBOR_EDN));
    eq('simple(255)', cdec('f8ff', CBOR_EDN));
});

?>
--EXPECT--
Done.
